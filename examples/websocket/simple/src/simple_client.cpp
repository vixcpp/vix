/**
 * @file simple_client.cpp
 * @brief Minimal WebSocket client example for Vix.cpp
 *
 * This example demonstrates the simplest possible interactive WebSocket
 * client built with the Vix.cpp WebSocket module. It connects to a server,
 * listens for typed JSON messages, prints structured chat output, and allows
 * the user to send messages through a basic terminal prompt.
 *
 * Core Features Demonstrated
 * ---------------------------
 * 1. Client Creation:
 *      The example creates a WebSocket client targeting localhost:9090 and
 *      automatically manages connection state through Vix.cpp abstractions.
 *
 * 2. Typed JSON Protocol Handling:
 *      Incoming frames are parsed using JsonMessage and routed based on their
 *      "type":
 *          • chat.system  – server/system events
 *          • chat.message – regular chat messages
 *          • fallback     – prints raw JSON
 *
 * 3. Auto-Reconnect:
 *      The client will automatically attempt to reconnect every 3 seconds
 *      if the connection is lost.
 *
 * 4. Heartbeat / Keep-Alive:
 *      A periodic ping is enabled to keep NAT/proxy connections alive.
 *
 * 5. Interactive Input Loop:
 *      The user enters a pseudonym and can then type messages in real time.
 *      Typing "/quit" closes the session gracefully.
 *
 * Intended Usage
 * --------------
 * This minimal client is ideal for:
 *  • Testing or debugging a Vix.cpp WebSocket server
 *  • Learning how to work with the JSON protocol
 *  • Building simple chat tools or monitoring utilities
 *  • Demonstrating the basics of WebSocket event handling in C++
 *
 * How to Run
 * ----------
 * 1. Start a Vix WebSocket server (see simple_server.cpp).
 * 2. Build this example:
 *        cmake -S . -B build && cmake --build build -j
 * 3. Run the client:
 *        ./build/examples/simple/simple_client
 * 4. Type messages interactively, or use "/quit" to exit.
 *
 * This is a deliberately minimal example—see the advanced client example for
 * support for rooms, reconnection logic, structured system events, and
 * persistent message workflows.
 */

#include <iostream>
#include <string>

#include <vix/websocket/client.hpp>
#include <vix/websocket/protocol.hpp>

int main()
{
    using vix::websocket::Client;
    using vix::websocket::JsonMessage;

    auto client = Client::create("localhost", "9090", "/");

    client->on_open([]
                    { std::cout << "[client] Connected ✅" << std::endl; });

    client->on_message([](const std::string &msg)
                       {
        auto jm = JsonMessage::parse(msg);

        if (!jm)
        {
            std::cout << msg << std::endl;
            return;
        }

        const std::string &type = jm->type;

        if (type == "chat.system")
        {
            std::cout << "[system] " << jm->get_string("text") << std::endl;
        }
        else if (type == "chat.message")
        {
            std::string user = jm->get_string("user");
            if (user.empty())
                user = "anonymous";

            std::cout << "[chat] " << user
                      << ": " << jm->get_string("text") << std::endl;
        }
        else
        {
            std::cout << msg << std::endl;
        } });

    client->on_close([]
                     { std::cout << "[client] Disconnected." << std::endl; });

    client->on_error([](const boost::system::error_code &ec)
                     { std::cerr << "[client] error: " << ec.message() << std::endl; });

    client->enable_auto_reconnect(true, std::chrono::seconds(3));
    client->enable_heartbeat(std::chrono::seconds(20));

    client->connect();

    // Prompt username
    std::cout << "Pseudo: ";
    std::string user;
    std::getline(std::cin, user);
    if (user.empty())
        user = "anonymous";

    std::cout << "Type messages, /quit to exit\n";

    // Message loop
    for (std::string line; std::getline(std::cin, line);)
    {
        if (line == "/quit")
            break;

        client->send(
            "chat.message",
            {
                "user",
                user,
                "text",
                line,
            });
    }

    client->close();
    return 0;
}
