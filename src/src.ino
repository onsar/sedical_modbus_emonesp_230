/*
 * -------------------------------------------------------------------
 * EmonESP Serial to Emoncms gateway
 * -------------------------------------------------------------------
 * Adaptation of Chris Howells OpenEVSE ESP Wifi
 * by Trystan Lea, Glyn Hudson, OpenEnergyMonitor
 * All adaptation GNU General Public License as below.
 *
 * -------------------------------------------------------------------
 *
 * This file is part of OpenEnergyMonitor.org project.
 * EmonESP is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * EmonESP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with EmonESP; see the file COPYING.  If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "emonesp.h"
#include "config.h"
#include "wifi.h"
#include "web_server.h"
#include "ota.h"
#include "input.h"
#include "emoncms.h"
#include "mqtt.h"
// 485-------------------------------------------------------------------
#include <ModbusMaster.h>

#define MAX485_DE      D1
#define MAX485_RE_NEG  D2

uint32_t t_last_tx=0;

// instantiate ModbusMaster object
ModbusMaster node;

void preTransmission()
{
  digitalWrite(MAX485_RE_NEG, 1);
  digitalWrite(MAX485_DE, 1);
}
void postTransmission()
{
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
}

// 485-------------------------------------------------------------------


// -------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------
void setup() {
  delay(2000);

  Serial.begin(19200);
#ifdef DEBUG_SERIAL1
  Serial1.begin(19200);
#endif

  DEBUG.println();
  DEBUG.print("EmonESP ");
  DEBUG.println(ESP.getChipId());
  DEBUG.println("Firmware: "+ currentfirmware);

  // Read saved settings from the config
  config_load_settings();

  // Initialise the WiFi
  DEBUG.println("wifi_setup");
  wifi_setup();

  // Bring up the web server

  DEBUG.println("web_server_setup");
  web_server_setup();

  // Start the OTA update systems
  // ota_setup();

  DEBUG.println("Server started");

  delay(100);


// 485------
  pinMode(MAX485_RE_NEG, OUTPUT);
  pinMode(MAX485_DE, OUTPUT);
  // Init in receive mode
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);

  // Modbus communication runs at 115200 baud

  // Modbus slave ID 1

  DEBUG.println("node.begin");
  node.begin(1, Serial);
  // Callbacks allow us to configure the RS485 transceiver correctly

  DEBUG.println("preTransmission");
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

// 485------

} // end setup

// 485------
bool state = true;
// 485------

// -------------------------------------------------------------------
// LOOP
// -------------------------------------------------------------------

void loop()
{

  // ota_loop();
  web_server_loop();
  wifi_loop();

  /*
  String input = "";
  boolean gotInput = input_get(input);

  if (wifi_mode == WIFI_MODE_STA || wifi_mode == WIFI_MODE_AP_AND_STA)
  {
    if(emoncms_apikey != 0 && gotInput) {
      emoncms_publish(input);
    }
    if(mqtt_server != 0)
    {
      mqtt_loop();
      if(gotInput) {
        mqtt_publish(input);
      }
    }
  }
*/
// 485------
  uint8_t result;
  uint16_t data[6];

  // Toggle the coil at address 0x0002 (Manual Load Control)
  // result = node.writeSingleCoil(0x0002, state);
  state = !state;

  // Read 16 registers starting at 0x3100)
  result = node.readInputRegisters(0x0000, 8);
  Serial.println("");
  Serial.print("node.ku8MBSuccess = ");
  Serial.println(node.ku8MBSuccess);
  Serial.print("node.ku8MBIllegalDataAddress = ");
  Serial.println(node.ku8MBIllegalDataAddress);

  Serial.println(result);
  if (result == node.ku8MBSuccess)
  {
    Serial.print("0x00: ");
    Serial.println(node.getResponseBuffer(0x00));
    Serial.print("0x01: ");
    Serial.println(node.getResponseBuffer(0x01));

    Serial.print("0x02: ");
    Serial.println(node.getResponseBuffer(0x02));
    Serial.print("0x03: ");
    Serial.println(node.getResponseBuffer(0x03));

    String name_value;
    name_value += "intensidad:";
    name_value += String(node.getResponseBuffer(0x03));
    Serial.println(name_value);


    uint32_t current_time= millis();
    if (wifi_mode == WIFI_MODE_STA || wifi_mode == WIFI_MODE_AP_AND_STA)
      {
        if(emoncms_apikey != 0 && ((current_time - t_last_tx) > 40000))
          {
            t_last_tx = current_time;
            emoncms_publish(name_value);

          }
      }


    Serial.print("0x04: ");
    Serial.println(node.getResponseBuffer(0x04));
    Serial.print("0x05: ");
    Serial.println(node.getResponseBuffer(0x05));

    // Serial.println(node.getResponseBuffer(0x04)/100.0f);
    // Serial.print("Vload: ");
    // Serial.println(node.getResponseBuffer(0xC0)/100.0f);
    //Serial.print("Pload: ");
    //Serial.println((node.getResponseBuffer(0x0D) +
    //                 node.getResponseBuffer(0x0E) << 16)/100.0f);
  }
  else
  {
    Serial.println("Conexion no establecida");
  }

  delay(2000);
  // node.preTransmission(preTransmission);
  // preTransmission();
  // Serial.println("preTransmission");
  // delay(10000);

  // node.postTransmission(postTransmission);
  // postTransmission();
  // Serial.println("postTransmission");
  // delay(10000);

// 485------


} // end loop