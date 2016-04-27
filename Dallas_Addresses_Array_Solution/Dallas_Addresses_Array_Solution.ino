/**
   The MySensors Arduino library handles the wireless radio link and protocol
   between your home built sensors/actuators and HA controller of choice.
   The sensors forms a self healing radio network with optional repeaters. Each
   repeater and gateway builds a routing tables in EEPROM which keeps track of the
   network topology allowing messages to be routed to nodes.
   Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
   Copyright (C) 2013-2015 Sensnology AB
   Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
   Documentation: http://www.mysensors.org
   Support Forum: http://forum.mysensors.org
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
 *******************************
   DESCRIPTION
   Example sketch showing how to send in DS1820B OneWire temperature readings back to the controller
   http://www.mysensors.org/build/temp
   Enhanced Version to keep ChildIDs fix based on petewill's http://forum.mysensors.org/topic/2607/video-how-to-monitor-your-refrigerator.
  - Present Dallas-Hardware-ID as comment
*/

// Enable debug prints to serial monitor
#define MY_DEBUG
// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69
#include <SPI.h>
#include <MySensor.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#define COMPARE_TEMP 1 // Send temperature only if changed?
#define ERASE_HASH // Clear EEPROM, if no 1w-device is present?
#define SEND_ID // Send also Dallas-Addresses?
#define ONE_WIRE_BUS 3 // Pin where dallase sensor is connected 
#define MAX_ATTACHED_DS18B20 16

uint8_t DS_First_Child_ID = 7; //First Child-ID to be used by Dallas Bus; set this to be higher than other Child-ID's who need EEPROM storage to avoid conflicts
OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire); // Pass the oneWire reference to Dallas Temperature.
float lastTemperature[MAX_ATTACHED_DS18B20];
uint8_t numSensors = 0;
unsigned long SLEEP_TIME = 30000; // Sleep time between reads (in milliseconds)
boolean metric = true;

// Initialize temperature message
MyMessage DallasMsg(0, V_TEMP);
DeviceAddress dallasAddresses[] = {
  {0x28, 0xFF, 0xF0, 0x5C, 0x54, 0x14, 0x01, 0x48},
  {0x28, 0xFF, 0x7C, 0x3E, 0x54, 0x14, 0x01, 0x35},
  {0x28, 0xFF, 0x36, 0x98, 0x54, 0x14, 0x01, 0xC1},
  {0x28, 0xFF, 0xF5, 0x15, 0x54, 0x14, 0x01, 0xE9}
};

void setup() {
  // Startup up the OneWire library
  sensors.begin();
  // requestTemperatures() will not block current thread
  sensors.setWaitForConversion(false);
}

void presentation()  {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Dallas Temp, fix Array", "1.0");
  // Register all sensors to gw (they will be created as child devices)
  // Fetch the number of attached temperature sensors
  numSensors = sensors.getDeviceCount();
  // Present all sensors to controller
  for (int i = 0; i < numSensors && i < MAX_ATTACHED_DS18B20; i++) {
    present(DS_First_Child_ID + i, S_TEMP);
  }
}

void loop() {
  // Fetch temperatures from Dallas sensors
  sensors.requestTemperatures();
  // query conversion time and sleep until conversion completed
  int16_t conversionTime = sensors.millisToWaitForConversion(sensors.getResolution());
  // sleep() call can be replaced by wait() call if node need to process incoming messages (or if node is repeater)
  sleep(conversionTime);
  // Read temperatures and send them to controller
  for (int i = 0; i < numSensors && i < MAX_ATTACHED_DS18B20; i++) {
    // Fetch and round temperature to one decimal
    float temperature = static_cast<float>(static_cast<int>((getConfig().isMetric ? sensors.getTempC(dallasAddresses[i]) : sensors.getTempF(dallasAddresses[i])) * 10.)) / 10.;
    //float temperature = static_cast<float>(static_cast<int>((getConfig().isMetric?sensors.getTempCByIndex(i):sensors.getTempFByIndex(i)) * 10.)) / 10.;

    // Only send data if temperature has changed and no error
#if COMPARE_TEMP == 1
    if (lastTemperature[i] != temperature && temperature != -127.00 && temperature != 85.00) {
#else
    if (temperature != -127.00 && temperature != 85.00) {
#endif
      // Send in the new temperature
      send(DallasMsg.setSensor(i + DS_First_Child_ID).set(temperature, 1));
      // Save new temperatures for next compare
      lastTemperature[i] = temperature;
    }
  }
  sleep(SLEEP_TIME);
}
