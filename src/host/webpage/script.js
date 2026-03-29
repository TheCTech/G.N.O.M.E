let mapW = 800;
let mapH = 600;
let clients = [];

let setupClientID = -1;

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
    function drawLine(angle, color, lineLength) {
        angle = -angle * Math.PI / 180;

        const x2 = client.x + Math.cos(angle) * lineLength;
        const y2 = client.y + Math.sin(angle) * lineLength;

        ctx.beginPath();
        ctx.moveTo(client.x, client.y);
        ctx.lineTo(x2, y2);
        ctx.strokeStyle = color;
        ctx.stroke();
    }

    // FOV
    drawLine(client.initial_direction + 90, "orange", 12);
    drawLine(client.initial_direction - 90, "orange", 12);

    if (setupClientID != -1) drawLine(client.initial_direction, "lime", 12);

    ctx.beginPath();
    ctx.arc(client.x, client.y, 5, 0, Math.PI * 2);
    ctx.fillStyle = !client.armed ? "red" : client.id == setupClientID ? "green" : "blue";
    ctx.fill();
    ctx.strokeStyle = "black";
    ctx.stroke();

    if (setupClientID != -1) return;
    // Direction
    drawLine(client.direction, "purple", 15);

    // Target
    drawLine(client.target_direction, "red", 13);
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
    const client = clients.find(c => c.id === id);
    client.target_direction += value;

    if (client.target_direction >= 360) client.target_direction = 0;
    if (client.target_direction < 0) client.target_direction = 359

    fetch("/setUser", {
        method: "PUT",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ variable: "target_direction", id: id, value: client.target_direction, addition: false })
    });

    updateClientsTable();
    drawMap();
}

function handleOffsetSlider(value) {
    const client = clients.find(c => c.id === setupClientID);
    client.initial_direction = parseInt(value)*-1; // Change direction

    drawMap();
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
    if (setupClientID != -1) return;
    fetch("/clients")
        .then(r => r.json())
        .then(data => {
            clients = data;
            updateClientsTable();
            drawMap();
        });
}

function setSetupMode(id) {
    if (id == -1 && setupClientID != -1) {
        const client = clients.find(c => c.id === setupClientID);

        fetch("/setUser", {
            method: "PUT",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ variable: "x", id: client.id, value: client.x })
        });

        fetch("/setUser", {
            method: "PUT",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ variable: "y", id: client.id, value: client.y })
        });

        fetch("/setUser", {
            method: "PUT",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ variable: "initial_direction", id: client.id, value: client.initial_direction })
        });
    }

    setupClientID = id;

    // Refresh view
    drawMap();
    updateClientsTable();
}

function handleSetupInput(inputID, value) {
    const client = clients.find(c => c.id === setupClientID);
    value = parseInt(value);
    switch (inputID) {
        case 1:
            client.y = value;
            break;
        case 2:
            client.y = mapH - value;
            break;
        case 3:
            client.x = value;
            break;
        case 4:
            client.x = mapW - value;
            break;
    }

    drawMap();
    updateClientsTable();
}

function removeClient() {
    if (!confirm("Do you really wish to remove this client?")) return;

    fetch("/removeClient?id="+setupClientID, {method: "POST"})

    clients.splice(setupClientID, 1);

    setupClientID = -1;

    // Refresh view
    drawMap();
    updateClientsTable();
}

function updateClientsTable() {
    let table = document.getElementById("clientTable");
    if (setupClientID == -1) {
        table.innerHTML = `
        <tr>
        <th>ID</th>
        <th>UUID</th>
        <th>CONTROL</th>
        <th>AUTO</th>
        <th>DIRECTION</th>
        <th>TARGET</th>
        <th>KNOCKOUT</th>
        <th>SETUP</th>
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
            <button id="left_${c.id}" onclick="sendMove(${c.id}, 1)" ${disabled}>&#8592;</button>
            <button id="right_${c.id}" onclick="sendMove(${c.id}, -1)" ${disabled}>&#8594;</button>
            </td>
            <td>
            <input type="checkbox" ${checked} ${unarmed_disabled} onchange="toggleAuto(${c.id}, this)">
            </td>
            <td ${unarmed_disabled_text}>${c.direction}</td>
            <td ${unarmed_disabled_text}>${c.target_direction}</td>
            <td>
            <button onclick="sendKO(${c.id})" style=${c.armed || c.knock_over ? "color:red;" : "background-color:red;"}>${c.armed || c.knock_over ? "KNOCK" : "REARMED"}</button>
            </td>
            <td>
            <button onclick="setSetupMode(${c.id})">SETUP</button>
            </td>
        </tr>`;
        });
    } else {
        // Setup mode
        const client = clients.find(c => c.id === setupClientID);

        table.innerHTML = `
        <center><br>
        <div>
            ↧&nbsp&nbsp<input type="number" value="${client.y}" onchange="handleSetupInput(1, this.value)">
            ↥&nbsp&nbsp<input type="number" value="${mapH - client.y}" onchange="handleSetupInput(2, this.value)">
        </div>
        <br>
        <div>
            ↦ <input type="number" value="${client.x}" onchange="handleSetupInput(3, this.value)">
            ↤ <input type="number" value="${mapW - client.x}" onchange="handleSetupInput(4, this.value)">
        </div>
        <br>
        <input type="range" min="-180" max="180" value="${client.initial_direction}" step="1" oninput="handleOffsetSlider(this.value)">
        <br><br>
        <button onclick="setSetupMode(-1)">EXIT SETUP</button>
        <br>
        <button onclick="removeClient()" style="background-color:red">REMOVE CLIENT</button>
        </center>`;
    }
}

setInterval(loadClients, 2000);
loadClients();
fetchMap();

document.addEventListener("DOMContentLoaded", () => {
    document.querySelector(".resizeBtn").addEventListener("click", resizeMap);
});