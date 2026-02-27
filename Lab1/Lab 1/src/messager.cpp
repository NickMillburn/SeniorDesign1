#include "WiFiS3.h"
#include "network_credentials.h"

char ssid[] = PRIVATE_SSID;
char pass[] = PRIVATE_PASSWORD;

const char* apiKey = "REMOVED_SENDGRID_KEY";
const char* host = "api.sendgrid.com";

#define EMAIL_SENDER      "tpurcell@uiowa.edu"
#define EMAIL_SENDER_NAME "Tyler Purcell"

//Constructor
messager::messager(){}

void messager::checkNotify(int sensorIndex, float tempC, const messageConfig& cfg) {
    if (sensorIndex < 0 || sensorIndex > 1) { 
      return; 
    }
    if (!cfg.alertEnabled) { 
      return; 
    }
    if (isnan(tempC)) { 
      return; 
    }

    bool reachedMax = (tempC > cfg.maxThresholdC);
    bool reachedMin    = (tempC < cfg.minThresholdC);

    if (!reachedMax && !reachedMin) return;

    // Build the subject — append direction hint for clarity
    String subject = cfg.subject;
    subject += reachedMax ? " [HIGH]" : " [LOW]";

    // Expand {sensor} / {temp} tokens in the user's body template
    String body = expandTemplate(cfg.bodyTemplate, sensorIndex + 1, tempC);

    // Append a direction line so the email is self-explanatory
    body += "\n\n";
    if (reachedMax) {
        body += "Max threshold : " + String(cfg.maxThresholdC, 1) + " deg C\n";
    } else {
        body += "Min threshold : " + String(cfg.minThresholdC, 1) + " deg C\n";
    }
    body += "Current reading: " + String(tempC, 1) + " deg C";

    Serial.print("[EmailNotifier] Threshold breached on sensor ");
    Serial.print(sensorIndex + 1);
    Serial.print(" (");
    Serial.print(reachedMax ? "HIGH" : "LOW");
    Serial.print("), sending to ");
    Serial.print(cfg.recipient);
    Serial.print(" ... ");

    bool ok = sendEmail(cfg.recipient, subject, body);
    Serial.println(ok ? "sent!" : "FAILED");
}

String messager::changeTemplate(const String& tmpl, int sensorNumber, float tempC) {
    String out = tmpl;
    out.replace("{sensor}", String(sensorNumber));
    out.replace("{temp}",   String(tempC, 1));
    return out;
}

void sendEmail(const String& recipient, const String& subject, const String& body)
{
  WiFiSSLClient client;
  if (client.connect("api.sendgrid.com", 443)) {

    // Escape the body for embedding in JSON
    String escapedBody = body;
    escapedBody.replace("\\", "\\\\");
    escapedBody.replace("\"", "\\\"");
    escapedBody.replace("\n", "\\n");
    escapedBody.replace("\r", "");

    // Escape subject too (user-supplied)
    String escapedSubject = subject;
    escapedSubject.replace("\\", "\\\\");
    escapedSubject.replace("\"", "\\\"");

    String message =
        "{"
          "\"personalizations\":[{"
            "\"to\":[{\"email\":\"" + recipient + "\"}]"
          "}],"
          "\"from\":{"
            "\"email\":\"" EMAIL_SENDER "\","
            "\"name\":\"" EMAIL_SENDER_NAME "\""
          "},"
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

    // Wait up to 5s for a response
    unsigned long timeout = millis();
    while (client.available() == 0) {
        if (millis() - timeout > 5000) {
            Serial.println("Response timed out.");
            client.stop();
            return false;
        }
    }

    String statusLine = client.readStringUntil('\n');
    client.stop();

    Serial.print("Email Messager Response: ");
    Serial.println(statusLine);

    return statusLine.indexOf("202") >= 0;
  }

 else {
    Serial.println("Connection failed.");
  }
}