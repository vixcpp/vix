#include <iostream>
#include <vix.hpp>
#include <vix/websocket/config.hpp>

int main()
{
  vix::config::Config core_cfg{".env"};
  vix::websocket::Config ws_cfg = vix::websocket::Config::from_core(core_cfg);

  std::cout << "WEBSOCKET_MAX_MESSAGE_SIZE=" << ws_cfg.maxMessageSize << '\n';
  std::cout << "WEBSOCKET_IDLE_TIMEOUT=" << ws_cfg.idleTimeout.count() << '\n';
  std::cout << "WEBSOCKET_ENABLE_DEFLATE="
            << (ws_cfg.enablePerMessageDeflate ? "true" : "false") << '\n';
  std::cout << "WEBSOCKET_PING_INTERVAL=" << ws_cfg.pingInterval.count() << '\n';
  std::cout << "WEBSOCKET_AUTO_PING_PONG="
            << (ws_cfg.autoPingPong ? "true" : "false") << '\n';

  return 0;
}
