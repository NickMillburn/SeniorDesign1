#pragma once

#include <Arduino.h>
#include <WiFiS3.h>
#include <map>
#include <vector>
#include "messageConfig.h"

class TempServer {
  public:
    static const int MAX_READINGS = 300; // Maximum number of readings to store per sensor

    explicit TempServer(int port, bool& sensor1Active, bool& sensor2Active, bool& systemPowerOn)
      : server(port),
        sensor1Active(sensor1Active),
        sensor2Active(sensor2Active),
        systemPowerOn(systemPowerOn),
        lastEmailSentMs(0),
        lastEmailSensor(-1),
        lastEmailTempC(NAN) {
        sensorData[0] = std::vector<float>(MAX_READINGS, NAN);
        sensorData[1] = std::vector<float>(MAX_READINGS, NAN);
    }

    void begin();
    std::vector<float> getSensorData(int sensorId);
    void writeSensorData(int sensorId, float temp);
    void handleClientRequest();
    void sendHTML(WiFiClient& client);
    void sendData(WiFiClient& client);

    const messageConfig& getMessageConfig() const { return alertConfig; }
    void recordEmailSent(int sensorIndex, float temperatureC);

    ~TempServer() = default;

  private:
    bool handleSettingsUpdate(const String& requestBody);

    std::map<int, std::vector<float> > sensorData;
    WiFiServer server;
    bool& sensor1Active;
    bool& sensor2Active;
    bool& systemPowerOn;
    messageConfig alertConfig;

    unsigned long lastEmailSentMs;
    int lastEmailSensor;
    float lastEmailTempC;
};
