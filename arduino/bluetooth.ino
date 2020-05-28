#include <STBLE.h>
#include "bluetooth.h"

#include <stdio.h>
#include <SPI.h>

#define COPY_UUID_128(uuid_struct, uuid_15, uuid_14, uuid_13, uuid_12, uuid_11, uuid_10, uuid_9, uuid_8, uuid_7, uuid_6, uuid_5, uuid_4, uuid_3, uuid_2, uuid_1, uuid_0) \
  do {\
    uuid_struct[0]  = uuid_0;   uuid_struct[1] = uuid_1;   uuid_struct[2] = uuid_2;   uuid_struct[3] = uuid_3; \
    uuid_struct[4]  = uuid_4;   uuid_struct[5] = uuid_5;   uuid_struct[6] = uuid_6;   uuid_struct[7] = uuid_7; \
    uuid_struct[8]  = uuid_8;   uuid_struct[9] = uuid_9;  uuid_struct[10] = uuid_10; uuid_struct[11] = uuid_11; \
    uuid_struct[12] = uuid_12; uuid_struct[13] = uuid_13; uuid_struct[14] = uuid_14; uuid_struct[15] = uuid_15; \
  }while(0)

#define COPY_UART_SERVICE_UUID(uuid_struct)  COPY_UUID_128(uuid_struct,0x6E, 0x40, 0x00, 0x01, 0xB5, 0xA3, 0xF3, 0x93, 0xE0, 0xA9, 0xE5, 0x0E, 0x24, 0xDC, 0xCA, 0x9E)
#define COPY_UART_TX_CHAR_UUID(uuid_struct)  COPY_UUID_128(uuid_struct,0x6E, 0x40, 0x00, 0x02, 0xB5, 0xA3, 0xF3, 0x93, 0xE0, 0xA9, 0xE5, 0x0E, 0x24, 0xDC, 0xCA, 0x9E)
#define COPY_UART_RX_CHAR_UUID(uuid_struct)  COPY_UUID_128(uuid_struct,0x6E, 0x40, 0x00, 0x03, 0xB5, 0xA3, 0xF3, 0x93, 0xE0, 0xA9, 0xE5, 0x0E, 0x24, 0xDC, 0xCA, 0x9E)
#define PIPE_UART_OVER_BTLE_UART_TX_TX       0

/*=== CONFIGURATION AND GLOBALS ======*/
#define  ADV_INTERVAL_MIN_MS  50
#define  ADV_INTERVAL_MAX_MS  100

volatile uint8_t ble_enabled      = 0;
volatile uint8_t do_connectable   = 0;
volatile uint8_t do_unconnectable = 0;

#define BLE_UART_RX_BUFFSIZE 21
#define BLE_UART_TX_BUFFSIZE 19
uint8_t ble_rx_buffer[BLE_UART_RX_BUFFSIZE];
uint8_t ble_rx_buffer_len = 0;

uint16_t BLEConnHandle,
         UARTServHandle,
         UARTTXCharHandle,
         UARTRXCharHandle;

// Bluetooth MAC Address
uint8_t BDADDR[] = {0x12, 0x34, 0x00, 0xE1, 0x80, 0x22};

// Bluetooth Names
const char LOCAL_NAME[] = {AD_TYPE_COMPLETE_LOCAL_NAME, 'O', 'p', 'e', 'n', 'O', 'B', 'D'};
const char BDNAME[] = "OpenOBD";

// forward declarations
void print_status(uint8_t bles, const char* msg);

/*=== BLE API ========================*/

// makes the device discoverable
void setConnectable(void) {
  tBleStatus ret;
  if (!ble_enabled) return;
  
  hci_le_set_scan_resp_data(0, NULL);
  ret = aci_gap_set_discoverable(ADV_IND,
                                 (ADV_INTERVAL_MIN_MS * 1000) / 625,
                                 (ADV_INTERVAL_MAX_MS * 1000) / 625,
                                 STATIC_RANDOM_ADDR,
                                 NO_WHITE_LIST_USE,
                                 sizeof(LOCAL_NAME),
                                 LOCAL_NAME, 0, NULL, 0, 0);
  
  if (ret == BLE_STATUS_SUCCESS)
    printf("BLE: Module is now discoverable\r\n");
  else
    print_status(ret, "BLE: Could not enter discoverable mode: ");
  do_connectable = 0;
}

