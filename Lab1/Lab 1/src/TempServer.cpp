#include "TempServer.h"
#include <math.h>

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

// Handles client requests by checking for available clients, reading their requests, and sending appropriate responses
// (HTML page or JSON data) based on the request type.
void TempServer::handleClientRequest() {
    WiFiClient client = server.available();
    // Only process the request if a client is connected
    if (client) {
        String currentLine = "";
        String requestLine = "";

        String headers      = "";
        int    contentLength = 0;

        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                if (c == '\n') {
                    // If the current line is blank, it means we've reached the end of the HTTP request headers
                    if (currentLine.length() == 0) {
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
                        } else if (requestLine.startsWith("POST /toggleSensor1")) {
                            sensor1Active = !sensor1Active; // Toggle the state of sensor 1
                            sendData(client); // Send updated data after toggling
                        } else if (requestLine.startsWith("POST /toggleSensor2")) {
                            sensor2Active = !sensor2Active; // Toggle the state of sensor 2
                            sendData(client); // Send updated data after toggling
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

<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>

<script>
  let currentUnit = 'C';
  let viewMode = 'both';
  let lastSensor0C = [];
  let lastSensor1C = [];
  let sensor1Enabled = false;
  let sensor2Enabled = false;

  function convertTemp(value) {
    if (value === null) return null;
    return currentUnit === 'F' ? (value * 9 / 5) + 32 : value;
  }

  function convertSeries(series) {
    return series.map(convertTemp);
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
          suggestedMin: 0,
          suggestedMax: 50,
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
    myChart.options.scales.y.suggestedMin = currentUnit === 'F' ? 32 : 0;
    myChart.options.scales.y.suggestedMax = currentUnit === 'F' ? 122 : 50;
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
    document.getElementById('sensor1Status').textContent = `Status: ${sensor1Enabled ? 'ON' : 'OFF'}`;
    document.getElementById('sensor2Status').textContent = `Status: ${sensor2Enabled ? 'ON' : 'OFF'}`;
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

    // building the JSON response string
    String jsonResponse = "{";
    bool firstSensor = true;

    for (const auto& entry : sensorData) {
        // comma for separating sensor entries, but only if it's not the first one
        if (!firstSensor) {
            jsonResponse += ",";
        }
        firstSensor = false;

        int sensorId = entry.first;
        const std::vector<float>& readings = entry.second;

        jsonResponse += "\"sensor" + String(sensorId) + "\":[";
        for (size_t i = 0; i < readings.size(); ++i) {
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
