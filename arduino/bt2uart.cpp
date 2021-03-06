#include <vector>
/* TODO: Include ArduinoJSON
#include ""
*/
#include "OBD2UART.h"



static const uint8_t valid_PIDs[128] = {
  0x04, //PID_ENGINE_LOAD 
  0x05, //PID_COOLANT_TEMP 
  0x06, //PID_SHORT_TERM_FUEL_TRIM_1 
  0x07, //PID_LONG_TERM_FUEL_TRIM_1 
  0x08, //PID_SHORT_TERM_FUEL_TRIM_2 
  0x09, //PID_LONG_TERM_FUEL_TRIM_2 
  0x0A, //PID_FUEL_PRESSURE 
  0x0B, //PID_INTAKE_MAP 
  0x0C, //PID_RPM 
  0x0D, //PID_SPEED 
  0x0E, //PID_TIMING_ADVANCE 
  0x0F, //PID_INTAKE_TEMP 
  0x10, //PID_MAF_FLOW 
  0x11, //PID_THROTTLE 
  0x1E, //PID_AUX_INPUT 
  0x1F, //PID_RUNTIME 
  0x21, //PID_DISTANCE_WITH_MIL 
  0x2C, //PID_COMMANDED_EGR 
  0x2D, //PID_EGR_ERROR 
  0x2E, //PID_COMMANDED_EVAPORATIVE_PURGE 
  0x2F, //PID_FUEL_LEVEL 
  0x30, //PID_WARMS_UPS 
  0x31, //PID_DISTANCE 
  0x32, //PID_EVAP_SYS_VAPOR_PRESSURE 
  0x33, //PID_BAROMETRIC 
  0x3C, //PID_CATALYST_TEMP_B1S1 
  0x3D, //PID_CATALYST_TEMP_B2S1 
  0x3E, //PID_CATALYST_TEMP_B1S2 
  0x3F, //PID_CATALYST_TEMP_B2S2 
  0x42, //PID_CONTROL_MODULE_VOLTAGE 
  0x43, //PID_ABSOLUTE_ENGINE_LOAD 
  0x44, //PID_AIR_FUEL_EQUIV_RATIO 
  0x45, //PID_RELATIVE_THROTTLE_POS 
  0x46, //PID_AMBIENT_TEMP 
  0x47, //PID_ABSOLUTE_THROTTLE_POS_B 
  0x48, //PID_ABSOLUTE_THROTTLE_POS_C 
  0x49, //PID_ACC_PEDAL_POS_D 
  0x4A, //PID_ACC_PEDAL_POS_E 
  0x4B, //PID_ACC_PEDAL_POS_F 
  0x4C, //PID_COMMANDED_THROTTLE_ACTUATOR 
  0x4D, //PID_TIME_WITH_MIL 
  0x4E, //PID_TIME_SINCE_CODES_CLEARED 
  0x52, //PID_ETHANOL_FUEL 
  0x59, //PID_FUEL_RAIL_PRESSURE 
  0x5B, //PID_HYBRID_BATTERY_PERCENTAGE 
  0x5C, //PID_ENGINE_OIL_TEMP 
  0x5D, //PID_FUEL_INJECTION_TIMING 
  0x5E, //PID_ENGINE_FUEL_RATE 
  0x61, //PID_ENGINE_TORQUE_DEMANDED 
  0x62, //PID_ENGINE_TORQUE_PERCENTAGE 
  0x63 //PID_ENGINE_REF_TORQUE 
  }

