#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "EmonLib.h"             // Include Emon Library

#include "config.h"
#define emonTxV3                 // Wemos D1 mini tolerates up to 3.3V

EnergyMonitor emon1;             // Create an instance
WiFiClient espClient;
PubSubClient client(espClient);

// WiFi settings
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
// MQTT settings
const char* mqttServer = MQTT_HOST;
const int mqttPort = MQTT_PORT;
// const char* mqttUser = MQTT_USER;
// const char* mqttPassword = MQTT_PASS;


int calcPower() {
  double Irms = emon1.calcIrms(1480);  // Calculate Irms only
  return Irms * MAINS_VOLTAGE;
}

void setup() {
  emon1.current(A0, 111.11);             // Current: input pin, calibration (2000/burden resistance)
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  Serial.printf("Connected to %s\n",ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  client.setServer(mqttServer, mqttPort);
}

void loop() {
  Serial.printf("Connecting to MQTT host: %s ", mqttServer);
  while (!client.connected()) {
    if (client.connect("ESP8266Client" )) {
      Serial.println("connected"); 
    } else {
      Serial.print("failed with state ");
      Serial.println(client.state());
      delay(2000);
    }
  }
  int power = calcPower();
  Serial.printf("Publishing %s to topic %s\n",String(power).c_str(), MQTT_TOPIC );
  client.publish(MQTT_TOPIC, String(power).c_str());
  delay(2000);
}