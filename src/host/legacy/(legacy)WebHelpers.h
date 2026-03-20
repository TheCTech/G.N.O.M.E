#ifndef WEBHELPERS_H
#define WEBHELPERS_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

extern ESP8266WebServer server;

const char PANEL_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body {
  font-family: Arial, sans-serif;
  margin: 0;
  padding: 20px;
  display: flex;
  flex-direction: column;
  align-items: center;
}

.resizeBtn {
  position: relative;
  margin-bottom: 10px;
  padding: 4px 8px;
  font-size: 8px;
  align-self: flex-start;
}

.panelWrapper {
  position: relative;
  width: 100%;
  max-width: 1000px;
  overflow: auto;
  margin-bottom: 20px;
  box-sizing: border-box;
  padding: 0 10px;
}

#roomMap {
  border: 3px solid black;
  background: #f5f5f5;
  display: block;
  width: 100%;
  height: auto;
  box-sizing: border-box;
}

table {
  border-collapse: collapse;
  width: 100%;
  max-width: 600px;
}

th, td {
  border: 1px solid black;
  padding: 6px;
  text-align: center;
  font-size: 14px;
}

button {
  padding: 4px 8px;
}
</style>
</head>
<body>

<button class="resizeBtn" onclick="resizeMap()">Resize</button>

<h2>ESP Panel</h2>

<div class="panelWrapper">
  <canvas id="roomMap"></canvas>
</div>

<table id="clientTable">
<tr>
  <th>ID</th>
  <th>UUID</th>
  <th>B1</th>
  <th>B2</th>
  <th>B3</th>
  <th>VAL1</th>
  <th>VAL2</th>
</tr>
</table>

<script>
let mapW = 300;
let mapH = 300;

function drawMap() {
  const canvas = document.getElementById("roomMap");
  canvas.width = mapW;
  canvas.height = mapH;

  const ctx = canvas.getContext("2d");
  ctx.clearRect(0, 0, canvas.width, canvas.height);

  ctx.fillStyle = "#f5f5f5";
  ctx.fillRect(0, 0, canvas.width, canvas.height);

  ctx.strokeStyle = "black";
  ctx.lineWidth = 3;
  ctx.strokeRect(0, 0, canvas.width, canvas.height);
}

function resizeMap() {
  let w = prompt("Width in px:", mapW);
  let h = prompt("Height in px:", mapH);
  if (!w || !h) return;

  mapW = parseInt(w);
  mapH = parseInt(h);

  drawMap();
  fetch(`/setMap?w=${mapW}&h=${mapH}`);
}

function loadClients() {
  fetch("/clients")
    .then(r => r.json())
    .then(data => {
      let table = document.getElementById("clientTable");
      table.innerHTML = `
      <tr>
        <th>ID</th>
        <th>UUID</th>
        <th>CONTROL</th>
        <th>B2</th>
        <th>B3</th>
        <th>VAL1</th>
        <th>VAL2</th>
      </tr>`;
      data.forEach(c => {
        table.innerHTML += `
        <tr>
          <td>${c.id}</td>
          <td>${c.uuid}</td>
          <td class="controlCell">
            <button onclick="changeValue(${c.id}, -1)">&#8592;</button>
            <button onclick="changeValue(${c.id}, 1)">&#8594;</button>
          </td>
          <td><button onclick="sendCmd(${c.id},2)">B2</button></td>
          <td><button onclick="sendCmd(${c.id},3)">B3</button></td>
          <td>${c.v1}</td>
          <td>${c.v2}</td>
        </tr>`;
      });
    });

  fetch("/map")
    .then(r => r.json())
    .then(data => {
      mapW = data.w;
      mapH = data.h;
      drawMap();
    });
}

function sendCmd(id, cmd) {
  fetch(`/cmd?id=${id}&c=${cmd}`);
}

function changeValue(id, delta) {
  // delta = +1 or -1
  fetch(`/changeValue?id=${id}&delta=${delta}`);
}

setInterval(loadClients, 2000);
loadClients();
</script>

</body>
</html>
)rawliteral";

// Redirect a client to a URL
void redirectClient(const String &url) {
  server.sendHeader("Location", url, true);
  server.send(303, "text/plain", "");
}

void handleNotFound() {
  server.send(404, "text/plain", "");
}

void handlePanel() {
  server.send_P(200, "text/html", PANEL_HTML);
}

#endif