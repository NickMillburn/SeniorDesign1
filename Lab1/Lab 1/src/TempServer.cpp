#include "TempServer.h"
#include <math.h>

// Constructor to initialize the TempServer with a specific port for the WiFiServer, and basic setup for sensor data storage
TempServer::TempServer(int port) : server(port) {
    sensorData[0] = std::vector<float>();
    sensorData[1] = std::vector<float>();
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

// Writes a new temperature reading for a specified sensor ID. It maintains only the latest 300 readings by removing the oldest entry when the limit is exceeded.
void TempServer::writeSensorData(int sensorId, float temp) {
    if (sensorData.find(sensorId) != sensorData.end()) {
        if (sensorData[sensorId].size() >= 300) {
            sensorData[sensorId].erase(sensorData[sensorId].begin());
        }
        sensorData[sensorId].push_back(temp);
    }
}

//Handles client requests by checking for available clients, reading their requests, and sending appropriate responses (HTML page or JSON data) based on the request type.
void TempServer::handleClientRequest() {
    WiFiClient client = server.available();
    if (client) {
        String currentLine = "";
        String requestLine = "";
        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                if (c == '\n') {
                    if (currentLine.length() == 0) {
                        if (requestLine.startsWith("GET /data ")) {
                            sendData(client);
                        } else {
                            sendHTML(client);
                        }
                        break;
                    }
                    if (requestLine.length() == 0) {
                        requestLine = currentLine;
                    }
                    currentLine = "";
                } else if (c != '\r') {
                    currentLine += c;
                }
            }
        }
        client.stop();
    }
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

<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>

<script>
  const ctx = document.getElementById('myChart');
  const myChart = new Chart(ctx, {
    type: 'line',
    data: {
      labels: [0, 1, 2, 3, 4, 5], // sample indices for x-axis
      datasets: [{
        label: 'Sensor 1',
        data: [10, 20, 30, null, 15, 5], // sample data for sensor 1
        spanGaps: false,
        borderWidth: 1,
        borderColor: 'rgb(255, 0, 0)',
        backgroundColor: 'rgba(255, 0, 0, 0.2)'
      },
    {
        label: 'Sensor 2',
        data: [100,90,null,null,80,70], // sample data for sensor 2
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
        },
        //null points aren't filed in
        spanGaps: false,
        //turn off animations
        animation:{
          duration:0
        }
      },
      
    }
  });

  //update chart every second with new data from server
  async function updateChart() {
    const numPoints = 300; // number of data points to display
    const response = await fetch('/data'); // Fetch new data from the server
    const data = await response.json(); // Parse the JSON response

    const sensor0 = data.sensor0 || [];
    const sensor1 = data.sensor1 || [];
  
    // Update the chart with the new data
    // Set x-axis labels to be the indices of the data points (0 to numPoints)
    myChart.data.labels = Array.from({ numPoints}, (_, i) => numPoints - i); 
    myChart.data.datasets[0].data = sensor0; // Sensor on D2
    myChart.data.datasets[1].data = sensor1; // Sensor on D3
    myChart.update(); // Refresh the chart
  }
  updateChart();
  setInterval(updateChart, 1000); // Update the chart every second
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

    String jsonResponse = "{";
    bool firstSensor = true;

    for (const auto& entry : sensorData) {
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

    jsonResponse += "}";
    client.println(jsonResponse);
}
