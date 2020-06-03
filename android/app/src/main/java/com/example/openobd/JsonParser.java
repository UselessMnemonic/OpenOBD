package com.example.openobd;

import android.util.JsonReader;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.IOException;

/* This class parses JSON data for the API */
public class JsonParser {
    public static JSONObject parseObject(JsonReader reader) throws IOException, JSONException {

        JSONObject jobj = new JSONObject();
        reader.beginObject();

        while (reader.hasNext()) {
            String key = reader.nextName();
            switch (reader.peek()) {
                case BEGIN_ARRAY:
                    jobj.put(key, parseArray(reader));
                    break;
                case BEGIN_OBJECT:
                    jobj.put(key, parseObject(reader));
                    break;
                case NUMBER:
                    String number = reader.nextString();
                    if (key.equals("pid"))
                        jobj.put("pid", Integer.valueOf(number));
                    else if (number.contains("."))
                        jobj.put(key, Double.valueOf(number));
                    else
                        jobj.put(key, Long.valueOf(number));

                case BOOLEAN:
                    jobj.put(key, reader.nextBoolean());
                default:
                case STRING:
                    jobj.put(key, reader.nextString());
                    break;
            }
        }

        reader.endObject();
        return jobj;
    }
    public static JSONArray parseArray(JsonReader reader) throws IOException, JSONException {

        JSONArray jary = new JSONArray();
        reader.beginArray();

        while (reader.hasNext()) {
            switch (reader.peek()) {
                case BEGIN_ARRAY:
                    jary.put(parseArray(reader));
                    break;
                case BEGIN_OBJECT:
                    jary.put(parseObject(reader));
                    break;
                case NUMBER:
                    String number = reader.nextString();
                    if (number.contains("."))
                        jary.put(Double.valueOf(number));
                    else
                        jary.put(Long.valueOf(number));
                    break;
                case BOOLEAN:
                    jary.put(reader.nextBoolean());
                    break;
                default:
                case STRING:
                    jary.put(reader.nextString());
                    break;
            }
        }

        reader.endArray();
        return jary;
    }
}