static const char PID_names[128][32] = {
  "PID_ENGINE_LOAD",
  "PID_COOLANT_TEMP",
  "PID_SHORT_TERM_FUEL_TRIM_1",
  "PID_LONG_TERM_FUEL_TRIM_1",
  "PID_SHORT_TERM_FUEL_TRIM_2",
  "PID_LONG_TERM_FUEL_TRIM_2",
  "PID_FUEL_PRESSURE",
  "PID_INTAKE_MAP",
  "PID_RPM",
  "PID_SPEED",
  "PID_TIMING_ADVANCE",
  "PID_INTAKE_TEMP",
  "PID_MAF_FLOW",
  "PID_THROTTLE",
  "PID_AUX_INPUT",
  "PID_RUNTIME",
  "PID_DISTANCE_WITH_MIL",
  "PID_COMMANDED_EGR",
  "PID_EGR_ERROR",
  "PID_COMMANDED_EVAPORATIVE_PURGE",
  "PID_FUEL_LEVEL",
  "PID_WARMS_UPS",
  "PID_DISTANCE",
  "PID_EVAP_SYS_VAPOR_PRESSURE",
  "PID_BAROMETRIC",
  "PID_CATALYST_TEMP_B1S1",
  "PID_CATALYST_TEMP_B2S1",
  "PID_CATALYST_TEMP_B1S2",
  "PID_CATALYST_TEMP_B2S2",
  "PID_CONTROL_MODULE_VOLTAGE",
  "PID_ABSOLUTE_ENGINE_LOAD",
  "PID_AIR_FUEL_EQUIV_RATIO",
  "PID_RELATIVE_THROTTLE_POS",
  "PID_AMBIENT_TEMP",
  "PID_ABSOLUTE_THROTTLE_POS_B",
  "PID_ABSOLUTE_THROTTLE_POS_C",
  "PID_ACC_PEDAL_POS_D",
  "PID_ACC_PEDAL_POS_E",
  "PID_ACC_PEDAL_POS_F",
  "PID_COMMANDED_THROTTLE_ACTUATOR",
  "PID_TIME_WITH_MIL",
  "PID_TIME_SINCE_CODES_CLEARED",
  "PID_ETHANOL_FUEL",
  "PID_FUEL_RAIL_PRESSURE",
  "PID_HYBRID_BATTERY_PERCENTAGE",
  "PID_ENGINE_OIL_TEMP",
  "PID_FUEL_INJECTION_TIMING",
  "PID_ENGINE_FUEL_RATE",
  "PID_ENGINE_TORQUE_DEMANDED",
  "PID_ENGINE_TORQUE_PERCENTAGE",
  "PID_ENGINE_REF_TORQUE"
  }

uint8_t init_complete = 0;

//TODO: Should convert vector uart_requests into an array and return it to pass to the ELM
void process_bluetooth_read_req(uint64_t upper_qword=0x00, uint64_t lower_qword=0x00) {
  if (upper_qword & (1 << 63) || !init_complete) {
    return;  //This is a Write request and not a read request. Something went wrong if this line triggers.
  }
  
  std::vector<uint8_t> uart_requests;


  //Process lower qword
  uint64_t mask = 1;
  for (int i = 0; i < 64; ++i) {
    if (i > 55) break; //Currently only 55 valid PIDs and I haven't null'd the empty parts of the array.
    if (lower_qword & mask)  uart_requests.push_back(valid_PIDs[i]);
    mask << mask << 1;
  }
  //Process upper qword
  /* currently unused; room for expansion
  mask = 1;
  for (int i = 0; i < 64; ++i) {
    if (lower_qword & mask)  uart_requests.push_back(valid_PIDs[i]);
    mask << mask << 1;
  }
  */
}

//TODO: This should either return the serialized object to take in a buffer to put it in
char* generate_json_response(uint16_t pids[], size_t len) {

  const int capacity = JSON_OBJECT_SIZE(len);
  StaticJsonDocument<capacity> doc;

  char output_buffer[4096];

  for (int i = 0; i < len * 2; i+=2) {
      size_t pos = 0;
      //TODO: Assumes it receives a good value; doesn't have a check for an invalid PID.
      for (int j = 0; j < 128; ++j) {
        if (valid_PIDs[j] == pids[i]) {
          pos = j;
          break;
        }
      }
    doc[PID_names[j]] = pids[i+1];
  }

  serializeJson(doc, output_buffer);
  return output_buffer;
}
