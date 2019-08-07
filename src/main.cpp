#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "EmonLib.h"             // Include Emon Library
#include "EEPROM.h"
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
const char* mqttUser = MQTT_USER;
const char* mqttPassword = MQTT_PASS;
// EmonLib
const int currentCalibration = CURRENT_CALIBRATION;
const double mainsVoltage = MAINS_VOLTAGE;
const int numMeasurements = 100;
// EEPROM data
const int powerEepromAddr = POWER_EEPROM_ADDR;
int totalPower = 0;


int calcInstantPower() {
  double Irms = 0.0;
  double instantPower = 0.0;
  Irms = emon1.calcIrms(1480);  // Calculate Irms only
  instantPower = Irms * mainsVoltage;
  return instantPower;
}

void setup() {
  emon1.current(A0, currentCalibration);             // Current: input pin, calibration (2000/burden resistance)
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
  // Do a bogus 5 reads from the sensor
  // this is to clear the large values read after micro initialization
  for(int i=0; i<5; i++){
    calcInstantPower();
  }
  // Read totalPower from EEPROM
  EEPROM.get(powerEepromAddr, totalPower);
  Serial.print("Read totalPower from EEPROM: ");
  Serial.println(totalPower,3);
}

void loop() {
  int instantPower = calcInstantPower();
  totalPower = totalPower + instantPower;
  EEPROM.put(powerEepromAddr,totalPower);
  while (!client.connected()) {
    Serial.printf("Connecting to MQTT host: %s ", mqttServer);
    if (client.connect("ESP8266Client", mqttUser, mqttPassword )) {
      Serial.println("connected"); 
    } else {
      Serial.print("failed with state ");
      Serial.println(client.state());
      delay(2000);
    }
  }
  Serial.printf("Publishing %s to topic %s\n",String(instantPower).c_str(), MQTT_TOPIC );
  client.publish(MQTT_TOPIC, String(instantPower).c_str());
  Serial.printf("TotalPower: %s\n",String(totalPower).c_str());
  delay(LOOP_DELAY*1000);
}