#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

WiFiClient client;

String uuid;
bool registered = false;

void registerClient() {
  HTTPClient http;
  WiFiClient client;

  if (http.begin(client, "http://192.168.4.1/REGISTER?uuid="+uuid)) {
    int httpCode = http.GET();

    if (httpCode > 0) {

      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_TOO_MANY_REQUESTS) {

        String response = http.getString();
        Serial.println(response);

        registered = true;
      }
    }
    http.end();
  }
}

void setup() {
  Serial.begin(9600);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }

  uuid = WiFi.macAddress();

  Serial.print("\nClient started as: ");
  Serial.println(uuid);
}

void loop() {
  if (!registered) {
    registerClient();
  }
  delay(1000*5);
}