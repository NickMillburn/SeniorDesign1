#include "messanger.h"
#include "WiFiS3.h"
#include <math.h>

constexpr const char* EMAIL_SENDER = "tylerp0417@gmail.com";
constexpr const char* EMAIL_SENDER_NAME = "Tyler Purcell";

#ifndef SENDGRID_API_KEY
#error "SENDGRID_API_KEY is not defined. Add it to .env (see .env.example)."
#endif

const char* apiKey = SENDGRID_API_KEY;

messager::messager() : lastSentMs{0, 0} {}

bool messager::checkAndNotify(int sensorIndex, float tempC, const messageConfig& cfg) {
    if (sensorIndex < 0 || sensorIndex > 1) {
        Serial.println("[Email] Skip: invalid sensor index");
        return false;
    }
    if (!cfg.alertEnabled) {
        Serial.println("[Email] Skip: alerts disabled");
        return false;
    }
    if (isnan(tempC) || tempC <= -100.0f) {
        Serial.println("[Email] Skip: invalid/disconnected reading");
        return false;
    }

    bool reachedMax = (tempC > cfg.maxThresholdC);
    bool reachedMin = (tempC < cfg.minThresholdC);
    if (!reachedMax && !reachedMin) {
        Serial.print("[Email] Skip: inside range. temp=");
        Serial.print(tempC, 1);
        Serial.print(" range=[");
        Serial.print(cfg.minThresholdC, 1);
        Serial.print(", ");
        Serial.print(cfg.maxThresholdC, 1);
        Serial.println("]");
        return false;
    }

    unsigned long now = millis();
    if (now - lastSentMs[sensorIndex] < ALERT_COOLDOWN_MS) {
        Serial.print("[Email] Skip: cooldown active for sensor ");
        Serial.println(sensorIndex + 1);
        return false;
    }

    String subject = cfg.subject;
    subject += reachedMax ? " [HIGH]" : " [LOW]";

    String body = changeTemplate(cfg.bodyTemplate, sensorIndex + 1, tempC);
    body += "\n\n";
    if (reachedMax) {
        body += "Max threshold : " + String(cfg.maxThresholdC, 1) + " deg C\n";
    } else {
        body += "Min threshold : " + String(cfg.minThresholdC, 1) + " deg C\n";
    }
    body += "Current reading: " + String(tempC, 1) + " deg C";

    bool ok = sendEmail(cfg.recipient, subject, body);
    if (ok) {
        lastSentMs[sensorIndex] = now;
        Serial.print("[Email] Sent for sensor ");
        Serial.print(sensorIndex + 1);
        Serial.print(" at ");
        Serial.print(tempC, 1);
        Serial.println("C");
    } else {
        Serial.print("[Email] Send failed for sensor ");
        Serial.println(sensorIndex + 1);
    }
    return ok;
}

String messager::changeTemplate(const String& tmpl, int sensorNumber, float tempC) {
    String out = tmpl;
    out.replace("{sensor}", String(sensorNumber));
    out.replace("{temp}", String(tempC, 1));
    return out;
}

bool messager::sendEmail(const String& recipient, const String& subject, const String& body) {
    WiFiSSLClient client;
    if (!client.connect("api.sendgrid.com", 443)) {
        Serial.println("Email connection failed.");
        return false;
    }
    
    String escapedBody = body;
    escapedBody.replace("\\", "\\\\");
    escapedBody.replace("\"", "\\\"");
    escapedBody.replace("\n", "\\n");
    escapedBody.replace("\r", "");

    String escapedSubject = subject;
    escapedSubject.replace("\\", "\\\\");
    escapedSubject.replace("\"", "\\\"");

    String message =
        "{"
          "\"personalizations\":[{"
            "\"to\":[{\"email\":\"" + recipient + "\"}]"
          "}],"
          "\"from\":{\"email\":\"" + String(EMAIL_SENDER) + "\",\"name\":\"" + String(EMAIL_SENDER_NAME) + "\"},"
          "\"subject\":\"" + escapedSubject + "\","
          "\"content\":[{"
            "\"type\":\"text/plain\","
            "\"value\":\"" + escapedBody + "\""
          "}]"
        "}";

    client.println("POST /v3/mail/send HTTP/1.1");
    client.println("Host: api.sendgrid.com");
    client.println("Authorization: Bearer " + String(apiKey));
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(message.length());
    client.println("Connection: close");
    client.println();
    client.println(message);

    unsigned long timeout = millis();
    while (client.available() == 0) {
        if (millis() - timeout > 5000) {
            client.stop();
            return false;
        }
    }

    String statusLine = client.readStringUntil('\n');
    client.stop();
    Serial.print("[Email] SendGrid status: ");
    Serial.println(statusLine);
    return statusLine.indexOf("202") >= 0;
}
