#pragma once

#include <Arduino.h>
#include <WiFiS3.h>
#include <map>
#include <vector>

class TempServer {
  public:
    static const int MAX_READINGS = 300; // Maximum number of readings to store per sensor
    explicit TempServer(int port);

    // Start accepting connections after WiFi is connected.
    void begin();

    //writes and stores the latest 300 temperature readings for the specified sensor ID
    std::vector<float> getSensorData(int sensorId);
    void writeSensorData(int sensorId, float temp);

    void handleClientRequest();

    // Helper functions to send HTML and data responses to clients
    // Sends basic HTML page with embedded JavaScript for charting
    void sendHTML(WiFiClient& client);
    // Sends JSON data response with current sensor readings
    void sendData(WiFiClient& client);

    ~TempServer() = default;

  private:
    std::map<int, std::vector<float> > sensorData;
    WiFiServer server;
};
