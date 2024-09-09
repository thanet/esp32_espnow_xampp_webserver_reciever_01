#include <esp_now.h>
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include <Arduino_JSON.h>
#include <HTTPClient.h> 

// Network configurations
const char* ssid1 = "True Enjoy";
const char* password1 = "enjoy7777777777";
String URL1 = "http://192.168.1.57/espdata_00/upload_01.php";

const char* ssid2 = "ENJMesh";
const char* password2 = "enjoy042611749";
String URL2 = "http://192.168.0.113/EspData/upload_01.php";

// Variables to hold the current configuration
const char* ssid = "";
const char* password = "";
String URL = "";

// Public variables
int readmoduleno = 0;
float temperature = 0; 
float humidity = 0;
int readId = 0;
bool readytoupload = false;

// Prototype function
void UploadData2Xampp();


// Structure to receive data
typedef struct struct_message {
  int read_module_no;
  float temp;
  float hum;
  unsigned int readingId;
} struct_message;

struct_message incomingReadings;

JSONVar board;

AsyncWebServer server(80);
AsyncEventSource events("/events");

// Callback function for received data
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) { 
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  
  board["read_module_no"] = incomingReadings.read_module_no;
  board["temperature"] = incomingReadings.temp;
  board["humidity"] = incomingReadings.hum;
  board["readingId"] = incomingReadings.readingId;
  String jsonString = JSON.stringify(board);
  events.send(jsonString.c_str(), "new_readings", millis());
  
  Serial.printf("Board ID %u: %u bytes\n", incomingReadings.read_module_no, len);
  Serial.printf("Temperature: %4.2f \n", incomingReadings.temp);
  Serial.printf("Humidity: %4.2f \n", incomingReadings.hum);
  Serial.printf("Reading ID: %d \n", incomingReadings.readingId);
  Serial.println();

  readmoduleno = incomingReadings.read_module_no;
  temperature = incomingReadings.temp;
  humidity = incomingReadings.hum;
  readId = incomingReadings.readingId;
  readytoupload = true;
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>ESP-NOW DASHBOARD</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    p {  font-size: 1.2rem;}
    body {  margin: 0;}
    .topnav { overflow: hidden; background-color: #2f4468; color: white; font-size: 1.7rem; }
    .content { padding: 20px; }
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
    .cards { max-width: 700px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); }
    .reading { font-size: 2.8rem; }
    .packet { color: #bebebe; }
    .card.temperature { color: #fd7e14; }
    .card.humidity { color: #1b78e2; }
  </style>
</head>
<body>
  <div class="topnav">
    <h3>ESP-NOW DASHBOARD</h3>
  </div>
  <div class="content">
    <div class="cards">
      <div class="card temperature">
        <h4><i class="fas fa-thermometer-half"></i> BOARD #1 - TEMPERATURE</h4>
        <p><span class="reading"><span id="t1">0.00</span> &deg;C</span></p>
        <p class="packet">Reading ID: <span id="rt1">0</span></p>
      </div>
      <div class="card humidity">
        <h4><i class="fas fa-tint"></i> BOARD #1 - HUMIDITY</h4>
        <p><span class="reading"><span id="h1">0.00</span> &percnt;</span></p>
        <p class="packet">Reading ID: <span id="rh1">0</span></p>
      </div>
      <div class="card temperature">
        <h4><i class="fas fa-thermometer-half"></i> BOARD #2 - TEMPERATURE</h4>
        <p><span class="reading"><span id="t2">0.00</span> &deg;C</span></p>
        <p class="packet">Reading ID: <span id="rt2">0</span></p>
      </div>
      <div class="card humidity">
        <h4><i class="fas fa-tint"></i> BOARD #2 - HUMIDITY</h4>
        <p><span class="reading"><span id="h2">0.00</span> &percnt;</span></p>
        <p class="packet">Reading ID: <span id="rh2">0</span></p>
      </div>
    </div>
  </div>
<script>
if (!!window.EventSource) {
 var source = new EventSource('/events');
 
 source.addEventListener('open', function(e) {
  console.log("Events Connected");
 }, false);
 
 source.addEventListener('error', function(e) {
  if (e.target.readyState != EventSource.OPEN) {
    console.log("Events Disconnected");
  }
 }, false);
 
 source.addEventListener('message', function(e) {
  console.log("message", e.data);
 }, false);
 
 source.addEventListener('new_readings', function(e) {
  console.log("new_readings", e.data);
  var obj = JSON.parse(e.data);
  
  // Update the temperature and humidity for the respective board ID
  document.getElementById("t" + obj.read_module_no).innerHTML = obj.temperature.toFixed(2);
  document.getElementById("h" + obj.read_module_no).innerHTML = obj.humidity.toFixed(2);
  document.getElementById("rt" + obj.read_module_no).innerHTML = obj.readingId;
  document.getElementById("rh" + obj.read_module_no).innerHTML = obj.readingId;
 }, false);
}
</script>
</body>
</html>
)rawliteral";

// Function to select network configuration
void selectNetworkConfiguration() {
  // Scan for available networks
  int n = WiFi.scanNetworks();
  Serial.println("Scan done");

  bool network1Found = false;
  bool network2Found = false;

  // Check if the desired networks are found
  for (int i = 0; i < n; i++) {
    if (WiFi.SSID(i) == ssid1) {
      network1Found = true;
      Serial.println("Network 1 found");
    }
    if (WiFi.SSID(i) == ssid2) {
      network2Found = true;
      Serial.println("Network 2 found");
    }
  }

  // Select network configuration based on availability
  if (network1Found) {
    ssid = ssid1;
    password = password1;
    URL = URL1;
  } else if (network2Found) {
    ssid = ssid2;
    password = password2;
    URL = URL2;
  } else {
    Serial.println("No preferred networks found, using default configuration.");
    ssid = ssid2;  // Default to second network if none of the preferred ones are found
    password = password2;
    URL = URL2;
  }
}

void setup() {
  Serial.begin(115200);

  // Initialize WiFi and select network configuration
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(WIFI_PS_NONE);
  
  selectNetworkConfiguration();
  
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Connected");
  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register for received data callback
  esp_now_register_recv_cb(OnDataRecv);

  // Setup web server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);
  server.begin();
}

void loop() {
  static unsigned long lastEventTime = millis();
  static const unsigned long EVENT_INTERVAL_MS = 5000;
  if ((millis() - lastEventTime) > EVENT_INTERVAL_MS) {
    events.send("ping", NULL, millis());
    lastEventTime = millis();
  }

  if (readytoupload && temperature > 0 && humidity > 0 ){
    UploadData2Xampp();
  } else {
    Serial.println("No data to upload..");
    delay(5000);
  }
}

void UploadData2Xampp() {
  Serial.println("Uploading data to server...");

  String postData = "read_module_no=" + String(readmoduleno) + 
                    "&temperature=" + String(temperature) + 
                    "&humidity=" + String(humidity) + 
                    "&readingId=" + String(readId); 

  HTTPClient http; 
  http.begin(URL);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  int httpCode = http.POST(postData); 
  if (httpCode > 0) {
    String payload = http.getString(); 
    Serial.print("HTTP Code: "); Serial.println(httpCode); 
    Serial.print("Response: "); Serial.println(payload); 
  } else {
    Serial.print("HTTP POST Error: "); Serial.println(httpCode); 
  }
  
  http.end();  // Close connection
  readytoupload = false;  // reset status to prevent double upload
  delay(20000); // Delay between uploads
}
