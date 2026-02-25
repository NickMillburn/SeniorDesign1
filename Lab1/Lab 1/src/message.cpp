#include "WiFiS3.h"

#include "network_credentials.h"

char ssid[] = PRIVATE_SSID;
char pass[] = PRIVATE_PASSWORD;

const char* apiKey = "REMOVED_SENDGRID_KEY";
const char* host = "api.sendgrid.com";

WiFiSSLClient client;

void sendEmail(const char* recipientEmail, const char* subject, const char* message)
{
  if (client.connect("api.sendgrid.com", 443)) {

    String body =
      "{"
      "\"personalizations\":[{\"to\":[{\"email\":\"" + String(recipientEmail) + "\"}]}],"
      "\"from\":{\"email\":\"verified_sender@email.com\"},"
      "\"subject\":\"" + String(subject) + "\","
      "\"content\":[{\"type\":\"text/plain\",\"value\":\"" + String(message) + "\"}]"
      "}";

    client.println("POST /v3/mail/send HTTP/1.1");
    client.println("Host: api.sendgrid.com");
    client.println("Authorization: Bearer " + String(apiKey));
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(body.length());
    client.println("Connection: close");
    client.println();
    client.println(body);

    client.stop();
  }
 else {
    Serial.println("Connection failed.");
  }
}

void setup() {
  Serial.begin(115200);

  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
  sendEmail();
}

void loop() {}