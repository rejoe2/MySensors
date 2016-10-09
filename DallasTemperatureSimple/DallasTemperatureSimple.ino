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
   Enhanced Version also sending the Dallas-ROM-ID
*/

// Enable debug prints to serial monitor
#define MY_DEBUG
// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69
#define MY_NODE_ID 110
#include <SPI.h>
#include <MySensors.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#define COMPARE_TEMP 1 // Send temperature only if changed?
//#define SEND_ID // Send also Dallas-Addresses?
#define ONE_WIRE_BUS 3 // Pin where dallase sensor is connected 
#define MAX_ATTACHED_DS18B20 16

uint8_t DS_First_Child_ID = 7; //First Child-ID to be used by Dallas Bus; set this to be higher than other Child-ID's who need EEPROM storage to avoid conflicts
uint16_t SLEEP_TIME = 30000; // Sleep time between reads (in milliseconds)
OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire); // Pass the oneWire reference to Dallas Temperature.
float lastTemperature[MAX_ATTACHED_DS18B20];
uint8_t numSensors = 0;
DeviceAddress tempDeviceAddress; // We'll use this variable to store a found device address
int  resolution = 12;
int  conversionTime = 0;
// Initialize temperature message
MyMessage msgTemp(0, V_TEMP);
MyMessage msgId(0, V_ID);
boolean receivedConfig = false;
boolean metric = true;
boolean IDsSent = false;


void before() {
  conversionTime = 750 / (1 << (12 - resolution));
  // Startup up the OneWire library
  sensors.begin();
  // requestTemperatures() will not block current thread
  sensors.setWaitForConversion(false);
  // Fetch the number of attached temperature sensors
  numSensors = sensors.getDeviceCount();
}

void presentation() {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Temperature Sensor", "1.4");
  // Present all sensors to controller
  for (int i = 0; i < numSensors && i < MAX_ATTACHED_DS18B20; i++) {
    present(i + 1, S_TEMP, tempDeviceAddress);
  }
}

void setup() {
  for (int i = 0; i < numSensors && i < MAX_ATTACHED_DS18B20; i++) {
    sensors.getAddress(tempDeviceAddress, i);
    //8 sorgt dafür, dass alle 16 Stellen übermittelt werden
    send(msgId.setSensor(i + 1).set(tempDeviceAddress, 8));
    sensors.setResolution(tempDeviceAddress, resolution);
  }
}

void loop() {
  // Fetch temperatures from Dallas sensors
  sensors.requestTemperatures();
  // query conversion time and sleep until conversion completed
  // sleep() call can be replaced by wait() call if node need to process incoming messages (or if node is repeater)
  wait(conversionTime);
  // Read temperatures and send them to controller
  for (int i = 0; i < numSensors && i < MAX_ATTACHED_DS18B20; i++) {
    // Fetch and round temperature to one decimal
    float temperature = static_cast<float>(static_cast<int>((getConfig().isMetric ? sensors.getTempCByIndex(i) : sensors.getTempFByIndex(i)) * 10.)) / 10.;
    // Only send data if temperature has changed and no error
#if COMPARE_TEMP == 1
    if (lastTemperature[i] != temperature && temperature != -127.00 && temperature != 85.00) {
#else
    if (temperature != -127.00 && temperature != 85.00) {
#endif
      // Send in the new temperature
      send(msgTemp.setSensor(i + 1).set(temperature, 1));
      // Save new temperatures for next compare
      lastTemperature[i] = temperature;
    }
  }
  wait(SLEEP_TIME);
}
