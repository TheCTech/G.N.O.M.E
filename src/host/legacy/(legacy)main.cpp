#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "WebHelpers.h"
#include "config.h"

ESP8266WebServer server(80);

String clientUUIDs[MAX_CLIENTS];

int mapWidth = 300;
int mapHeight = 300;


void createClient(int index, String uuid) {
  clientUUIDs[index] = uuid;
}

void handleRegister() {
  if (!server.hasArg("uuid")) {
    server.send(400, "text/plain", "Missing uuid");
    return;
  }

  String uuid = server.arg("uuid");
  Serial.println("Client with uuid:"+uuid+" trying to connect");
  
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clientUUIDs[i] == uuid) {
      server.send(200, "text/plain", "Reconnected as C"+String(i));
      Serial.println("Reconnected as C"+String(i));
      return;
    }
  }

  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clientUUIDs[i] == "") {
      createClient(i, uuid);
      server.send(200, "text/plain", "Connected as C"+String(i));
      Serial.println("Connected as C"+String(i));
      return;
    }
  }
  server.send(429, "text/plain", "Too many clients");
  Serial.println("Rejected client: server full");
}

void handleClients() {
  String json = "[";

  for(int i=0;i<MAX_CLIENTS;i++)
  {
    if(clientUUIDs[i] == "") continue;

    if(json != "[") json += ",";

    json += "{";
    json += "\"id\":" + String(i) + ",";
    json += "\"uuid\":\"" + clientUUIDs[i] + "\",";
    json += "\"v1\":123,";
    json += "\"v2\":456";
    json += "}";
  }

  json += "]";

  server.send(200,"application/json",json);
}

void handleSetMap(){

  if(server.hasArg("w")) mapWidth = server.arg("w").toInt();
  if(server.hasArg("h")) mapHeight = server.arg("h").toInt();

  Serial.printf("Map resized to %d x %d\n", mapWidth, mapHeight);

  server.send(200,"text/plain","OK");
}

void handleMap(){

  String json = "{";
  json += "\"w\":" + String(mapWidth) + ",";
  json += "\"h\":" + String(mapHeight);
  json += "}";

  server.send(200,"application/json",json);
}


void setup() {
  Serial.begin(9600);

  WiFi.softAP(WIFI_SSID, WIFI_PASS);
  Serial.println("Host started");

  // Define routes
  server.on("/REGISTER", handleRegister);
  server.on("/panel", handlePanel);

  server.on("/clients", handleClients);
  server.on("/map", handleMap);
  server.on("/setMap", handleSetMap);

  server.onNotFound(handleNotFound);

  server.begin();
}

void loop() {
  server.handleClient();
}