// makes the device undiscoverable
void setNonConnectable(void) {
  tBleStatus ret;
  
  hci_le_set_scan_resp_data(0, NULL);
  ret = aci_gap_set_non_discoverable();
  
  if (ret == BLE_STATUS_SUCCESS)
    printf("BLE: Module is now undiscoverable\r\n");
  else
    print_status(ret, "BLE: Could not enter undiscoverable mode: ");
  
  aci_hal_device_standby();
  do_unconnectable = 0;
}

// used to initiate the UART profile on the ble module
// TODO implement error checking
uint8_t Add_UART_Service() {
  tBleStatus ret;
  uint8_t uuid[16];

  COPY_UART_SERVICE_UUID(uuid);
  ret = aci_gatt_add_serv(UUID_TYPE_128, uuid, PRIMARY_SERVICE, 7, &UARTServHandle);
  //if (ret != BLE_STATUS_SUCCESS) return BLE_STATUS_ERROR ;

  COPY_UART_TX_CHAR_UUID(uuid);
  ret =  aci_gatt_add_char(UARTServHandle, UUID_TYPE_128, uuid, 20, CHAR_PROP_WRITE_WITHOUT_RESP, ATTR_PERMISSION_NONE, GATT_NOTIFY_ATTRIBUTE_WRITE, 16, 1, &UARTTXCharHandle);
  //if (ret != BLE_STATUS_SUCCESS) return BLE_STATUS_ERROR ;

  COPY_UART_RX_CHAR_UUID(uuid);
  ret =  aci_gatt_add_char(UARTServHandle, UUID_TYPE_128, uuid, 20, CHAR_PROP_NOTIFY,             ATTR_PERMISSION_NONE, 0,                           16, 1, &UARTRXCharHandle);
  //if (ret != BLE_STATUS_SUCCESS) return BLE_STATUS_ERROR;

  return BLE_STATUS_SUCCESS;
}

// this allows a read of the attributes installed in Add_UART_Service
void OnReadRequest(uint16_t handle) {
  if (BLEConnHandle != 0)
    aci_gatt_allow_read(BLEConnHandle);
}

// when the UART attribute is modified, we should notify the applicaiton
void OnAttributeModified(uint16_t handle, uint8_t data_length, uint8_t *att_data) {
  if (handle == UARTTXCharHandle + 1) {
    int i;
    for (i = 0; i < data_length; i++) {
      ble_rx_buffer[i] = att_data[i];
    }
    ble_rx_buffer[i] = '\0';
    ble_rx_buffer_len = data_length;
    bluetooth_on_uart_rx(ble_rx_buffer, ble_rx_buffer_len);
  }
}

// occurs when a point-to-point connection is established
void OnConnectionComplete(uint8_t addr[6], uint16_t connection_handle) {
  printf("BLE: Connected to MAC ");
  for (int i = 5; i > 0; i--) {
    printf("%02X-", addr[i]);
  }
  printf("%02X -> #%hu\r\n", addr[0], connection_handle);

  BLEConnHandle = connection_handle;
  do_unconnectable = 1;
}

// occurs when a point-to-point connection is dropped
void OnDisconnectionComplete(void) {
  printf("BLE: Disconnected\r\n");

  BLEConnHandle = 0;
  do_connectable = 1;
}

// occurs when an authorization request is recieved
void OnAuthorizationRequest(uint16_t handle) {
  tBleStatus ret;
  if (BLEConnHandle != 0) {
    ret = aci_gap_authorization_response(handle, CONNECTION_AUTHORIZED);
    if (ret != BLE_STATUS_SUCCESS) print_status(ret, "Error during auth response: ");
  }
}

