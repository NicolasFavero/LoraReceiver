// ===================== Abas =====================
document.querySelectorAll(".tabButton").forEach(btn => {
    btn.addEventListener("click", () => {
        document.querySelectorAll(".tabButton").forEach(b => b.classList.remove("active"));
        document.querySelectorAll(".tabPanel").forEach(p => p.classList.add("hidden"));

        btn.classList.add("active");
        document.getElementById("tab-" + btn.dataset.tab).classList.remove("hidden");
    });
});

// ===================== Status (polling) =====================
function fmtMs(ms) {
    const s = Math.floor(ms / 1000);
    if (s < 60) return s + "s";
    const m = Math.floor(s / 60);
    if (m < 60) return m + "m" + (s % 60) + "s";
    const h = Math.floor(m / 60);
    return h + "h" + (m % 60) + "m";
}

async function pollStatus() {
    try {
        const r = await fetch("/status");
        if (!r.ok) return;
        const s = await r.json();

        const wifiBadge = document.getElementById("wifiBadge");
        wifiBadge.textContent = "WiFi: " + (s.wifiConnected ? s.wifiIp : "desconectado");
        wifiBadge.className = "badge " + (s.wifiConnected ? "ok" : "error");

        const apBadge = document.getElementById("apBadge");
        if (s.apActive) {
            apBadge.textContent = "AP: " + s.apSsid;
            apBadge.className = "badge ok";
        } else {
            apBadge.className = "badge hidden";
        }

        const mqttBadge = document.getElementById("mqttBadge");
        if (!s.mqttEnabled) {
            mqttBadge.textContent = "MQTT: desabilitado";
            mqttBadge.className = "badge";
        } else {
            mqttBadge.textContent = "MQTT: " + (s.mqttConnected ? "conectado" : "desconectado");
            mqttBadge.className = "badge " + (s.mqttConnected ? "ok" : "error");
        }

        const lastPacketBadge = document.getElementById("lastPacketBadge");
        lastPacketBadge.textContent = "Ultimo pacote: ha " + fmtMs(s.msSinceLastPacket);
        lastPacketBadge.className = "badge " + (s.msSinceLastPacket < 15000 ? "ok" : "error");

        document.getElementById("statRssi").textContent = s.lastRssi.toFixed(1) + " dBm";
        document.getElementById("statSnr").textContent  = s.lastSnr.toFixed(1) + " dB";
        document.getElementById("statHeap").textContent = Math.round(s.freeHeap / 1024) + " KB";
        document.getElementById("statUptime").textContent = fmtMs(s.uptimeMs);
    } catch (e) {
        // servidor reiniciando/trocando de WiFi -- ignora e tenta de novo no proximo ciclo
    }
}
setInterval(pollStatus, 2000);
pollStatus();

// ===================== Pacotes ao vivo (WebSocket) =====================
const packetLog = document.getElementById("packetLog");
const MAX_LOG_ENTRIES = 50;
let lastEntryEl = null;

function addPacketEntry(packet, rssi, snr) {
    const el = document.createElement("div");
    el.className = "packetEntry";

    const time = new Date().toLocaleTimeString();

    el.innerHTML =
        '<span class="meta">[' + time + '] RSSI ' + rssi.toFixed(1) +
        ' dBm, SNR ' + snr.toFixed(1) + ' dB</span><br>' + packet;

    packetLog.prepend(el);
    lastEntryEl = el;

    while (packetLog.children.length > MAX_LOG_ENTRIES) {
        packetLog.removeChild(packetLog.lastChild);
    }
}

function markLastMqttResult(ok) {
    if (!lastEntryEl) return;
    const tag = document.createElement("span");
    tag.className = ok ? "mqttOk" : "mqttFail";
    tag.textContent = ok ? "  [MQTT OK]" : "  [MQTT FALHOU]";
    lastEntryEl.appendChild(tag);
}

function connectWebSocket() {
    const ws = new WebSocket("ws://" + location.host + "/ws");

    ws.onmessage = (event) => {
        try {
            const msg = JSON.parse(event.data);

            if (msg.type === "packet") {
                addPacketEntry(msg.packet, msg.rssi, msg.snr);
            } else if (msg.type === "mqtt") {
                markLastMqttResult(msg.ok);
            }
        } catch (e) { /* mensagem inesperada -- ignora */ }
    };

    ws.onclose = () => {
        // reconecta sozinho -- comum durante troca de WiFi/reboot
        setTimeout(connectWebSocket, 2000);
    };
}
connectWebSocket();

// ===================== LoRa =====================
async function loadLoraConfig() {
    try {
        const r = await fetch("/getLora");
        if (!r.ok) return;
        const cfg = await r.json();

        ["freq", "sf", "bd", "sync", "cr", "power"].forEach(f => {
            const el = document.getElementById(f);
            if (el && cfg[f] !== undefined) el.value = cfg[f];
        });

        const crcEl = document.getElementById("crc");
        if (crcEl && cfg.crc !== undefined) crcEl.value = cfg.crc ? "1" : "0";
    } catch (e) {}
}

document.getElementById("loraForm").addEventListener("submit", async (e) => {
    e.preventDefault();
    const statusEl = document.getElementById("loraStatus");
    statusEl.textContent = "Salvando...";
    statusEl.className = "status";

    const body = {
        freq:  parseInt(document.getElementById("freq").value, 10),
        sf:    parseInt(document.getElementById("sf").value, 10),
        bd:    parseInt(document.getElementById("bd").value, 10),
        sync:  parseInt(document.getElementById("sync").value, 10),
        cr:    parseInt(document.getElementById("cr").value, 10),
        power: parseInt(document.getElementById("power").value, 10),
        crc:   parseInt(document.getElementById("crc").value, 10)
    };

    try {
        const r = await fetch("/setLora", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify(body)
        });
        const text = await r.text();
        if (!r.ok) throw new Error(text);
        statusEl.textContent = "Configuracao aplicada e salva.";
        statusEl.className = "status ok";
    } catch (err) {
        statusEl.textContent = "Erro: " + err.message;
        statusEl.className = "status error";
    }
});
loadLoraConfig();

