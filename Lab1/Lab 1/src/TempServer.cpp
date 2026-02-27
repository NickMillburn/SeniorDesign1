#include "TempServer.h"
#include <ArduinoJson.h>
#include <math.h>

static bool isDisconnectedReading(float tempC) {
    return !isnan(tempC) && tempC <= -100.0f;
}

static String jsonEscape(String value) {
    value.replace("\\", "\\\\");
    value.replace("\"", "\\\"");
    return value;
}

// Starts the server; I was getting some weird client connection issues when I had the server start in the TempServer constructor,
// so I moved it to a separate begin() function that is called after WiFi connection is established in main.cpp
void TempServer::begin() {
    server.begin();
}

// Retrieves the latest temperature readings for a given sensor ID. If the sensor ID does not exist, it returns an empty vector.
std::vector<float> TempServer::getSensorData(int sensorId) {
    if (sensorData.find(sensorId) != sensorData.end()) {
        return sensorData[sensorId];
    }
    return std::vector<float>();
}

// Writes a new temperature reading for a specified sensor ID. It maintains only the latest (MAX_READINGS)
// readings by removing the oldest entry when the limit is exceeded.
void TempServer::writeSensorData(int sensorId, float temp) {
    if (sensorData.find(sensorId) != sensorData.end()) {
        // erase the oldest reading
        sensorData[sensorId].erase(sensorData[sensorId].begin());
        // add the new reading to the highest index in the vector
        sensorData[sensorId].push_back(temp);
    }
}

bool TempServer::handleSettingsUpdate(const String& requestBody) {
    // Parses /settings JSON and applies only provided fields.
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, requestBody);
    if (err) {
        return false;
    }

    bool updated = false;

    if (doc["emailAddress"].is<const char*>()) {
        emailAddress = doc["emailAddress"].as<const char*>();
        updated = true;
    }
    if (doc["highTempThreshold"].is<float>() || doc["highTempThreshold"].is<int>()) {
        highTempThreshold = doc["highTempThreshold"].as<float>();
        updated = true;
    }
    if (doc["lowTempThreshold"].is<float>() || doc["lowTempThreshold"].is<int>()) {
        lowTempThreshold = doc["lowTempThreshold"].as<float>();
        updated = true;
    }

    return updated;
}

// Handles client requests by checking for available clients, reading their requests, and sending appropriate responses
// (HTML page or JSON data) based on the request type.
void TempServer::handleClientRequest() {
    WiFiClient client = server.available();
    // Only process the request if a client is connected
    if (client) {
        String currentLine = "";
        String requestLine = "";
        int contentLength = 0;
        String requestBody = "";

        String headers      = "";
        int    contentLength = 0;

        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                if (c == '\n') {
                    // If the current line is blank, it means we've reached the end of the HTTP request headers
                    if (currentLine.length() == 0) {
                        if (contentLength > 0) {
                            unsigned long bodyStartMs = millis();
                            while ((int)requestBody.length() < contentLength && (millis() - bodyStartMs) < 1000) {
                                if (client.available()) {
                                    requestBody += (char)client.read();
                                }
                            }
                        }

                        // Check the request line to determine if the client is requesting the HTML page or the data endpoint
                        if (requestLine.startsWith("GET /data ")) {
                            sendData(client);
                        } else if (requestLine.startsWith("POST /sensor1/on")) {
                            sensor1Active = true;
                            sendData(client);
                        } else if (requestLine.startsWith("POST /sensor1/off")) {
                            sensor1Active = false;
                            sendData(client);
                        } else if (requestLine.startsWith("POST /sensor2/on")) {
                            sensor2Active = true;
                            sendData(client);
                        } else if (requestLine.startsWith("POST /sensor2/off")) {
                            sensor2Active = false;
                            sendData(client);
                        } else if (requestLine.startsWith("POST /settings")) {
                            if (!handleSettingsUpdate(requestBody)) {
                                Serial.println("Warning: invalid /settings payload");
                            }
                            sendData(client);
                        } else if (requestLine.startsWith("POST /messageConfig")) {
                            handleMessageConfig(client, body);
                          }
                        else {
                            sendHTML(client);
                        }
                        break;
                    }
                    // Store the first line of the request (the request line) for later processing
                    if (requestLine.length() == 0) {
                        requestLine = currentLine;
                    }
                    if (currentLine.startsWith("Content-Length:")) {
                        String value = currentLine.substring(String("Content-Length:").length());
                        value.trim();
                        contentLength = value.toInt();
                    }
                    currentLine = "";
                    // If the line is blank, we have reached the end of the request headers, so we can break out of the loop
                } else if (c != '\r') {
                    currentLine += c;
                }
            }
        }
        client.stop();
    }
}

