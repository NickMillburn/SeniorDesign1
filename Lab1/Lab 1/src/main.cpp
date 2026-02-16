// Based on "Simple webserver example from https://docs.arduino.cc/tutorials/uno-r4-wifi/wifi-examples/"

#include <Arduino.h>
#include "WiFiS3.h"
#include "network_credentials.h" // include the header file with wifi credentials
#include "display.h"
#include "sensors.h"
#include <RTC.h>
// #include <OneWire.h>
// #include <DallasTemperature.h>

//getting wifi credientials from separate file for security purposes
char ssid[] = PRIVATE_SSID; // your network SSID (name)
char password[] = PRIVATE_PASSWORD; // your network password (use for WPA, or use as key

int led = LED_BUILTIN;
int status = WL_IDLE_STATUS;
WiFiServer server(80);

//creating OneWire objects for each temp sensor, using D2 and D3 as the data pins:
// OneWire tempsensorPin1(D2);
// OneWire tempsensorPin2(D3);

//creating DallasTemperature objects for each temp sensor, passing in the corresponding OneWire objects:
// DallasTemperature tempsensor1(&tempsensorPin1);
// DallasTemperature tempsensor2(&tempsensorPin2);

// put function declarations here:
// function to print WiFi status to serial monitor, including the IP address of the board, network SSID, and signal strength:
void printWifiStatus();

void setup() {
  Serial.begin(115200); // setting baud
  pinMode(led, OUTPUT);

  //check for WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  } 

  //attempt to connect to WiFi network:

  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, password);

    // wait 10 seconds for connection:
    delay(10000);
  }

  server.begin();
  printWifiStatus();

  // Serial.begin(115200); // setting baud
  // delay(500);

  display_init();
  sensors_init();
  // display_show_default();

  Serial.println("Boot complete.");
}

void loop() {
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    Serial.println("new client");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out to the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print("<p style=\"font-size:7vw;\">Click <a href=\"/H\">here</a> turn the LED on<br></p>");
            client.print("<p style=\"font-size:7vw;\">Click <a href=\"/L\">here</a> turn the LED off<br></p>");
            
            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break; //Still not displaying anything
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /H")) {
          digitalWrite(LED_BUILTIN, HIGH);               // GET /H turns the LED on
        }
        if (currentLine.endsWith("GET /L")) {
          digitalWrite(LED_BUILTIN, LOW);                // GET /L turns the LED off
        }
      }
      
    }

    // read temperature values from each sensor:
    // tempsensor1.requestTemperatures();
    // tempsensor2.requestTemperatures();
    // float temp1 = tempsensor1.getTempCByIndex(0);
    // float temp2 = tempsensor2.getTempCByIndex(0);

    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }

  // Update sensors and display every loop iteration
  sensors_update();
  float temp1 = sensors_getTempC(0);
  float temp2 = sensors_getTempC(1);

  Serial.print("Temp Sensor 1: ");
  Serial.print(temp1);
  Serial.print(" | Temp Sensor 2: ");
  Serial.println(temp2);
  // TODO: replace with actual button reads when wired
  bool btn1 = true;
  bool btn2 = true;
  display_update(btn1, temp1, btn2, temp2);
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

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}