// ===================== WiFi =====================
async function loadWifiConfig() {
    try {
        const r = await fetch("/getWifi");
        if (!r.ok) return;
        const cfg = await r.json();
        document.getElementById("currentSsid").textContent = cfg.ssid || "--";
    } catch (e) {}
}

document.getElementById("wifiForm").addEventListener("submit", async (e) => {
    e.preventDefault();
    const statusEl = document.getElementById("wifiStatus");
    const ssid = document.getElementById("wifiSsid").value;
    const password = document.getElementById("wifiPassword").value;

    if (!ssid) {
        statusEl.textContent = "Preencha o SSID.";
        statusEl.className = "status error";
        return;
    }

    statusEl.textContent = "Enviando -- a pagina pode ficar fora do ar por alguns segundos...";
    statusEl.className = "status";

    try {
        const r = await fetch("/setWifi", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ ssid, password })
        });
        const text = await r.text();
        if (!r.ok) throw new Error(text);
        statusEl.textContent = text;
        statusEl.className = "status ok";
    } catch (err) {
        statusEl.textContent = "Erro: " + err.message + " (se a placa sumiu da rede, aguarde ~15s -- ela reverte sozinha)";
        statusEl.className = "status error";
    }
});
loadWifiConfig();

// ===================== MQTT =====================
async function loadMqttConfig() {
    try {
        const r = await fetch("/getMqtt");
        if (!r.ok) return;
        const cfg = await r.json();
        document.getElementById("mqttServer").value = cfg.server || "";
        document.getElementById("mqttPort").value = cfg.port || 1883;
        document.getElementById("mqttUser").value = cfg.user || "";
        document.getElementById("mqttTopic").value = cfg.topic || "";
        document.getElementById("mqttEnabled").value = cfg.enabled ? "1" : "0";
    } catch (e) {}
}

document.getElementById("mqttForm").addEventListener("submit", async (e) => {
    e.preventDefault();
    const statusEl = document.getElementById("mqttStatus");
    statusEl.textContent = "Salvando...";
    statusEl.className = "status";

    const body = {
        server: document.getElementById("mqttServer").value,
        port: parseInt(document.getElementById("mqttPort").value, 10),
        user: document.getElementById("mqttUser").value,
        password: document.getElementById("mqttPassword").value,
        topic: document.getElementById("mqttTopic").value,
        enabled: document.getElementById("mqttEnabled").value === "1"
    };

    try {
        const r = await fetch("/setMqtt", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify(body)
        });
        const text = await r.text();
        if (!r.ok) throw new Error(text);
        statusEl.textContent = "Configuracao MQTT aplicada e salva.";
        statusEl.className = "status ok";
    } catch (err) {
        statusEl.textContent = "Erro: " + err.message;
        statusEl.className = "status error";
    }
});
loadMqttConfig();

// ===================== WiFi secundario (fallback) =====================
const wifiSecondaryList = document.getElementById("wifiSecondaryList");

function addWifiSecondaryRow(ssid = "", priority = 0) {
    const row = document.createElement("section");
    row.className = "card wifiSecondaryRow";

    row.innerHTML =
        '<input type="text" class="wsSsid" placeholder="nome da rede" value="' +
            ssid.replace(/"/g, "&quot;") + '">' +
        '<input type="password" class="wsPassword" placeholder="senha (vazio = manter a atual)">' +
        '<input type="number" class="wsPriority" placeholder="prioridade" value="' + priority + '">' +
        '<button type="button" class="wsRemoveBtn">Remover</button>';

    row.querySelector(".wsRemoveBtn").addEventListener("click", () => row.remove());

    wifiSecondaryList.appendChild(row);
}

async function loadWifiSecondaryConfig() {
    try {
        const r = await fetch("/getWifiSecondary");
        if (!r.ok) return;
        const list = await r.json();

        wifiSecondaryList.innerHTML = "";
        list.forEach(net => addWifiSecondaryRow(net.ssid, net.priority));
    } catch (e) {}
}

document.getElementById("wifiSecondaryAddBtn").addEventListener("click", () => {
    addWifiSecondaryRow("", wifiSecondaryList.children.length);
});

document.getElementById("wifiSecondarySaveBtn").addEventListener("click", async () => {
    const statusEl = document.getElementById("wifiSecondaryStatus");
    statusEl.textContent = "Salvando...";
    statusEl.className = "status";

    const list = Array.from(wifiSecondaryList.querySelectorAll(".wifiSecondaryRow")).map(row => ({
        ssid: row.querySelector(".wsSsid").value,
        password: row.querySelector(".wsPassword").value,
        priority: parseInt(row.querySelector(".wsPriority").value, 10) || 0
    })).filter(net => net.ssid.length > 0);

    try {
        const r = await fetch("/setWifiSecondary", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify(list)
        });
        const text = await r.text();
        if (!r.ok) throw new Error(text);
        statusEl.textContent = "Redes secundarias salvas.";
        statusEl.className = "status ok";
        loadWifiSecondaryConfig(); // recarrega -- senhas mantidas aparecem "vazias" de novo, de proposito
    } catch (err) {
        statusEl.textContent = "Erro: " + err.message;
        statusEl.className = "status error";
    }
});
loadWifiSecondaryConfig();