// Parses the form POST body and saves to messageConfig
void TempServer::handleMessageConfig(WiFiClient& client, const String& body) {
    String recipient    = formValue(body, "recipient");
    String subject     = formValue(body, "subject");
    String bodyTemplate = formValue(body, "bodyTemplate");
    String maxStr       = formValue(body, "maxThreshold");
    String minStr       = formValue(body, "minThreshold");
    String enabledStr   = formValue(body, "alertEnabled");

    if (recipient.length()    > 0) alertConfig.recipient     = recipient;
    if (subject.length()      > 0) alertConfig.subject       = subject;
    if (bodyTemplate.length() > 0) alertConfig.bodyTemplate  = bodyTemplate;
    if (maxStr.length()       > 0) alertConfig.maxThresholdC = maxStr.toFloat();
    if (minStr.length()       > 0) alertConfig.minThresholdC = minStr.toFloat();
    alertConfig.alertEnabled = (enabledStr == "true" || enabledStr == "1" || enabledStr == "on");

    Serial.println("Message config updated:");
    Serial.print("  recipient=");    Serial.println(alertConfig.recipient);
    Serial.print("  subject=");      Serial.println(alertConfig.subject);
    Serial.print("  maxThreshold="); Serial.println(alertConfig.maxThresholdC);
    Serial.print("  minThreshold="); Serial.println(alertConfig.minThresholdC);
    Serial.print("  enabled=");      Serial.println(alertConfig.alertEnabled);

    // Respond with updated JSON so the UI can confirm the saved values
    sendData(client);
  
}

// Decoding %XX and '+' in a URL-encoded string
String TempServer::urlDecode(const String& src) {
    String decoded = "";
    for (int i = 0; i < (int)src.length(); ++i) {
        if (src[i] == '+') {
            decoded += ' ';
        } else if (src[i] == '%' && i + 2 < (int)src.length()) {
            char hi = src[i + 1];
            char lo = src[i + 2];
            auto hexVal = [](char c) -> int {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                return 0;
            };
            decoded += (char)((hexVal(hi) << 4) | hexVal(lo));
            i += 2;
        } else {
            decoded += src[i];
        }
    }
    return decoded;
}


