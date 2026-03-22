let mapW = 800;
let mapH = 600;
let clients = [];


async function fetchMap() {
    const response = await fetch("/map");

    const responseJson = await response.json();

    mapW = responseJson.width;
    mapH = responseJson.height;

    drawMap();
}

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

    clients.forEach(client => drawClient(ctx, client));
}

function drawClient(ctx, client) {
    ctx.beginPath();
    ctx.arc(client.x, client.y, 5, 0, Math.PI * 2);
    ctx.fillStyle = !client.armed ? "red" : "blue";
    ctx.fill();
    ctx.strokeStyle = "black";
    ctx.stroke();
}

function resizeMap() {
    let w = prompt("Width in cm:", mapW);
    let h = prompt("Height in cm:", mapH);
    if (!w || !h) return;

    mapW = parseInt(w);
    mapH = parseInt(h);

    drawMap();

    fetch("/setMap", {
        method: "PUT",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ width: w, height: h })
    });
}

function toggleAuto(id, checkbox) {
    const client = clients.find(c => c.id === id);
    client.auto_move = checkbox.checked;

    fetch("/setUser", {
        method: "PUT",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ variable: "auto_move", id: id, value: client.auto_move })
    });

    const leftBtn = document.getElementById(`left_${id}`);
    const rightBtn = document.getElementById(`right_${id}`);
    leftBtn.disabled = client.auto_move;
    rightBtn.disabled = client.auto_move;
}

function sendMove(id, value) {
    fetch("/setUser", {
        method: "PUT",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ variable: "move_queue", id: id, value: value, addition: true })
    });

    const client = clients.find(c => c.id === id);
    client.move_queue += value;
    updateClientsTable();
}

function sendKO(id) {
    const client = clients.find(c => c.id === id);
    if (client.armed) {
        fetch("/setUser", {
            method: "PUT",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ variable: "knock_over", id: id, value: true })
        });
        client.knock_over = true;
        updateClientsTable();
    } else {
        if (!confirm("Confirm to arm")) return;

        fetch("/setUser", {
            method: "PUT",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ variable: "armed", id: id, value: true })
        });
    }
}

function loadClients() {
    fetch("/clients")
        .then(r => r.json())
        .then(data => {
            clients = data;
            updateClientsTable();
            drawMap();
        });
}

function updateClientsTable() {
    let table = document.getElementById("clientTable");
    table.innerHTML = `
    <tr>
      <th>ID</th>
      <th>UUID</th>
      <th>CONTROL</th>
      <th>AUTO</th>
      <th>QUEUE</th>
      <th>KNOCKOUT</th>
      <th>PLACEHOLDER</th>
    </tr>`;

    clients.forEach(c => {
        let checked = c.auto_move ? "checked" : "";
        let disabled = c.auto_move || !c.armed || c.knock_over ? "disabled" : "";
        let unarmed_disabled_text = !c.armed || c.knock_over ? "class=disabled-text" : "";
        let unarmed_disabled = !c.armed || c.knock_over ? "disabled" : "";

        table.innerHTML += `
      <tr>
        <td>${c.id}</td>
        <td>${c.uuid}</td>
        <td>
          <button id="left_${c.id}" onclick="sendMove(${c.id}, -1)" ${disabled}>&#8592;</button>
          <button id="right_${c.id}" onclick="sendMove(${c.id}, 1)" ${disabled}>&#8594;</button>
        </td>
        <td>
          <input type="checkbox" ${checked} ${unarmed_disabled} onchange="toggleAuto(${c.id}, this)">
        </td>
        <td ${unarmed_disabled_text}>${c.move_queue}</td>
        <td>
          <button onclick="sendKO(${c.id})" style=${c.armed || c.knock_over ? "color:red;" : "background-color:red;"}>${c.armed || c.knock_over ? "KNOCK" : "REARMED"}</button>
        </td>
        <td>
          <button onclick="">PLACEHOLDER</button>
        </td>
      </tr>`;
    });
}

setInterval(loadClients, 2000);
loadClients();
fetchMap();

document.addEventListener("DOMContentLoaded", () => {
    document.querySelector(".resizeBtn").addEventListener("click", resizeMap);
});