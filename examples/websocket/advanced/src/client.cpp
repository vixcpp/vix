/**
 * @file client.cpp
 * @brief Advanced interactive WebSocket client example for Vix.cpp
 *
 * This example demonstrates how to build a fully interactive, terminal-based
 * WebSocket client using the Vix.cpp WebSocket module. It is designed to show
 * how a real-world application would:
 *
 *  • Connect to a WebSocket server using hostname, port, and path
 *  • Send typed JSON messages ("type" + "payload" protocol)
 *  • Join and leave chat rooms dynamically
 *  • Exchange messages in real time
 *  • Handle system events (server messages, room notifications, etc.)
 *  • Reconnect automatically on failure
 *  • Send heartbeat/ping frames to maintain long-lived sessions
 *
 * Key Features Illustrated
 * -------------------------
 * 1. Auto-Reconnect:
 *      The client automatically reconnects when the connection drops, with a
 *      configurable backoff delay. This is essential for offline-first systems.
 *
 * 2. Heartbeats:
 *      A periodic ping keeps the connection alive behind proxies or NATs.
 *
 * 3. Typed Protocol Handling:
 *      The client processes structured messages sent by the server:
 *          • chat.system  – system or room events
 *          • chat.message – user-generated chat messages
 *          • other types  – forwarded as raw JSON
 *
 * 4. Interactive Command Loop:
 *      The user can:
 *          • Type chat messages directly
 *          • Switch rooms with "/join <room>"
 *          • Leave the current room with "/leave"
 *          • Exit with "/quit"
 *
 * 5. Example Workflow:
 *      • User enters a pseudonym and room name on startup
 *      • The client connects and sends a "chat.join" message
 *      • Incoming messages are formatted and printed to stdout
 *      • User input is automatically serialized as JSON frames
 *
 * Intended Usage
 * --------------
 * This example serves as a reference for building:
 *
 *  • CLI-based chat applications
 *  • Debug tools for WebSocket servers
 *  • Real-time monitoring dashboards
 *  • Stress-testing tools for room-based WebSocket systems
 *  • Education/demos for Vix.cpp networking capabilities
 *
 * How to Run
 * ----------
 *  1. Compile the example:
 *         cmake -S . -B build && cmake --build build -j
 *
 *  2. Start a compatible WebSocket server (see advanced/server.cpp example).
 *
 *  3. Run the client:
 *         ./build/examples/advanced/client
 *
 *  4. Interact:
 *         /join general
 *         /join africa
 *         hello world!
 *         /leave
 *         /quit
 *
 * This file demonstrates best practices when building interactive,
 * reconnect-friendly, real-time WebSocket clients with Vix.cpp.
 */
#include <iostream>
#include <string>
#include <memory>
#include <chrono>

#include <boost/system/error_code.hpp>

#include <vix/websocket.hpp> // expose Client + JsonMessage

namespace ws = vix::websocket;

using ClientPtr = std::shared_ptr<ws::Client>;
using Clock = std::chrono::steady_clock;

// Petit helper local : starts_with
bool starts_with(const std::string &s, const std::string &prefix)
{
    return s.size() >= prefix.size() &&
           s.compare(0, prefix.size(), prefix) == 0;
}

/**
 * @brief Configure le client WebSocket (handlers, auto-reconnect, heartbeat).
 */
ClientPtr create_chat_client(std::string host,
                             std::string port,
                             std::string path,
                             std::string user,
                             std::string room)
{
    using ws::JsonMessage;

    auto client = ws::Client::create(std::move(host),
                                     std::move(port),
                                     std::move(path));

    // ───────────── Handlers ─────────────

    // Quand la connexion WS s'ouvre → join de la room courante
    client->on_open([client, user, room]
                    {
                        std::cout << "[client] Connected ✅" << std::endl;

                        client->send(
                            "chat.join",
                            {
                                "room", room,
                                "user", user,
                            }); });

    // Réception des messages
    client->on_message([](const std::string &msg)
                       {
                           auto jm = JsonMessage::parse(msg);

                           if (!jm)
                           {
                               // Pas du JSON du protocole → afficher brut
                               std::cout << msg << std::endl;
                               return;
                           }

                           const std::string &type = jm->type;

                           if (type == "chat.system")
                           {
                               std::string text     = jm->get_string("text");
                               std::string roomName = jm->get_string("room"); // optionnel

                               if (!roomName.empty())
                               {
                                   std::cout << "[system][" << roomName << "] " << text << std::endl;
                               }
                               else
                               {
                                   std::cout << "[system] " << text << std::endl;
                               }
                           }
                           else if (type == "chat.message")
                           {
                               std::string user     = jm->get_string("user");
                               std::string text     = jm->get_string("text");
                               std::string roomName = jm->get_string("room");

                               if (user.empty())
                                   user = "anonymous";

                               if (!roomName.empty())
                               {
                                   std::cout << "[chat][" << roomName << "] " << user << ": " << text << std::endl;
                               }
                               else
                               {
                                   std::cout << "[chat] " << user << ": " << text << std::endl;
                               }
                           }
                           else
                           {
                               // Types non gérés explicitement → dump brut
                               std::cout << msg << std::endl;
                           } });

    client->on_close([]()
                     { std::cout << "[client] Disconnected." << std::endl; });

    client->on_error([](const boost::system::error_code &ec)
                     { std::cerr << "[client] error: " << ec.message() << std::endl; });

    // Auto-reconnect + heartbeat
    client->enable_auto_reconnect(true, std::chrono::seconds(3));
    client->enable_heartbeat(std::chrono::seconds(20));

    return client;
}

/**
 * @brief Boucle CLI : gère /join, /leave, /quit et envoie les messages.
 */
void run_chat_cli(ClientPtr client, std::string user, std::string room)
{
    std::cout << "Type messages, /join <room>, /leave, /quit\n";

    for (std::string line; std::getline(std::cin, line);)
    {
        if (line == "/quit")
            break;

        // /join <room>
        if (starts_with(line, "/join "))
        {
            std::string newRoom = line.substr(6);
            if (newRoom.empty())
            {
                std::cout << "[client] Usage: /join <room>\n";
                continue;
            }

            // Leave ancienne room
            client->send(
                "chat.leave",
                {
                    "room",
                    room,
                    "user",
                    user,
                });

            room = newRoom;

            // Join nouvelle room
            client->send(
                "chat.join",
                {
                    "room",
                    room,
                    "user",
                    user,
                });

            std::cout << "[client] Switched to room: " << room << "\n";
            continue;
        }

        // /leave (reste connecté, mais ne participe plus à la room)
        if (line == "/leave")
        {
            client->send(
                "chat.leave",
                {
                    "room",
                    room,
                    "user",
                    user,
                });

            std::cout << "[client] Left room: " << room << "\n";
            continue;
        }

        // Message normal → chat.message dans la room courante
        if (!line.empty())
        {
            client->send(
                "chat.message",
                {
                    "room",
                    room,
                    "user",
                    user,
                    "text",
                    line,
                });
        }
    }
}

int main()
{
    // ───────────── Prompt user + room ─────────────
    std::cout << "Pseudo: ";
    std::string user;
    std::getline(std::cin, user);
    if (user.empty())
        user = "anonymous";

    std::cout << "Room (ex: general): ";
    std::string room;
    std::getline(std::cin, room);
    if (room.empty())
        room = "general";

    // Création du client configuré
    auto client = create_chat_client("localhost", "9090", "/", user, room);

    // Connexion (async à l’intérieur)
    client->connect();

    // Boucle CLI bloquante
    run_chat_cli(client, user, room);

    // Fermeture propre
    client->close();
    return 0;
}
