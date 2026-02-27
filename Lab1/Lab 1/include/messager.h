#pragma once

#include <Arduino.h>
#include "messageConfig.h"

class messager {
  public:
    messager();
    bool checkAndNotify(int sensorIndex, float tempC, const messageConfig& cfg);

  private:
    bool sendEmail(const String& recipient, const String& subject, const String& body);
    static String changeTemplate(const String& tmpl, int sensorNumber, float tempC);

    static const unsigned long ALERT_COOLDOWN_MS = 300000UL; // 5 minutes
    unsigned long lastSentMs[2];
};
