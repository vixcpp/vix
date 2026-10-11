#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cassert>
#include <cstdint>

// Private host-test fixture.  It deliberately uses a native listener: Vix has
// no listener API in this phase, while clients always use Provider::connect.
struct TcpLoopbackFixture
{
  TcpLoopbackFixture()
  {
    listener=::socket(AF_INET,SOCK_STREAM|SOCK_CLOEXEC,0);
    assert(listener>=0);
    sockaddr_in address{}; address.sin_family=AF_INET; address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    assert(::bind(listener,reinterpret_cast<sockaddr*>(&address),sizeof address)==0);
    assert(::listen(listener,8)==0);
    socklen_t size=sizeof address;
    assert(::getsockname(listener,reinterpret_cast<sockaddr*>(&address),&size)==0);
    port=ntohs(address.sin_port);
  }
  ~TcpLoopbackFixture() { if(peer>=0)::close(peer); if(listener>=0)::close(listener); }
  int accept_peer() { peer=::accept4(listener,nullptr,nullptr,SOCK_CLOEXEC); assert(peer>=0); return peer; }
  int listener{-1};
  int peer{-1};
  std::uint16_t port{};
};
