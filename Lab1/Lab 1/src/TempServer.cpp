#include <WiFiS3.h>
#include "display.h"
#include "sensors.h"
#include <vector>
#include <map>

using namespace std;

//TempServer class to handle all wifi and server related functions, including handling client requests and responses, and managing the server lifecycle
class TempServer {
    public:
    //inintialize the server and set up any necessary data structures
    TempServer(int port) : server(port) {
        sensorData[0] = vector<float>();
        sensorData[1] = vector<float>();
        server.begin();
        Serial.println("Server started");
    }

    //function to retrieve list of temperature readings for a given sensor ID (0 or 1)
    vector<float> getSensorData(int sensorId) {
        if (sensorData.find(sensorId) != sensorData.end()) {
            return sensorData[sensorId];
        } else {
            return vector<float>(); // return empty vector if sensor ID not found
        }
    }   

    //function to write the latest temperature reading for a given sensor ID (0 or 1) to the server's data structure, which can be retrieved by clients when they make requests
    //Also ensures that temperature readings are sequential, and only the latest 300 readings are stored for each sensor
    void writeSensorData(int sensorId, float temp) {
        if (sensorData.find(sensorId) != sensorData.end()) {
            // Ensure that we only store the latest 300 readings for each sensor
            if (sensorData[sensorId].size() >= 300) {
                sensorData[sensorId].erase(sensorData[sensorId].begin()); // remove the oldest reading to maintain a maximum of 300 readings
            }

            sensorData[sensorId].push_back(temp);
        }
    }

    //function to handle incoming client requests, parse the request, and send appropriate responses based on the request type and URL
    void handleClientRequest() {
        WiFiClient client = server.available();   // listen for incoming clients
        if ((client)) {                             // if you get a client,
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

            // close the connection:
            client.stop();
            Serial.println("client disconnected");
        }
    }
    //function to return access client information
    WiFiClient getClient() {
        return server.available();
    }

    //funciton to send HTML outline to client
    void sendHTML(WiFiClient& client) {
        client.println("HTTP/1.1 200 OK");
        client.println("Content-type:text/html");
        client.println("Connection: close");
        client.println();

        // The html response to set the webpage content. 
        client.println(R"HTML(
<!DOCTYPE html>
<html>
<head>
    <title>Temperature Monitor</title>
</head>
<body>
    <h1>Temperature Monitor</h1>
    <p style="font-size:7vw;">Current Temp: </a> <br></p>
    <p style="font-size:7vw;">Click <a href="/L">here</a><br></p>
</body>
</html>)HTML");
    }

    //sends last 300 temperature readings for each sensor to client in a JSON to be parsed and displayed using Chart.js on the client side
    void sendData(WiFiClient& client){
        client.println("HTTP/1.1 200 OK");
        client.println("Content-type:application/json");
        client.println("Connection: close");
        client.println();

        // Construct JSON response with sensor data
        String jsonResponse = "{";
        for (const auto& entry : sensorData) {
            int sensorId = entry.first;
            const vector<float>& readings = entry.second;

            jsonResponse += "\"sensor" + String(sensorId) + "\":[";
            for (size_t i = 0; i < readings.size(); ++i) {
                jsonResponse += String(readings[i], 1); // Convert float to string with 1 decimal place
                if (i < readings.size() - 1) {
                    jsonResponse += ",";
                }
            }
            jsonResponse += "]";
        }
        jsonResponse += "}";

        client.println(jsonResponse);
    }

    // default destructor is fine; server cleaned up by WiFi stack
    ~TempServer() = default;

    private:
    //member variable to store sensor data from each sensor, mapped by sensor ID (0 or 1) to a vector of temperature readings
    std::map<int, std::vector<float> > sensorData;
    WiFiServer server;
}; 
