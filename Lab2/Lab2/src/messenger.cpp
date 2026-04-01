#include "messenger.h"
#include "WiFiS3.h"
#include <math.h>

#define EMAIL_SENDER      "tylerp0417@gmail.com"
#define EMAIL_SENDER_NAME "Tyler Purcell"
#define SENDGRID_API_KEY "SG.XXXXXXXXXX" // LAST TO-DO: Define in seperate file for security

static const char SENDGRID_HOST[] = "api.sendgrid.com";
static const int  SENDGRID_PORT   = 443;

const char* apiKey = SENDGRID_API_KEY;

messenger::messenger() {}

bool messenger::sendMessage(int hour, int minute, int month, int day, int year) {
    String message = formatMessage(hour, minute, month, day, year);

    Serial.print("[messenger] Sending critical safety alert: ");
    Serial.println(message);

    bool ok = sendEmail("Critical Safety Event", message);
    Serial.println(ok ? "[messenger] Sent!" : "[messenger] FAILED");
    return ok;
}

String messenger::formatMessage(int hour, int minute, int month, int day, int year) {
    // AM/PM Converter
    String period = (hour < 12) ? "AM" : "PM";
    int hour12    = hour % 12;
    if (hour12 == 0) hour12 = 12; //preventing midnight & noon from showing as 00

    String timestamp = "Critical Safety Event at " + twoDigits(hour12) + ":" + twoDigits(minute) + " " + period + " on " + twoDigits(month) + "/" + twoDigits(day) + "/" + String(year);

    return timestamp;
}

String messenger::twoDigits(int value) {
    return (value < 10 ? "0" : "") + String(value);
}

bool messenger::sendEmail(const String& subject, const String& body) {
    WiFiSSLClient client;
    if (!client.connect(SENDGRID_HOST, SENDGRID_PORT)) {
        Serial.println("[messenger] Connection to SendGrid failed.");
        return false;
    }

    // Escape body for embedding in JSON
    String escapedBody = body;
    escapedBody.replace("\\", "\\\\");
    escapedBody.replace("\"", "\\\"");
    escapedBody.replace("\n", "\\n");

    String jsonPayload =
        "{"
          "\"personalizations\":[{"
            "\"to\":[{\"email\":\"" EMAIL_RECIPIENT "\"}]"
          "}],"
          "\"from\":{"
            "\"email\":\"" EMAIL_SENDER "\","
            "\"name\":\"" EMAIL_SENDER_NAME "\""
          "},"
          "\"subject\":\"" + subject + "\","
          "\"content\":[{"
            "\"type\":\"text/plain\","
            "\"value\":\"" + escapedBody + "\""
          "}]"
        "}";

    client.println("POST /v3/mail/send HTTP/1.1");
    client.println("Host: api.sendgrid.com");
    client.println("Content-Type: application/json");
    client.print("Authorization: Bearer ");
    client.println(SENDGRID_API_KEY);
    client.print("Content-Length: ");
    client.println(jsonPayload.length());
    client.println("Connection: close");
    client.println();
    client.println(jsonPayload);

    // Wait up to 5 s for a response
    unsigned long timeout = millis();
    while (client.available() == 0) {
        if (millis() - timeout > 5000) {
            Serial.println("[messenger] Response timed out.");
            client.stop();
            return false;
        }
    }

    String statusLine = client.readStringUntil('\n');
    client.stop();

    Serial.print("[messenger] Response: ");
    Serial.println(statusLine);

    return statusLine.indexOf("202") >= 0;
}