// the real meat and potatoes of the loop logic
extern "C" void HCI_Event_CB(void *pckt);
void HCI_Event_CB(void *pckt) {
  hci_uart_pckt *hci_pckt = (hci_uart_pckt *)pckt;
  hci_event_pckt *event_pckt = (hci_event_pckt*)hci_pckt->data;

  if (hci_pckt->type != HCI_EVENT_PKT) return;

  switch (event_pckt->evt) {
    
    case EVT_DISCONN_COMPLETE: {
      OnDisconnectionComplete();
      break;
    }
    
    case EVT_LE_META_EVENT: {
      evt_le_meta_event *evt = (evt_le_meta_event *)event_pckt->data;
      switch (evt->subevent) {
        case EVT_LE_CONN_COMPLETE: {
          evt_le_connection_complete *cc = (evt_le_connection_complete *)evt->data;
          OnConnectionComplete(cc->peer_bdaddr, cc->handle);
          break;
        }
      }
      break;
    }
    
    case EVT_VENDOR: {
      evt_blue_aci *blue_evt = (evt_blue_aci*) event_pckt->data;
      switch (blue_evt->ecode) {
        case EVT_BLUE_GATT_READ_PERMIT_REQ: {
          evt_gatt_read_permit_req *pr = (evt_gatt_read_permit_req*) blue_evt->data;
          OnReadRequest(pr->attr_handle);
          break;
        }
        case EVT_BLUE_GATT_ATTRIBUTE_MODIFIED: {
          evt_gatt_attr_modified_IDB05A1 *evt = (evt_gatt_attr_modified_IDB05A1*) blue_evt->data;
          OnAttributeModified(evt->attr_handle, evt->data_length, evt->att_data);
          break;
        }
      }
      break;
    }

    case EVT_BLUE_GAP_AUTHORIZATION_REQUEST: {
      evt_gap_author_req *auth_evt = (evt_gap_author_req*) event_pckt->data;
      OnAuthorizationRequest(auth_evt->conn_handle);
      break;
    }

    case EVT_BLUE_GAP_PAIRING_CMPLT: {
      evt_gap_pairing_cmplt *pair_evt = (evt_gap_pairing_cmplt*) event_pckt->data;
      // TODO: OnPairingComplete();
      break;
    }
    
  }
}

