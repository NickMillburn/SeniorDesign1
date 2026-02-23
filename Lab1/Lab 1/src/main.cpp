// Based on "Simple webserver example from https://docs.arduino.cc/tutorials/uno-r4-wifi/wifi-examples/"

#include <Arduino.h>
#include "WiFiS3.h"
#include "network_credentials.h" // include the header file with wifi credentials
#include "display.h"
#include "sensors.h"
#include "phys_input.h"
#include <RTC.h>
#include <math.h>
#include "TempServer.h"

//getting wifi credientials from separate file for security purposes
char ssid[] = PRIVATE_SSID; // your network SSID (name)
char password[] = PRIVATE_PASSWORD; // your network password (use for WPA, or use as key

int led = LED_BUILTIN;
int status = WL_IDLE_STATUS;


TempServer server(80, sensor1Active, sensor2Active); // Create an instance of the TempServer class to manage WiFi and server functions

// put function declarations here:
// function to print WiFi status to serial monitor, including the IP address of the board, network SSID, and signal strength:
void printWifiStatus();

void setup() {
  Serial.begin(115200); // setting baud
  Serial.println("Serial ready");

  //check for WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }
  
  Serial.println("WiFi module found");


  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  } 
  //attempt to connect to WiFi network:

  while (status != WL_CONNECTED ) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, password);

    // wait 5 seconds for connection:
    delay(5000);
  }


  Serial.println("Connected to wifi");
  server.begin();
  Serial.println("TempServer started");

  printWifiStatus();

  display_init();
  sensors_init();
  phys_input_init();

  Serial.println("Boot complete.");
}

void loop() {
  // Delegate all HTTP request/response handling to TempServer.
  server.handleClientRequest();

  //Physical inputs (buttons + power switch)
  phys_input_update();

  // If system is powered off, turn off display, write NAN to sensor data
  if (!systemPowerOn) {
    display_off();
    server.writeSensorData(0, NAN);
    server.writeSensorData(1, NAN);
    Serial.println("systemPowerOn=OFF");
    return;
  }
  
  sensors_update();
  float temp1 = sensors_getTempC(0);
  float temp2 = sensors_getTempC(1);

  //Serial output for debugging
  Serial.print(" sensor1Active=");
  Serial.print(sensor1Active ? "ON" : "OFF");
  Serial.print(" sensor2Active=");
  Serial.print(sensor2Active ? "ON" : "OFF");
  Serial.print(" | Temp1=");
  Serial.print(temp1);
  Serial.print("C Temp2=");
  Serial.print(temp2);
  Serial.println("C");

  //write the latest sensor readings to the TempServer if it is on, otherwiseNAN
  if(sensor1Active) {
    server.writeSensorData(0, temp1);
  } else {
    server.writeSensorData(0, NAN);
  }

  if(sensor2Active) {
    server.writeSensorData(1, temp2);
  } else {
    server.writeSensorData(1, NAN);
  }

  //update OLED
  display_update(sensor1Active, temp1, sensor2Active, temp2);
}

// put function definitions here:
void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}

