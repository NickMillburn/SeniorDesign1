#pragma once

#include <Arduino.h>
#include <WiFiS3.h>
#include <map>
#include <vector>

class TempServer {
  public:
    static const int MAX_READINGS = 300; // Maximum number of readings to store per sensor
    static const unsigned long ALERT_COOLDOWN_MS = 300000UL; // 5 minutes

    // Constructor to initialize the TempServer with a specific port for the WiFiServer, and basic setup for sensor data storage
    explicit TempServer(int port, bool& sensor1Active,  bool& sensor2Active, bool& systemPowerOn)
      : server(port),
        sensor1Active(sensor1Active),
        sensor2Active(sensor2Active),
        systemPowerOn(systemPowerOn),
        emailAddress(""),
        highTempThreshold(100.0f),
        lowTempThreshold(-100.0f),
        lastAlertEmailMs(0) {
        sensorData[0] = std::vector<float>(MAX_READINGS, NAN); // Initialize sensor 0 data vector with MAX_READINGS NAN values
        sensorData[1] = std::vector<float>(MAX_READINGS, NAN); // Initialize sensor 1 data vector with MAX_READINGS NAN values
     }

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

    //send email when temperature exceeds threshold
    void sendEmailAlert(String emailAddress, float temperature);
    void checkTemperatureAlerts(float sensor0Temp, bool sensor0Enabled, float sensor1Temp, bool sensor1Enabled);

    ~TempServer() = default;

  private:
    bool handleSettingsUpdate(const String& requestBody);

    std::map<int, std::vector<float> > sensorData;
    WiFiServer server;
    bool& sensor1Active;
    bool& sensor2Active;
    bool& systemPowerOn;
    String emailAddress; // Store the email address for alerts
    float highTempThreshold; // Store the temperature threshold for alerts
    float lowTempThreshold; // Store the temperature threshold for alerts
    unsigned long lastAlertEmailMs; // Rate limit alert notifications
};