// prints status codes to readable form
void print_status(uint8_t bles, const char* msg) {
  printf("BLE: %s", msg);
  switch (bles) {
    //case ERR_CMD_SUCCESS: printf("ERR_CMD_SUCCESS"); break;
    case BLE_STATUS_SUCCESS: printf("BLE_STATUS_SUCCESS"); break;
    case ERR_UNKNOWN_HCI_COMMAND: printf("ERR_UNKNOWN_HCI_COMMAND"); break;
    case ERR_UNKNOWN_CONN_IDENTIFIER: printf("ERR_UNKNOWN_CONN_IDENTIFIER"); break;
    case ERR_AUTH_FAILURE: printf("ERR_AUTH_FAILURE"); break;
    case ERR_PIN_OR_KEY_MISSING: printf("ERR_PIN_OR_KEY_MISSING"); break;
    case ERR_MEM_CAPACITY_EXCEEDED: printf("ERR_MEM_CAPACITY_EXCEEDED"); break;
    case ERR_CONNECTION_TIMEOUT: printf("ERR_CONNECTION_TIMEOUT"); break;
    case ERR_COMMAND_DISALLOWED: printf("ERR_COMMAND_DISALLOWED"); break;
    case ERR_UNSUPPORTED_FEATURE: printf("ERR_UNSUPPORTED_FEATURE"); break;
    case ERR_INVALID_HCI_CMD_PARAMS: printf("ERR_INVALID_HCI_CMD_PARAMS"); break;
    case ERR_RMT_USR_TERM_CONN: printf("ERR_RMT_USR_TERM_CONN"); break;
    case ERR_RMT_DEV_TERM_CONN_LOW_RESRCES: printf("ERR_RMT_DEV_TERM_CONN_LOW_RESRCES"); break;
    case ERR_RMT_DEV_TERM_CONN_POWER_OFF: printf("ERR_RMT_DEV_TERM_CONN_POWER_OFF"); break;
    case ERR_LOCAL_HOST_TERM_CONN: printf("ERR_LOCAL_HOST_TERM_CONN"); break;
    case ERR_UNSUPP_RMT_FEATURE: printf("ERR_UNSUPP_RMT_FEATURE"); break;
    case ERR_INVALID_LMP_PARAM: printf("ERR_INVALID_LMP_PARAM"); break;
    case ERR_UNSPECIFIED_ERROR: printf("ERR_UNSPECIFIED_ERROR"); break;
    case ERR_LL_RESP_TIMEOUT: printf("ERR_LL_RESP_TIMEOUT"); break;
    case ERR_LMP_PDU_NOT_ALLOWED: printf("ERR_LMP_PDU_NOT_ALLOWED"); break;
    case ERR_INSTANT_PASSED: printf("ERR_INSTANT_PASSED"); break;
    case ERR_PAIR_UNIT_KEY_NOT_SUPP: printf("ERR_PAIR_UNIT_KEY_NOT_SUPP"); break;
    case ERR_CONTROLLER_BUSY: printf("ERR_CONTROLLER_BUSY"); break;
    case ERR_DIRECTED_ADV_TIMEOUT: printf("ERR_DIRECTED_ADV_TIMEOUT"); break;
    case ERR_CONN_END_WITH_MIC_FAILURE: printf("ERR_CONN_END_WITH_MIC_FAILURE"); break;
    case ERR_CONN_FAILED_TO_ESTABLISH: printf("ERR_CONN_FAILED_TO_ESTABLISH"); break;
    case BLE_STATUS_FAILED: printf("BLE_STATUS_FAILED"); break;
    case BLE_STATUS_INVALID_PARAMS: printf("BLE_STATUS_INVALID_PARAMS"); break;
    case BLE_STATUS_NOT_ALLOWED: printf("BLE_STATUS_NOT_ALLOWED"); break;
    case BLE_STATUS_ERROR: printf("BLE_STATUS_ERROR"); break;
    case BLE_STATUS_ADDR_NOT_RESOLVED: printf("BLE_STATUS_ADDR_NOT_RESOLVED"); break;
    case FLASH_READ_FAILED: printf("FLASH_READ_FAILED"); break;
    case FLASH_WRITE_FAILED: printf("FLASH_WRITE_FAILED"); break;
    case FLASH_ERASE_FAILED: printf("FLASH_ERASE_FAILED"); break;
    case BLE_STATUS_INVALID_CID: printf("BLE_STATUS_INVALID_CID"); break;
    case TIMER_NOT_VALID_LAYER: printf("TIMER_NOT_VALID_LAYER"); break;
    case TIMER_INSUFFICIENT_RESOURCES: printf("TIMER_INSUFFICIENT_RESOURCES"); break;
    case BLE_STATUS_CSRK_NOT_FOUND: printf("BLE_STATUS_CSRK_NOT_FOUND"); break;
    case BLE_STATUS_IRK_NOT_FOUND: printf("BLE_STATUS_IRK_NOT_FOUND"); break;
    case BLE_STATUS_DEV_NOT_FOUND_IN_DB: printf("BLE_STATUS_DEV_NOT_FOUND_IN_DB"); break;
    case BLE_STATUS_SEC_DB_FULL: printf("BLE_STATUS_SEC_DB_FULL"); break;
    case BLE_STATUS_DEV_NOT_BONDED: printf("BLE_STATUS_DEV_NOT_BONDED"); break;
    case BLE_STATUS_DEV_IN_BLACKLIST: printf("BLE_STATUS_DEV_IN_BLACKLIST"); break;
    case BLE_STATUS_INVALID_HANDLE: printf("BLE_STATUS_INVALID_HANDLE"); break;
    case BLE_STATUS_INVALID_PARAMETER: printf("BLE_STATUS_INVALID_PARAMETER"); break;
    case BLE_STATUS_OUT_OF_HANDLE: printf("BLE_STATUS_OUT_OF_HANDLE"); break;
    case BLE_STATUS_INVALID_OPERATION: printf("BLE_STATUS_INVALID_OPERATION"); break;
    case BLE_STATUS_INSUFFICIENT_RESOURCES: printf("BLE_STATUS_INSUFFICIENT_RESOURCES"); break;
    case BLE_INSUFFICIENT_ENC_KEYSIZE: printf("BLE_INSUFFICIENT_ENC_KEYSIZE"); break;
    case BLE_STATUS_CHARAC_ALREADY_EXISTS: printf("BLE_STATUS_CHARAC_ALREADY_EXISTS"); break;
    case BLE_STATUS_NO_VALID_SLOT: printf("BLE_STATUS_NO_VALID_SLOT"); break;
    case BLE_STATUS_SCAN_WINDOW_SHORT: printf("BLE_STATUS_SCAN_WINDOW_SHORT"); break;
    case BLE_STATUS_NEW_INTERVAL_FAILED: printf("BLE_STATUS_NEW_INTERVAL_FAILED"); break;
    case BLE_STATUS_INTERVAL_TOO_LARGE: printf("BLE_STATUS_INTERVAL_TOO_LARGE"); break;
    case BLE_STATUS_LENGTH_FAILED: printf("BLE_STATUS_LENGTH_FAILED"); break;
    case BLE_STATUS_TIMEOUT: printf("BLE_STATUS_TIMEOUT"); break;
    case BLE_STATUS_PROFILE_ALREADY_INITIALIZED: printf("BLE_STATUS_PROFILE_ALREADY_INITIALIZED"); break;
    case BLE_STATUS_NULL_PARAM: printf("BLE_STATUS_NULL_PARAM"); break;
    default: printf("Unknown Status"); break;
  }
  printf("\r\n");
}

