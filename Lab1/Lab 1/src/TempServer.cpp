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
        //erase the oldest reading
        sensorData[sensorId].erase(sensorData[sensorId].begin());
        //add the new reading to the highest index in the vector
        sensorData[sensorId].push_back(temp);
    }
}

//Handles client requests by checking for available clients, reading their requests, and sending appropriate responses 
//(HTML page or JSON data) based on the request type.
void TempServer::handleClientRequest() {
    WiFiClient client = server.available();
    // Only process the request if a client is connected
    if (client) {
        String currentLine = "";
        String requestLine = "";

        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                if (c == '\n') {
                    // If the current line is blank, it means we've reached the end of the HTTP request headers
                    if (currentLine.length() == 0) {
                        // Check the request line to determine if the client is requesting the HTML page or the data endpoint
                        if (requestLine.startsWith("GET /data ")) {
                            sendData(client);
                        } else if (requestLine.startsWith("POST /toggleSensor1")) {
                            sensor1Active = !sensor1Active; // Toggle the state of sensor 1
                            sendData(client); // Send updated data after toggling
                        } else if(requestLine.startsWith("POST /toggleSensor2")) {
                            sensor2Active = !sensor2Active; // Toggle the state of sensor 2
                            sendData(client); // Send updated data after toggling
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
<button id="sensor1Toggle">Toggle Sensor 1</button>
<button id="sensor2Toggle">Toggle Sensor 2</button>

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
      },
       //null points aren't filed in
        spanGaps: false,
        //turn off animations
        animation:{
          duration:0
        }
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
    myChart.data.labels = Array.from({ length:numPoints}, (_, i) => numPoints - i); 
    myChart.data.datasets[0].data = sensor0; // Sensor on D2
    myChart.data.datasets[1].data = sensor1; // Sensor on D3
    myChart.update(); // Refresh
    //  the chart
  }

  //Handle client-side button presses to turn sensors on/off
  document.getElementById('sensor1Toggle').onclick = async () => {
    await fetch('/toggleSensor1', { method: 'POST' }); // Send a request to toggle sensor 1
  };
  document.getElementById('sensor2Toggle').onclick = async () => {
    await fetch('/toggleSensor2', { method: 'POST' }); // Send a request to toggle sensor 2
  };
  
  updateChart();
  setInterval(updateChart, 1000); // Update the chart every second
</script>
</body>
</html>
 
)HTML");
}

// Helper function to send a JSON response containing the current sensor readings.
// This function constructs an HTTP response with appropriate headers for JSON content
// and sends a JSON object that includes the latest temperature readings for each sensor. 
void TempServer::sendData(WiFiClient& client) {
    //changes the HTTP response headers to indicate that we're sending JSON data, and also
    // includes cache control headers to prevent caching of the response so client gets most up-to-date data on each request
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:application/json");
    client.println("Cache-Control: no-store, no-cache, must-revalidate, max-age=0");
    client.println("Pragma: no-cache");
    client.println("Expires: 0");
    client.println("Connection: close");
    client.println();

    //building the JSON response string
    String jsonResponse = "{";
    bool firstSensor = true;

    for (const auto& entry : sensorData) {
        //comma for separating sensor entries, but only if it's not the first one
        if (!firstSensor) {
            jsonResponse += ",";
        }
        firstSensor = false;

        int sensorId = entry.first;
        const std::vector<float>& readings = entry.second;

        //
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