#include "TempServer.h"
#include <ArduinoJson.h>
#include <math.h>

static String jsonEscape(String value) {
    value.replace("\\", "\\\\");
    value.replace("\"", "\\\"");
    return value;
}

void TempServer::recordEmailSent(int sensorIndex, float temperatureC) {
    lastEmailSentMs = millis();
    lastEmailSensor = sensorIndex;
    lastEmailTempC = temperatureC;
}

void TempServer::begin() {
    server.begin();
}

std::vector<float> TempServer::getSensorData(int sensorId) {
    if (sensorData.find(sensorId) != sensorData.end()) {
        return sensorData[sensorId];
    }
    return std::vector<float>();
}

void TempServer::writeSensorData(int sensorId, float temp) {
    if (sensorData.find(sensorId) != sensorData.end()) {
        sensorData[sensorId].erase(sensorData[sensorId].begin());
        sensorData[sensorId].push_back(temp);
    }
}

bool TempServer::handleSettingsUpdate(const String& requestBody) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, requestBody);
    if (err) {
        return false;
    }

    bool updated = false;

    if (doc["emailAddress"].is<const char*>()) {
        alertConfig.recipient = doc["emailAddress"].as<const char*>();
        updated = true;
    }
    if (doc["highTempThreshold"].is<float>() || doc["highTempThreshold"].is<int>()) {
        alertConfig.maxThresholdC = doc["highTempThreshold"].as<float>();
        updated = true;
    }
    if (doc["lowTempThreshold"].is<float>() || doc["lowTempThreshold"].is<int>()) {
        alertConfig.minThresholdC = doc["lowTempThreshold"].as<float>();
        updated = true;
    }

    return updated;
}

void TempServer::handleClientRequest() {
    WiFiClient client = server.available();
    if (!client) return;

    String currentLine = "";
    String requestLine = "";
    int contentLength = 0;
    String requestBody = "";

    while (client.connected()) {
        if (!client.available()) continue;

        char c = client.read();
        if (c == '\n') {
            if (currentLine.length() == 0) {
                if (contentLength > 0) {
                    unsigned long bodyStartMs = millis();
                    while ((int)requestBody.length() < contentLength && (millis() - bodyStartMs) < 1000) {
                        if (client.available()) {
                            requestBody += (char)client.read();
                        }
                    }
                }

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
                } else {
                    sendHTML(client);
                }
                break;
            }

            if (requestLine.length() == 0) {
                requestLine = currentLine;
            }
            if (currentLine.startsWith("Content-Length:")) {
                String value = currentLine.substring(String("Content-Length:").length());
                value.trim();
                contentLength = value.toInt();
            }
            currentLine = "";
        } else if (c != '\r') {
            currentLine += c;
        }
    }

    client.stop();
}

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
<div id="emailStatus">Email: none sent yet</div>
<div></div>

<fieldset>
  <legend>Temperature Warnings:</legend>
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
    if (data.emailSentRecently) {
      const emailSensor = (typeof data.lastEmailSensor === 'number') ? data.lastEmailSensor + 1 : '?';
      const emailTemp = (typeof data.lastEmailTempC === 'number') ? data.lastEmailTempC.toFixed(1) : '--';
      document.getElementById('emailStatus').textContent = `Email: sent recently (S${emailSensor}, ${emailTemp} °C)`;
    } else if (typeof data.lastEmailAgeSec === 'number' && data.lastEmailAgeSec >= 0) {
      document.getElementById('emailStatus').textContent = `Email: last sent ${Math.floor(data.lastEmailAgeSec)}s ago`;
    } else {
      document.getElementById('emailStatus').textContent = 'Email: none sent yet';
    }
    document.getElementById('sensor1On').disabled = !devicePowerOn;
    document.getElementById('sensor1Off').disabled = !devicePowerOn;
    document.getElementById('sensor2On').disabled = !devicePowerOn;
    document.getElementById('sensor2Off').disabled = !devicePowerOn;
    renderChartFromRaw();

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
  }

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

void TempServer::sendData(WiFiClient& client) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:application/json");
    client.println("Cache-Control: no-store, no-cache, must-revalidate, max-age=0");
    client.println("Pragma: no-cache");
    client.println("Expires: 0");
    client.println("Connection: close");
    client.println();

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
    jsonResponse += String(alertConfig.maxThresholdC, 1);
    jsonResponse += ",\"lowTempThreshold\":";
    jsonResponse += String(alertConfig.minThresholdC, 1);
    jsonResponse += ",\"emailAddress\":\"";
    jsonResponse += jsonEscape(alertConfig.recipient);
    jsonResponse += "\"";
    bool hasSentEmail = (lastEmailSentMs > 0);
    unsigned long emailAgeSec = hasSentEmail ? ((millis() - lastEmailSentMs) / 1000UL) : 0;
    bool emailSentRecently = hasSentEmail && (emailAgeSec <= 15UL);
    jsonResponse += ",\"emailSentRecently\":";
    jsonResponse += (emailSentRecently ? "true" : "false");
    jsonResponse += ",\"lastEmailSensor\":";
    jsonResponse += String(lastEmailSensor);
    jsonResponse += ",\"lastEmailTempC\":";
    if (isnan(lastEmailTempC)) {
        jsonResponse += "null";
    } else {
        jsonResponse += String(lastEmailTempC, 1);
    }
    jsonResponse += ",\"lastEmailAgeSec\":";
    jsonResponse += hasSentEmail ? String(emailAgeSec) : String(-1);
    jsonResponse += "}";

    client.println(jsonResponse);
}