// Helper function to send an HTML page with embedded JavaScript for charting the temperature data.
// This function constructs a basic HTTP response and serves a simple webpage that uses Chart.js to
// visualize the temperature readings from the sensors. The html is stored in client.html, and copied here
void TempServer::sendHTML(WiFiClient& client) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println("Connection: close");
    client.println();
    client.println(R"HTML(
<!DOCTYPE html>
<html>
<head>
    <title>Temperature Monitor</title>
</head>
<body>
    <h1>Temperature Monitor</h1>
<div>
  <canvas id="myChart"></canvas>
</div>
<div>
  <strong>Sensor 1:</strong>
  <button id="sensor1On">On</button>
  <button id="sensor1Off">Off</button>
  <span id="sensor1Status">Status: --</span>
</div>
<div>
  <strong>Sensor 2:</strong>
  <button id="sensor2On">On</button>
  <button id="sensor2Off">Off</button>
  <span id="sensor2Status">Status: --</span>
</div>
<button id="viewBoth">View Both</button>
<button id="viewSensor1Only">View Sensor 1 Only</button>
<button id="viewSensor2Only">View Sensor 2 Only</button>
<fieldset>
  <legend>Temperature Unit</legend>
  <label><input type="radio" name="tempUnit" value="C" checked> Celsius</label>
  <label><input type="radio" name="tempUnit" value="F"> Fahrenheit</label>
</fieldset>
<div id="powerStatus">Power: --</div>
<fieldset>
  <legend>Temperature Warnings</legend>
  <p>Email Address:</p>
  <input type="email" id="emailInput" placeholder="Enter email address">
  <p>High Temperature Threshold (C):</p>
  <input type="number" inputmode="decimal" id="highTempThreshold" placeholder="Enter high temp threshold">
  <p>Low Temperature Threshold (C):</p>
  <input type="number" inputmode="decimal" id="lowTempThreshold" placeholder="Enter low temp threshold">
  <button id="saveSettings">Save Settings</button>
</fieldset>

<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>

<script>
  let currentUnit = 'C';
  let viewMode = 'both';
  let lastSensor0C = [];
  let lastSensor1C = [];
  let sensor1Enabled = false;
  let sensor2Enabled = false;
  let devicePowerOn = true;
  let highTempThreshold = 100;
  let lowTempThreshold = -100;
  let emailAddress = '';
  let settingsInitialized = false;

  function convertTemp(value) {
    if (value === null) return null;
    return currentUnit === 'F' ? (value * 9 / 5) + 32 : value;
  }

  function isDisconnectedValue(value) {
    return value !== null && value <= -100;
  }

  function normalizeReading(value) {
    if (value === null || isDisconnectedValue(value)) return null;
    return value;
  }

  function convertSeries(series) {
    return series.map((value) => convertTemp(normalizeReading(value)));
  }

  function latestReading(series) {
    if (!series || series.length === 0) return null;
    return series[series.length - 1];
  }

  const ctx = document.getElementById('myChart');
  const myChart = new Chart(ctx, {
    type: 'line',
    data: {
      labels: [0, 1, 2, 3, 4, 5],
      datasets: [{
        label: 'Sensor 1',
        data: [10, 20, 30, null, 15, 5],
        spanGaps: false,
        borderWidth: 1,
        borderColor: 'rgb(255, 0, 0)',
        backgroundColor: 'rgba(255, 0, 0, 0.2)'
      },
      {
        label: 'Sensor 2',
        data: [100, 90, null, null, 80, 70],
        spanGaps: false,
        borderWidth: 1,
        borderColor: 'rgb(0, 0, 255)',
        backgroundColor: 'rgba(0, 0, 255, 0.2)'
      }]
    },
    options: {
      scales: {
        y: {
          min: 10,
          max: 50,
          title: {
            display: true,
            text: 'Temperature (°C)'
          }
        },
        x: {
          title: {
            display: true,
            text: 'Time (s)'
          }
        }
      },
      spanGaps: false,
      animation: {
        duration: 0
      }
    }
  });

  function renderChartFromRaw() {
    const numPoints = 300;
    myChart.data.labels = Array.from({ length: numPoints }, (_, i) => numPoints - i);
    myChart.data.datasets[0].data = convertSeries(lastSensor0C);
    myChart.data.datasets[1].data = convertSeries(lastSensor1C);
    myChart.options.scales.y.title.text = currentUnit === 'F' ? 'Temperature (°F)' : 'Temperature (°C)';
    myChart.options.scales.y.min = currentUnit === 'F' ? 50 : 10;
    myChart.options.scales.y.max = currentUnit === 'F' ? 122 : 50;
    myChart.data.datasets[0].hidden = (viewMode === 'sensor2');
    myChart.data.datasets[1].hidden = (viewMode === 'sensor1');
    myChart.update();
  }

  async function updateChart() {
    const response = await fetch('/data');
    const data = await response.json();
    lastSensor0C = data.sensor0 || [];
    lastSensor1C = data.sensor1 || [];
    sensor1Enabled = !!data.sensor1Active;
    sensor2Enabled = !!data.sensor2Active;
    devicePowerOn = data.systemPowerOn !== false;
    const sensor1Unplugged = sensor1Enabled && isDisconnectedValue(latestReading(lastSensor0C));
    const sensor2Unplugged = sensor2Enabled && isDisconnectedValue(latestReading(lastSensor1C));
    const sensor1Status = !devicePowerOn ? 'POWER OFF' : (sensor1Unplugged ? 'UNPLUGGED' : (sensor1Enabled ? 'ON' : 'OFF'));
    const sensor2Status = !devicePowerOn ? 'POWER OFF' : (sensor2Unplugged ? 'UNPLUGGED' : (sensor2Enabled ? 'ON' : 'OFF'));
    document.getElementById('sensor1Status').textContent = `Status: ${sensor1Status}`;
    document.getElementById('sensor2Status').textContent = `Status: ${sensor2Status}`;
    document.getElementById('powerStatus').textContent = `Power: ${devicePowerOn ? 'ON' : 'OFF'}`;
    document.getElementById('sensor1On').disabled = !devicePowerOn;
    document.getElementById('sensor1Off').disabled = !devicePowerOn;
    document.getElementById('sensor2On').disabled = !devicePowerOn;
    document.getElementById('sensor2Off').disabled = !devicePowerOn;
    if (!settingsInitialized) {
      if (typeof data.emailAddress === 'string') {
        emailAddress = data.emailAddress;
        document.getElementById('emailInput').value = emailAddress;
      }
      if (typeof data.highTempThreshold === 'number') {
        highTempThreshold = data.highTempThreshold;
        document.getElementById('highTempThreshold').value = String(highTempThreshold);
      }
      if (typeof data.lowTempThreshold === 'number') {
        lowTempThreshold = data.lowTempThreshold;
        document.getElementById('lowTempThreshold').value = String(lowTempThreshold);
      }
      settingsInitialized = true;
    }
    renderChartFromRaw();
  }

  
  <fieldset>
    <legend>Email Message Settings</legend>

    <div class="row">
      <label for="cfgEnabled">Messages enabled</label>
      <input type="checkbox" id="cfgEnabled" checked>
    </div>

    <div class="row">
      <label for="cfgRecipient">Recipient email</label>
      <input type="email" id="cfgRecipient" size="32" placeholder="tpurcell@uiowa.edu">
    </div>

    <div class="row">
      <label for="cfgSubject">Email subject</label>
      <input type="text" id="cfgSubject" size="40" placeholder="Temperature Sensor Notification">
    </div>

    <div class="row" style="align-items:flex-start">
      <label for="cfgBody" style="margin-top:4px">Email body</label>
      <div>
        <textarea id="cfgBody" rows="3" cols="45" placeholder="Sensor {sensor} reached {temp} deg C."></textarea>
        <div class="hint">Use <code>{sensor}</code> for sensor number, <code>{temp}</code> for the temperature value.</div>
      </div>
    </div>

    <div class="row">
      <label for="cfgMax">Max threshold (°C)</label>
      <input type="number" id="cfgMax" size="7" step="0.5" placeholder="50">
      <span class="hint">Message when temp rises above this</span>
    </div>

    <div class="row">
      <label for="cfgMin">Min threshold (°C)</label>
      <input type="number" id="cfgMin" size="7" step="0.5" placeholder="10">
      <span class="hint">Message when temp falls below this</span>
    </div>

    <div class="row">
      <button id="messageSaveBtn">Save Settings</button>
      <span id="messageSaveStatus"></span>
    </div>
  </fieldset>

  // Populate message settings form from JSON response
    function messageAlertConfig(cfg) {
      if (!cfg) return;
      document.getElementById('cfgEnabled').checked     = !!cfg.alertEnabled;
      document.getElementById('cfgRecipient').value     = cfg.recipient    || '';
      document.getElementById('cfgSubject').value       = cfg.subject      || '';
      document.getElementById('cfgBody').value          = cfg.bodyTemplate || '';
      document.getElementById('cfgMax').value           = cfg.maxThresholdC != null ? cfg.maxThresholdC : '';
      document.getElementById('cfgMin').value           = cfg.minThresholdC != null ? cfg.minThresholdC : '';
    }

    // Save message settings 
    document.getElementById('messageSaveBtn').onclick = async () => {
      const params = new URLSearchParams({
        recipient:    document.getElementById('cfgRecipient').value,
        subject:      document.getElementById('cfgSubject').value,
        bodyTemplate: document.getElementById('cfgBody').value,
        maxThreshold: document.getElementById('cfgMax').value,
        minThreshold: document.getElementById('cfgMin').value,
        alertEnabled: document.getElementById('cfgEnabled').checked ? 'on' : 'off'
      });
      const statusEl = document.getElementById('messageSaveStatus');
      statusEl.textContent = 'Saving…';
      statusEl.style.color = '#888';
      try {
        const res  = await fetch('/messageConfig', { method: 'POST', body: params.toString(),
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' } });
        const data = await res.json();
        applyAlertConfig(data.messageConfig);
        statusEl.textContent = '✓ Saved';
        statusEl.style.color = 'green';
      } catch(e) {
        statusEl.textContent = '✗ Error saving';
        statusEl.style.color = 'red';
      }
      setTimeout(() => { statusEl.textContent = ''; }, 3000);
    };

  document.getElementById('sensor1On').onclick = async () => {
    await fetch('/sensor1/on', { method: 'POST' });
    updateChart();
  };

  document.getElementById('sensor1Off').onclick = async () => {
    await fetch('/sensor1/off', { method: 'POST' });
    updateChart();
  };

  document.getElementById('sensor2On').onclick = async () => {
    await fetch('/sensor2/on', { method: 'POST' });
    updateChart();
  };

  document.getElementById('sensor2Off').onclick = async () => {
    await fetch('/sensor2/off', { method: 'POST' });
    updateChart();
  };

  document.getElementsByName('tempUnit').forEach((radio) => {
    radio.addEventListener('change', () => {
      if (!radio.checked) return;
      currentUnit = radio.value;
      renderChartFromRaw();
    });
  });

  document.getElementById('viewBoth').onclick = () => {
    viewMode = 'both';
    renderChartFromRaw();
  };

  document.getElementById('viewSensor1Only').onclick = () => {
    viewMode = 'sensor1';
    renderChartFromRaw();
  };

  document.getElementById('viewSensor2Only').onclick = () => {
    viewMode = 'sensor2';
    renderChartFromRaw();
  };

  document.getElementById('saveSettings').onclick = async () => {
    const email = document.getElementById('emailInput').value.trim();
    const high = parseFloat(document.getElementById('highTempThreshold').value);
    const low = parseFloat(document.getElementById('lowTempThreshold').value);

    if (!Number.isFinite(high) || !Number.isFinite(low)) {
      alert('Please enter valid numeric thresholds.');
      return;
    }
    if (low >= high) {
      alert('Low threshold must be less than high threshold.');
      return;
    }

    const response = await fetch('/settings', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        emailAddress: email,
        highTempThreshold: high,
        lowTempThreshold: low
      })
    });

    if (!response.ok) {
      alert('Failed to save settings.');
      return;
    }

    const updated = await response.json();
    emailAddress = updated.emailAddress || email;
    highTempThreshold = Number.isFinite(updated.highTempThreshold) ? updated.highTempThreshold : high;
    lowTempThreshold = Number.isFinite(updated.lowTempThreshold) ? updated.lowTempThreshold : low;
    document.getElementById('emailInput').value = emailAddress;
    document.getElementById('highTempThreshold').value = String(highTempThreshold);
    document.getElementById('lowTempThreshold').value = String(lowTempThreshold);
    alert('Settings saved.');
  };

  updateChart();
  setInterval(updateChart, 1000);
