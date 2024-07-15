
#include <Arduino.h>
#include <NetworkClient.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <shelly_ip.h>
//#include <WiFiManager.h>
#include "TFT_eSPI.h"


unsigned colour = 0xFFFF;
TFT_eSPI tft = TFT_eSPI();


const char* ssid = "WlanDundA";
const char* password = "88564715531805827929";

//String server = "http://192.168.178.59/rpc/Shelly.GetStatus";
//String serverapi = "http://192.168.178.48/api/1/vitals";
String server = server_host;
#define builtIn_Led 2
#define PIN_POWER_ON 15  // LCD and battery Power Enable
#define PIN_LCD_BL 38    // BackLight enable pin (see Dimming.txt)
#define topbutton 0
#define lowerbutton 14
#define PIN_POWER_ON 15  // LCD and battery Power Enable
#define PIN_LCD_BL 38    // BackLight enable pin (see Dimming.txt)
WebServer twcserver(80);
bool charging; 
bool vehicle_connected;
bool output;
float current = 0.0;
int total;
bool connected;
float voltage = 0.0;
byte start_charging = 0;
float session_energy_wh_neu = 0.001;
float session_energy_wh = 0.000;
float leistung = 0;

long myTimer = 0;
long myTimeout = 1000;



DynamicJsonDocument doc(1024);
//StaticJsonDocument<1024> jsonDocument;
//char buffer[1024];


void handleRoot() {
  twcserver.send(200, "text/plain", "hello from esp32!");
}
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += twcserver.uri();
  message += "\nMethod: ";
  message += (twcserver.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += twcserver.args();
  message += "\n";
  for (uint8_t i = 0; i < twcserver.args(); i++) {
    message += " " + twcserver.argName(i) + ": " + twcserver.arg(i) + "\n";
  }
  twcserver.send(404, "text/plain", message);
  
}

void fetch_data()
{
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      HTTPClient http;

      Serial.print("\n Making HTTP request to URL: ");
      Serial.println(server);
      
      http.begin(server.c_str());
      
      // Send HTTP GET request
      int httpResponseCode = http.GET();
      
      if (httpResponseCode==200) {
        Serial.print("Success!!! HTTP Response code: ");
        Serial.println(httpResponseCode);
        String result = http.getString(); //this variable stores the json data returned by the server        
        Serial.println(result);
        
        deserializeJson(doc, result);
        JsonObject obj = doc.as<JsonObject>();

        current = obj["switch:0"]["current"];
        voltage = obj["switch:0"]["voltage"];
        total = obj["switch:0"]["aenergy"]["total"];
        output = obj["switch:0"]["output"];

        Serial.print("\nCurrent: ");Serial.println(current);
        Serial.print("Voltage: ");Serial.println(voltage);
        Serial.print("Total: ");Serial.println(total);
        Serial.print("Output: ");Serial.println(output);
        tft.setTextSize(2);
        tft.setCursor(80,30);
        tft.setTextColor(TFT_DARKGREY, TFT_DARKGREY);
        tft.println(" Offline");
        tft.setCursor(80,30);
        tft.setTextColor(TFT_GREEN, TFT_DARKGREY);
        tft.println(" Online");
      }
      else {
        Serial.print("Server not reachable. Error code: ");
        Serial.println(httpResponseCode);
        tft.setTextSize(2);
        tft.setCursor(80,30);
        tft.setTextColor(TFT_RED, TFT_DARKGREY);
        tft.println(" Offline");
        loop();
      }
      // Free resources
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
      setup_wifi();
    }
}
void setup_wifi(){
  delay(50);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int c=0;
  while (WiFi.status() != WL_CONNECTED) {
   
    delay(1000); //
    Serial.print(".");
    c=c+1;
    if(c>10){
        ESP.restart(); //restart ESP after 10 seconds if not connected to wifi
    }
  }

  Serial.println("");
  Serial.print("connected to Wifi with IP address: ");
  Serial.println(WiFi.localIP());
  
}

void setup() {
  Serial.begin(115200); 
  pinMode(builtIn_Led, OUTPUT);
  setup_wifi();
  twcserver.begin(); 
  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }

  twcserver.on("/", handleRoot);

  twcserver.on("/inline", []() {
  twcserver.send(200, "text/plain", "this works as well");
  });

  twcserver.onNotFound(handleNotFound);
  twcserver.on("/api/1/vitals",HTTP_GET, sendData);
  twcserver.begin();
  Serial.println("HTTP server started");
  pinMode(PIN_POWER_ON, OUTPUT);  //enables the LCD and to run on battery
  pinMode(PIN_LCD_BL, OUTPUT);    // BackLight enable pin
  pinMode(lowerbutton, INPUT);    //Right button pulled up, push = 0
  pinMode(topbutton, INPUT);      //Left button  pulled up, push = 0
  delay(100);
  digitalWrite(PIN_POWER_ON, HIGH);
  digitalWrite(PIN_LCD_BL, HIGH);
  tft.init();
  tft.setRotation(3);
  tft.setSwapBytes(true);
  tft.fillScreen(TFT_DARKGREY);  //horiz / vert<> position/dimension
  tft.setTextSize(2);
  tft.setCursor(65,10);
  tft.setTextColor(TFT_BLACK);
  tft.println("TWC3 Simulator");
  
}




