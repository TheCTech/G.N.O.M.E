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

float posX = -1;
float posY = -1;

String hostIP;

/* #region Setup Page */

float lastCornerRotation = -1;
int lastCornerID = -1;

void handlePanel() {
    if (posX == -1) {
        server.send(200, "text/html",
        "<!DOCTYPE html>"
        "<html><head>"
        "<meta name='viewport' content='width=device-width, initial-scale=1.5'>"
        "<meta charset='UTF-8'>"
        "</head><body>"

        "<p style='font-size: 8px'>Width x Height<br></p>"
        "<input id='width' type='number'><br>"
        "<input id='height' type='number'><br><br>"

        "<button onclick='sendRotate(-1)'>↶</button>"
        "<button onclick='sendRotate(1)'>↷</button><br><br>"

        "<button onclick='setCorner(1)'>◤</button>"
        "<button onclick='setCorner(2)'>◥</button><br>"
        "<button onclick='setCorner(3)'>◣</button>"
        "<button onclick='setCorner(4)'>◢</button><br><br>"

        "<button onclick='resetPos()'>RESET</button>"

        "<script>"
        "function setCorner(v){"
        "var width=document.getElementById('width').value;"
        "var height=document.getElementById('height').value;"
        "fetch('/setCorner?c='+v+'&w='+width+'&h='+height);"
        "};"
        "function sendRotate(d){"
        "fetch('/rotate?dir='+d);"
        "};"
        "function resetPos(){"
        "fetch('/resetPos');"
        "};"
        "</script>"
        "</body></html>"
        );
    } else {
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
}

void getCornerCoordinates(int id, float width, float height, float &outputCornerX, float &outputCornerY) {
    switch (id) {
        case 1: outputCornerX = 0;     outputCornerY = height; break; // LT
        case 2: outputCornerX = width; outputCornerY = height; break; // RT
        case 3: outputCornerX = 0;     outputCornerY = 0;      break; // LB
        case 4: outputCornerX = width; outputCornerY = 0;      break; // RB
    }
}

void intersectRays(
    float x1, float y1, float theta1,
    float x2, float y2, float theta2,
    float &ix, float &iy
) {
    float a1 = theta1 * DEG_TO_RAD;
    float a2 = theta2 * DEG_TO_RAD;

    float dx1 = cos(a1);
    float dy1 = sin(a1);

    float dx2 = cos(a2);
    float dy2 = sin(a2);

    float denom = dx1 * dy2 - dy1 * dx2;

    if (fabs(denom) < 1e-6) {
        Serial.println("Rays are parallel");
        return;
    }

    float t = ((x2 - x1) * dy2 - (y2 - y1) * dx2) / denom;

    ix = x1 + t * dx1;
    iy = y1 + t * dy1;

    return;
}

void resetPos() {
    posX = -1;
    posY = -1;
    lastCornerID = -1;
    lastCornerRotation = -1;

    server.send(200, "text/plain", "OK");
}

void setCorner() {
    if (!server.hasArg("w") || !server.hasArg("h")) return;

    int cornerID = server.arg("c").toInt();
    float width = server.arg("w").toFloat();
    float height = server.arg("h").toFloat();

    if (!(width > 0 && height > 0)) return;

    //### TODO: remove this placeholder logic ###
    float rotation = 45;

    if (lastCornerID == -1 || lastCornerID == cornerID) {
        lastCornerID = cornerID;
        lastCornerRotation = -45;//### TODO: remove this placeholder logic ###
    } else {
        // The last corner is an other corner, calculate pos
        float lastCornerX, lastCornerY, currentCornerX, currentCornerY;
        getCornerCoordinates(lastCornerID, width, height, lastCornerX, lastCornerY);
        getCornerCoordinates(cornerID, width, height, currentCornerX, currentCornerY);

        intersectRays(
            lastCornerX, lastCornerY, lastCornerRotation,
            currentCornerX, currentCornerY, rotation,
            posX, posY
        );

        Serial.print(posX);
        Serial.print(" ");
        Serial.println(posY);

        // Set the current corner as the last one
        lastCornerID = cornerID;
        lastCornerRotation = -45;//### TODO: remove this placeholder logic ###
    }

    server.send(200, "text/plain", "OK");
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

void handleRotate() {
    int direction = server.arg("dir").toInt();

    servo.write(servo.read() - direction);

    server.send(200, "text/plain", "OK");
}

void setupSetup() {
    WiFi.softAP("G.N.O.M.E " + uuid);

    Serial.println("Setup page started");

    server.on("/", handlePanel);
    server.on("/setCorner", setCorner);
    server.on("/resetPos", resetPos);
    server.on("/rotate", handleRotate);
    server.on("/setWiFi", setWiFi);
    server.begin();
}

/* #endregion */

/* #region Main Logic */
void registerClient() {
    if (http.begin(client, "http://"+hostIP+":8000/register?uuid="+uuid+"&x="+String(posX)+"&y="+String(posY))) {
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