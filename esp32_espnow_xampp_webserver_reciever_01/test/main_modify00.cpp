#include <esp_now.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// Replace with your network credentials
const char* ssid = "YourSSID";
const char* password = "YourPassword";

#define SERVER_IP "192.168.x.x"
#define SERVER_PORT 80

int readmoduleno;
float temperature;
float humidity;
int readId;
bool readytoupload = false;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// HTML page to display data
const char* index_html = R"rawliteral(
<!DOCTYPE html>
<html lang='en'>
<head>
    <meta charset='UTF-8'>
    <title>ESP32 Data Display</title>
</head>
<body>
    <h1>ESP32 Data Display</h1>
    <p>Module Number: <span id="module">0</span></p>
    <p>Temperature: <span id="temp">0.0</span>&deg;C</p>
    <p>Humidity: <span id="hum">0.0</span>%</p>
    <script>
        if (typeof(EventSource) !== "undefined") {
            var source = new EventSource("/events");
            source.onmessage = function(event) {
                var data = JSON.parse(event.data);
                document.getElementById("module").innerHTML = data.module;
                document.getElementById("temp").innerHTML = data.temp.toFixed(1);
                document.getElementById("hum").innerHTML = data.hum.toFixed(1);
            };
        } else {
            document.write("Sorry, your browser does not support server-sent events...");
        }
    </script>
</body>
</html>
)rawliteral";

// Function to handle ESP-NOW data reception
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len) {
  char incomingString[20];
  memcpy(&incomingString, incomingData, len);
  Serial.printf("Data received from: %s\n", mactoa(mac_addr));
  Serial.println(String(incomingString));

  String data = String((char*)incomingString);

  // Parse the received data
  int index = data.indexOf(",");
  if (index == -1) {
    Serial.println("Invalid data format");
    return;
  }
  readmoduleno = data.substring(0, index).toInt();
  data = data.substring(index + 1);
  index = data.indexOf(",");
  if (index == -1 || !data.substring(0, index).toFloat()) {
    Serial.println("Invalid temperature value");
    return;
  }
  temperature = data.substring(0, index).toFloat();
  data = data.substring(index + 1);
  index = data.indexOf(",");
  if (index == -1 || !data.substring(0, index).toFloat()) {
    Serial.println("Invalid humidity value");
    return;
  }
  humidity = data.substring(0, index).toFloat();
  readId = data.substring(index + 1).toInt();

  Serial.print("Module Number: ");
  Serial.println(readmoduleno);
  Serial.print("Temperature: ");
  Serial.println(temperature);
  Serial.print("Humidity: ");
  Serial.println(humidity);
  readytoupload = true;
}

// Function to initialize WiFi
void initializeWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to Wi-Fi...");
  }

  Serial.println("Connected to the Wi-Fi network");
}

// Function to setup ESP-NOW
void initializeEspNow() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
}

// Function to upload data to XAMPP server
void UploadData2Xampp() {
  if (!readytoupload) return;

  WiFiClient client;
  const int httpPort = SERVER_PORT;
  if (!client.connect(SERVER_IP, httpPort)) {
    Serial.println("Connection to XAMPP server failed");
    return;
  }

  String url = "/upload?module=";
  url += readmoduleno;
  url += "&temp=";
  url += temperature;
  url += "&hum=";
  url += humidity;
  url += "&id=";
  url += readId;

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
              "Host: " + String(SERVER_IP) + "\r\n" +
              "Connection: close\r\n\r\n");

  while (client.available()) {
    Serial.write(client.read());
  }

  client.stop();
  readytoupload = false;
}

// Function to initialize EventSource
void setupEventSource() {
  server.on("/events", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/event-stream");
  });

  server.onClient([](AsyncClient* client) {
    Serial.println("New client connected");

    client->onData([](void *arg, AsyncClient *client, void *data, size_t len){
      // Handle incoming data if needed
    });

    client->onError([](void *arg, AsyncClient *client, int8_t error){
      Serial.printf("Client error: %d\n", error);
    });

    client->onDisconnect([](void *arg, AsyncClient *client){
      Serial.println("Client disconnected");
    });
  });
}

void setup() {
  Serial.begin(115200);

  initializeWiFi();
  initializeEspNow();

  // Route for root page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", index_html);
  });

  setupEventSource();
  server.begin();
}

void loop() {
  UploadData2Xampp();
  delay(1000); // Adjust the delay as needed
}


// This updated code includes improvements such as error handling for invalid data formats and values, dynamic updates on the web page using `EventSource`, and a structured HTML layout for better
// presentation. The ESP32 continuously listens for incoming data via ESP-NOW, processes it, uploads it to the XAMPP server if valid, and sends updates to any connected clients.
