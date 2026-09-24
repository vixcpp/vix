const statusValue = document.getElementById("status-value");
const messageValue = document.getElementById("message-value");
const uptimeValue = document.getElementById("uptime-value");
const tickValue = document.getElementById("tick-value");
const clientsValue = document.getElementById("clients-value");
const logBox = document.getElementById("log-box");
const refreshBtn = document.getElementById("refresh-btn");
const statusBtn = document.getElementById("status-btn");

function appendLog(line) {
  const ts = new Date().toISOString();
  logBox.textContent = `[${ts}] ${line}\n` + logBox.textContent;
}

function updateSnapshot(payload) {
  statusValue.textContent = payload.status ?? "unknown";
  messageValue.textContent = payload.message ?? "";
  uptimeValue.textContent = `${payload.uptime_seconds ?? 0}s`;
  tickValue.textContent = `${payload.tick ?? 0}`;
  clientsValue.textContent = `${payload.active_clients ?? 0}`;
}

function wsUrl() {
  const protocol = window.location.protocol === "https:" ? "wss:" : "ws:";
  return `${protocol}//${window.location.hostname}:9090/`;
}

const socket = new WebSocket(wsUrl());

socket.addEventListener("open", () => {
  appendLog("websocket connected");
});

socket.addEventListener("message", (event) => {
  appendLog(`received ${event.data}`);

  try {
    const message = JSON.parse(event.data);

    if (message.type === "dashboard.snapshot") {
      updateSnapshot(message.payload || {});
    }

    if (message.type === "dashboard.status") {
      const payload = message.payload || {};
      if (payload.status) {
        statusValue.textContent = payload.status;
      }
      if (payload.active_clients !== undefined) {
        clientsValue.textContent = `${payload.active_clients}`;
      }
    }
  } catch (error) {
    appendLog(`json parse error: ${error.message}`);
  }
});

socket.addEventListener("close", () => {
  appendLog("websocket closed");
});

socket.addEventListener("error", () => {
  appendLog("websocket error");
});

refreshBtn.addEventListener("click", () => {
  socket.send(
    JSON.stringify({
      type: "dashboard.refresh",
      payload: {},
    }),
  );
});

statusBtn.addEventListener("click", () => {
  socket.send(
    JSON.stringify({
      type: "dashboard.status",
      payload: {},
    }),
  );
});

fetch("/stats")
  .then((response) => response.json())
  .then((data) => {
    updateSnapshot(data);
    appendLog("initial snapshot loaded from /stats");
  })
  .catch((error) => {
    appendLog(`initial fetch failed: ${error.message}`);
  });
