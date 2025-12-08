# Vix WebSocket - Long Polling (Offline First)

Long Polling is used when WebSockets cannot connect or the network is unstable.

---

# 1. Server Poll Endpoint

```cpp
auto pending = store.dequeue(session_id, max);
return pending;
```

---

# 2. Browser Poll Loop

```js
async function poll() {
  try {
    const res = await fetch("/ws/poll?session_id=123");
    const arr = await res.json();
    for (const msg of arr) handle(msg);
  } catch (e) {
    console.log("Offline:", e);
  }
  poll();
}
```

---

# 3. Sending messages (LP mode)

```js
await fetch("/ws/send", {
  method: "POST",
  body: JSON.stringify({
    session_id: sid,
    type: "chat.message",
    payload: { text: "Hello" }
  })
});
```

---

# 4. Queue Behavior

- Messages stored in SQLite  
- Incrementing IDs  
- Poll returns only new messages  
- Queue expires older entries  

---

# 5. Combined WS + LP Logic

1. Try WS  
2. If WS closes -> LP  
3. When WS reconnects -> stop LP  
4. If offline -> queue messages  
5. Replay queue on reconnect  