</script>
</body>
</html>
)HTML");
}

// Helper function to send a JSON response containing the current sensor readings.
// This function constructs an HTTP response with appropriate headers for JSON content
// and sends a JSON object that includes the latest temperature readings for each sensor.
void TempServer::sendData(WiFiClient& client) {
    // changes the HTTP response headers to indicate that we're sending JSON data, and also
    // includes cache control headers to prevent caching of the response so client gets most up-to-date data on each request
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:application/json");
    client.println("Cache-Control: no-store, no-cache, must-revalidate, max-age=0");
    client.println("Pragma: no-cache");
    client.println("Expires: 0");
    client.println("Connection: close");
    client.println();

    // Manual serialization is faster/leaner for large, fixed-size numeric arrays.
    String jsonResponse;
    jsonResponse.reserve(7000);
    jsonResponse = "{";
    bool firstSensor = true;

    for (const auto& entry : sensorData) {
        if (!firstSensor) {
            jsonResponse += ",";
        }
        firstSensor = false;

        const int sensorId = entry.first;
        const std::vector<float>& readings = entry.second;
        jsonResponse += "\"sensor" + String(sensorId) + "\":[";

        for (int i = 0; i < readings.size(); ++i) {
            if (isnan(readings[i])) {
                jsonResponse += "null";
            } else {
                jsonResponse += String(readings[i], 1);
            }
            if (i < readings.size() - 1) {
                jsonResponse += ",";
            }
        }
        jsonResponse += "]";
    }

    jsonResponse += ",\"sensor1Active\":";
    jsonResponse += (sensor1Active ? "true" : "false");
    jsonResponse += ",\"sensor2Active\":";
    jsonResponse += (sensor2Active ? "true" : "false");
    jsonResponse += ",\"systemPowerOn\":";
    jsonResponse += (systemPowerOn ? "true" : "false");
    jsonResponse += ",\"highTempThreshold\":";
    jsonResponse += String(highTempThreshold, 1);
    jsonResponse += ",\"lowTempThreshold\":";
    jsonResponse += String(lowTempThreshold, 1);
    jsonResponse += ",\"emailAddress\":\"";
    jsonResponse += jsonEscape(emailAddress);
    jsonResponse += "\"";
    auto esc = [](const String& s) -> String {
        String out;
        for (int i = 0; i < (int)s.length(); ++i) {
            if (s[i] == '"')       out += "\\\"";
            else if (s[i] == '\\') out += "\\\\";
            else                   out += s[i];
        }
        return out;
    };

    json += ",\"messageConfig\":{";
    json += "\"alertEnabled\":"   + String(messageConfig.alertEnabled ? "true" : "false");
    json += ",\"recipient\":\""   + esc(messageConfig.recipient)     + "\"";
    json += ",\"subject\":\""     + esc(messageConfig.subject)       + "\"";
    json += ",\"bodyTemplate\":\"" + esc(messageConfig.bodyTemplate)  + "\"";
    json += ",\"maxThresholdC\":" + String(messageConfig.maxThresholdC, 2);
    json += ",\"minThresholdC\":" + String(messageConfig.minThresholdC, 2);
    json += "}";

    jsonResponse += "}";

    client.println(jsonResponse);
}


