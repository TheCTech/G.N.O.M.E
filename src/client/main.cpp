#include <Arduino.h>
#include <Servo.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>

#define SERVO_PIN D1

String uuid;

Servo servo;

ESP8266WebServer server(80);
WiFiClient client;
HTTPClient http;

bool setupCompleted = false;
bool registered = false;

String hostIP;

/* #region Setup Page */
void handlePanel() {
    server.send(200, "text/html",
    "<!DOCTYPE html>"
    "<html><head>"
    "<meta name='viewport' content='width=device-width, initial-scale=1.5'>"
    "<meta charset='UTF-8'>"
    "</head><body>"

    "<form action='/setWiFi'>"
        "<label for='ssid'>SSID:</label>"
        "<input type='text' id='ssid' name='ssid'><br>"
        "<label for='pass'>PASS:</label>"
        "<input type='password' id='pass' name='pass'><br>"
        "<label for='hip'>HOST:</label>"
        "<input type='text' id='hip' name='hip'><br>"
        "<input type='submit' value='Submit'>"
    "</body></html>"
    );
}

void setWiFi() {
    server.sendHeader("Location", "/", true);
    server.send(303, "text/plain", "");
    
    if (!server.hasArg("ssid") || !server.hasArg("hip")) return;

    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    hostIP = server.arg("hip");

    server.stop();
    WiFi.softAPdisconnect(true);

    WiFi.begin(ssid, pass);

    Serial.print("Connecting to WiFi ..");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(1000);
    }

    Serial.println("\nConnected to " + ssid);

    setupCompleted = true;
}

void setupSetup() {
    WiFi.softAP("G.N.O.M.E " + uuid);

    Serial.println("Setup page started");

    server.on("/", handlePanel);
    server.on("/setWiFi", setWiFi);
    server.begin();
}

/* #endregion */

/* #region Main Logic */
void registerClient() {
    if (http.begin(client, "http://"+hostIP+":8000/register?uuid="+uuid)) {
        int httpCode = http.GET();

        if (httpCode > 0) {

            String response = http.getString();
            Serial.println(response);
            
            if (httpCode == HTTP_CODE_OK) {
                registered = true;
            }
        }

        http.end();
    }
}
/* #endregion */

void setup() {
    Serial.begin(9600);

    uuid = WiFi.macAddress();
    Serial.println("Client started with uuid: " + uuid);

    servo.attach(SERVO_PIN, 544, 2400);

    servo.write(90);

    setupSetup();
}

void loop() {
    if (setupCompleted) {
        if (!registered) {
            registerClient();
        }
        delay(5*1000);
    } else {
        server.handleClient();
    }
}