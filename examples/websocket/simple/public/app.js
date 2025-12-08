const statusEl = document.getElementById("ws-status");
const usernameInput = document.getElementById("username");
const connectBtn = document.getElementById("connect-btn");
const disconnectBtn = document.getElementById("disconnect-btn");
const messagesEl = document.getElementById("messages");
const formEl = document.getElementById("message-form");
const inputEl = document.getElementById("message-input");

let socket = null;

function setStatus(online, extra = "") {
  if (online) {
    statusEl.textContent = extra ? `Connected (${extra})` : "Connected";
    statusEl.classList.remove("offline");
    statusEl.classList.add("online");
  } else {
    statusEl.textContent = extra ? `Disconnected (${extra})` : "Disconnected";
    statusEl.classList.remove("online");
    statusEl.classList.add("offline");
  }
}

function addMessage({ kind, user, text, raw }) {
  const div = document.createElement("div");
  div.classList.add("message");

  if (kind === "system") {
    div.classList.add("system");
    div.textContent = text;
  } else if (kind === "chat") {
    div.classList.add("chat");
    const spanUser = document.createElement("span");
    spanUser.classList.add("user");
    spanUser.textContent = user + ":";

    const spanText = document.createElement("span");
    spanText.classList.add("text");
    spanText.textContent = " " + text;

    div.appendChild(spanUser);
    div.appendChild(spanText);
  } else {
    div.textContent = raw;
  }

  messagesEl.appendChild(div);
  messagesEl.scrollTop = messagesEl.scrollHeight;
}

function connect() {
  if (socket && socket.readyState === WebSocket.OPEN) return;

  const host = window.location.hostname || "localhost";
  const url = `ws://${host}:9090/`;

  addMessage({
    kind: "system",
    text: `Connecting to ${url} ...`,
  });

  try {
    socket = new WebSocket(url);
  } catch (e) {
    console.error("WebSocket ctor error", e);
    addMessage({ kind: "system", text: "Failed to create WebSocket." });
    return;
  }

  socket.onopen = () => {
    setStatus(true, host);
    connectBtn.disabled = true;
    disconnectBtn.disabled = false;
    addMessage({ kind: "system", text: "Connected to Vix WebSocket ✅" });
  };

  socket.onclose = () => {
    setStatus(false);
    connectBtn.disabled = false;
    disconnectBtn.disabled = true;
    addMessage({ kind: "system", text: "Disconnected." });
  };

  socket.onerror = (ev) => {
    console.error("WebSocket error", ev);
    addMessage({
      kind: "system",
      text: "WebSocket error (check browser console + server logs).",
    });
  };

  socket.onmessage = (ev) => {
    const raw = ev.data;

    let parsed;
    try {
      parsed = JSON.parse(raw);
    } catch (e) {
      addMessage({ kind: "raw", raw });
      return;
    }

    const type = parsed.type || "";
    const payload = parsed.payload || {};

    if (type === "chat.system") {
      addMessage({ kind: "system", text: payload.text || raw });
    } else if (type === "chat.message") {
      const user = payload.user || "anonymous";
      const text = payload.text || "";
      addMessage({ kind: "chat", user, text });
    } else {
      addMessage({ kind: "raw", raw });
    }
  };
}

function disconnect() {
  if (socket) {
    socket.close();
    socket = null;
  }
}

// Boutons

connectBtn.addEventListener("click", (e) => {
  e.preventDefault();
  connect();
});

disconnectBtn.addEventListener("click", (e) => {
  e.preventDefault();
  disconnect();
});

// Formulaire d'envoi

formEl.addEventListener("submit", (e) => {
  e.preventDefault();
  const text = inputEl.value.trim();
  if (!text || !socket || socket.readyState !== WebSocket.OPEN) {
    addMessage({
      kind: "system",
      text: "Cannot send: WebSocket is not connected.",
    });
    return;
  }

  let user = usernameInput.value.trim();
  if (!user) user = "anonymous";

  const msg = {
    type: "chat.message",
    payload: {
      user,
      text,
    },
  };

  socket.send(JSON.stringify(msg));
  inputEl.value = "";
});

// Init

window.addEventListener("load", () => {
  setStatus(false);
});