// Helper function to send an email alert when temperature exceeds a certain threshold.
// This is a placeholder function and would need to be implemented with actual email sending logic using an email service or SMTP protocol.
void TempServer::sendEmailAlert(String emailAddress, float temperature) {
    // Placeholder for email alert functionality
    // In a real implementation, this function would use an email sending service or SMTP protocol to send an email alert
    Serial.print("ALERT: Temperature threshold exceeded! Sending email to ");
    Serial.print(emailAddress);
    Serial.print(" with temperature: ");
    Serial.println(temperature);
}

void TempServer::checkTemperatureAlerts(float sensor0Temp, bool sensor0Enabled, float sensor1Temp, bool sensor1Enabled) {
    if (!systemPowerOn || emailAddress.length() == 0) {
        return;
    }

    const unsigned long now = millis();
    if (now - lastAlertEmailMs < ALERT_COOLDOWN_MS) {
        return;
    }

    bool outOfRange = false;
    float triggeringTemp = NAN;

    if (sensor0Enabled && !isnan(sensor0Temp) && !isDisconnectedReading(sensor0Temp)) {
        if (sensor0Temp > highTempThreshold || sensor0Temp < lowTempThreshold) {
            outOfRange = true;
            triggeringTemp = sensor0Temp;
        }
    }

    if (!outOfRange && sensor1Enabled && !isnan(sensor1Temp) && !isDisconnectedReading(sensor1Temp)) {
        if (sensor1Temp > highTempThreshold || sensor1Temp < lowTempThreshold) {
            outOfRange = true;
            triggeringTemp = sensor1Temp;
        }
    }

    if (!outOfRange) {
        return;
    }

    sendEmailAlert(emailAddress, triggeringTemp);
    lastAlertEmailMs = now;
}