/*=== PUBLIC API =====================*/
void bluetooth_init() {
  
  tBleStatus ret;
  uint16_t service_handle, dev_name_char_handle, appearance_char_handle;

  HCI_Init();
  
  /* Init SPI (SERCOM1) interface */
  BNRG_SPI_Init();
  
  /* Reset BlueNRG/BlueNRG-MS SPI interface */
  BlueNRG_RST();

  /* Edit configuration */
  ret = aci_hal_write_config_data(CONFIG_DATA_PUBADDR_OFFSET, CONFIG_DATA_PUBADDR_LEN, BDADDR);
  while (ret != BLE_STATUS_SUCCESS) print_status(ret, "aci_hal_write_config_data failed: ");

  ret = aci_gatt_init();
  while (ret != BLE_STATUS_SUCCESS) print_status(ret, "aci_gatt_init failed: ");
  
  ret = aci_gap_init_IDB05A1(GAP_PERIPHERAL_ROLE_IDB05A1, 0, 0x07, &service_handle, &dev_name_char_handle, &appearance_char_handle);
  while (ret != BLE_STATUS_SUCCESS) print_status(ret, "aci_gap_init_IDB05A1 failed: ");

  ret = aci_gatt_update_char_value(service_handle, dev_name_char_handle, 0, strlen(BDNAME), (uint8_t *)BDNAME);
  while (ret != BLE_STATUS_SUCCESS) print_status(ret, "aci_gatt_update_char_value failed: ");

  ret = Add_UART_Service();
  while (ret != BLE_STATUS_SUCCESS) print_status(ret, "Error while adding UART service: ");

  // +4 dBm output power
  ret = aci_hal_set_tx_power_level(1, 3);
  while (ret != BLE_STATUS_SUCCESS) print_status(ret, "Error while setting power output: ");

  ret = aci_gap_set_io_capability(IO_CAP_DISPLAY_YES_NO);
  while (ret != BLE_STATUS_SUCCESS) print_status(ret, "Error while setting IO capability: ");

  uint8_t oob_data[16];
  ret = aci_gap_set_auth_requirement(MITM_PROTECTION_REQUIRED, OOB_AUTH_DATA_ABSENT, oob_data, MIN_ENCRY_KEY_SIZE,
                                     MAX_ENCRY_KEY_SIZE, USE_FIXED_PIN_FOR_PAIRING, 123456, BONDING);
  while (ret != BLE_STATUS_SUCCESS) print_status(ret, "Error while setting auth requirement: ");

  bluetooth_disable();
}

void bluetooth_handle() {
  if (do_unconnectable) setNonConnectable();
  if (do_connectable) setConnectable();
  while (!HCI_Queue_Empty()) {
    HCI_Process();
  }
}

uint8_t bluetooth_uart_tx(uint8_t *buf, uint32_t len) {

  uint8_t nextLen;
  tBleStatus ret;
  if (!bluetooth_connected()) return 0;
  
  while(len > 0) {
    
    nextLen = (len > BLE_UART_TX_BUFFSIZE) ? (BLE_UART_TX_BUFFSIZE) : (uint8_t) len;
    ret = aci_gatt_update_char_value(UARTServHandle, UARTRXCharHandle, 0, nextLen, buf);
    
    if (ret != BLE_STATUS_SUCCESS) {
      print_status(ret, "Error while updating UART: ");
      return 0;
    }
    
    len = len - (uint32_t)nextLen;
    buf = (uint8_t*)(buf + nextLen);
    
  }
  return 1;
}

uint8_t bluetooth_connected() {
  return BLEConnHandle != 0;
}

void bluetooth_enable() {
  ble_enabled = 1;
  do_connectable = 1;
  do_unconnectable = 0;
  printf("BLE: Bluetooth enabling...\r\n");
}

void bluetooth_disable() {
  ble_enabled = 0;
  do_unconnectable = 1;
  do_connectable = 0;
  if (bluetooth_connected()) {
    hci_disconnect(BLEConnHandle, HCI_OE_USER_ENDED_CONNECTION);
  }
  printf("BLE: Bluetooth disabling...\r\n");
}