void sendData(){
    StaticJsonDocument<300> obj;  
    obj["contactor_closed"] = charging;
    obj["vehicle_connected"] = connected;
    obj["session_s"] = 0;      
    obj["grid_v"] = voltage;
    obj["grid_hz"] = 49.828;
    obj["vehicle_current_a"] = current;  
    obj["currentA_a"] = current;  
    obj["currentB_a"] = 0.0;       
    obj["currentC_a"] = 0.0;      
    obj["currentN_a"] = 0.0;     
    obj["voltageA_v"] = voltage;
    obj["voltageB_v"] = 0.0;
    obj["voltageC_v"] = 0.0;
    obj["relay_coil_v"] = 11.9;
    obj["pcba_temp_c"] = 7.4;     
    obj["handle_temp_c"] = 1.8;      
    obj["mcu_temp_c"] = 15.2;     
    obj["uptime_s"] = 26103;      
    obj["input_thermopile_uv"] = -176;      
    obj["prox_v"] = 0.0;     
    obj["pilot_high_v"] = 11.9;      
    obj["pilot_low_v"] = 11.8;      
    obj["session_energy_wh"] = session_energy_wh;      
    obj["config_status"] = 5;      
    obj["evse_state"] = 1;      
   // obj["current_alerts"] = alarts; 
    String jsonString;
    serializeJson(obj, jsonString);
    twcserver.sendHeader("Content-Type", "application/json");
    twcserver.send(200, "application/json", jsonString);
    serializeJson(obj, Serial);
    twcserver.on("/api/1/vitals",HTTP_GET, sendData);
    Serial.println();
}

void loop(void) {
  // 1,5 seconds delay
  for (int i = 0; i < 15; i++) {
    delay(350);
    Serial.print("-");
    tft.setTextSize(1);
    tft.setCursor(5,150);
    tft.setTextColor(TFT_BLACK, TFT_DARKGREY);
    tft.println(i);
    if (i == 14){
      tft.setTextSize(1);
      tft.setCursor(5,150);
      tft.setTextColor(TFT_DARKGREY);
      tft.println(14);
      }
  }
  
  leistung = voltage * current;
  fetch_data();
  sendData();
  twcserver.handleClient();
  twcserver.on("/api/1/vitals",HTTP_GET, sendData);
 // Serial.print("session_energy_wh: ");Serial.println(session_energy_wh);
 // Serial.print("session_energy_wh_neu: ");Serial.println(session_energy_wh_neu);
//  Serial.print("charging: ");Serial.println(charging);
 // Serial.print("start_charging: ");Serial.println(start_charging);
  
  
  if (current <= 2) {
   charging = false;
   start_charging = 0;
    if ((session_energy_wh_neu != 0.00) && (start_charging == 0)){
     session_energy_wh_neu = total;
     start_charging = 1;
   }
  }
  else {
   charging = true;   
   session_energy_wh = (total - session_energy_wh_neu);
   
  }

  if ((charging == 1) && (session_energy_wh >= 0) && (start_charging = 1)){
    start_charging = 0;
    }

  if (output = 1) {
   connected = true;
  }
  else {
   connected = false; 
  }
  tft.setTextSize(2);
  tft.setCursor(5,30);
  tft.setTextColor(TFT_BLACK, TFT_DARKGREY);
  tft.println("Status:");
  tft.setTextSize(2);
  tft.setCursor(180,30);
  tft.setTextColor(TFT_BLACK, TFT_DARKGREY);
  tft.println("IP: ");
  tft.setTextSize(1);
  tft.setCursor(220,33);
  tft.println(WiFi.localIP());
  tft.setTextSize(1);
  tft.setCursor(240,150);
  tft.setTextColor(TFT_BLACK, TFT_DARKGREY);
  tft.println("by TheKey82");
  // Daten ausgeben (Spannung,Strom,Wh)
  tft.setTextSize(1);
  tft.setCursor(5,60);
  tft.setTextColor(TFT_BLACK, TFT_DARKGREY);
  tft.println("Volage: ");
  tft.setCursor(50,60);
  tft.setTextColor(TFT_BLACK, TFT_DARKGREY);
  tft.println(voltage,1);
  tft.setCursor(90,60);
  tft.println("V");
  tft.setTextSize(1);
  tft.setCursor(100,60);
  tft.setTextColor(TFT_BLACK, TFT_DARKGREY);
  tft.println("Current: ");
  tft.setCursor(155,60);
  tft.setTextColor(TFT_BLACK, TFT_DARKGREY);
  tft.println(current,2);
  tft.setCursor(180,60);
  tft.println("A");
  tft.setCursor(200,60);
  tft.println("Leisung:");
  tft.setCursor(270,60);
  tft.println(leistung ,0);
  tft.setTextSize(1);
  tft.setCursor(5,80);
  tft.setTextColor(TFT_BLACK, TFT_DARKGREY);
  tft.println("Total: ");
  tft.setCursor(50,80);
  tft.setTextColor(TFT_BLACK, TFT_DARKGREY);
  tft.println(total);
  tft.setTextSize(1);
  tft.setCursor(100,80);
  tft.setTextColor(TFT_BLACK, TFT_DARKGREY);
  tft.println("Season: ");
  tft.setCursor(150,80);
  tft.setTextColor(TFT_BLACK, TFT_DARKGREY);
  tft.println(session_energy_wh,0);
}
