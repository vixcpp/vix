/*
 * wasi-libc Preview 2 socket ABI probe.
 *
 * This validates the SDK/runtime libc surface only. It is intentionally not
 * registered as a Vix Network provider test until the provider can honor the
 * library's asynchronous cancellation and completion lifecycle without
 * pthread workers.
 */
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static int fail(const char *what)
{
  perror(what);
  return 1;
}

static int expect_host_denial(void)
{
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd < 0)
    return fail("socket (TCP must be enabled by the host)");

  struct sockaddr_in address = {0};
  address.sin_family = AF_INET;
  address.sin_port = htons(9);
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  int result = connect(socket_fd, (struct sockaddr *)&address, sizeof(address));
  if (result == 0)
  {
    close(socket_fd);
    fputs("host policy unexpectedly permitted loopback connect\n", stderr);
    return 1;
  }
  const int error = errno;
  close(socket_fd);
  if (error != EACCES && error != EPERM)
  {
    errno = error;
    return fail("connect did not report host policy denial");
  }
  return 0;
}

static int loopback_contract(void)
{
  int listener = socket(AF_INET, SOCK_STREAM, 0);
  int client = -1;
  int accepted = -1;
  if (listener < 0)
    return fail("socket listener");

  struct sockaddr_in address = {0};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if (bind(listener, (struct sockaddr *)&address, sizeof(address)) != 0 ||
      listen(listener, 4) != 0)
    return fail("bind/listen");
  socklen_t address_size = sizeof(address);
  if (getsockname(listener, (struct sockaddr *)&address, &address_size) != 0)
    return fail("getsockname");

  client = socket(AF_INET, SOCK_STREAM, 0);
  if (client < 0)
    return fail("socket client");
  if (connect(client, (struct sockaddr *)&address, sizeof(address)) != 0)
    return fail("connect");
  accepted = accept(listener, NULL, NULL);
  if (accepted < 0)
    return fail("accept");

  const char payload[] = "vix-wasip2-tcp";
  size_t sent = 0;
  while (sent < sizeof(payload))
  {
    ssize_t amount = send(client, payload + sent, sizeof(payload) - sent, 0);
    if (amount <= 0)
      return fail("send");
    sent += (size_t)amount;
  }
  struct pollfd readable = {.fd = accepted, .events = POLLIN};
  if (poll(&readable, 1, 1000) != 1 || !(readable.revents & POLLIN))
  {
    fputs("poll did not report readable TCP data\n", stderr);
    return 1;
  }
  char received[sizeof(payload)] = {0};
  size_t received_size = 0;
  while (received_size < sizeof(received))
  {
    ssize_t amount = recv(accepted, received + received_size,
                          sizeof(received) - received_size, 0);
    if (amount <= 0)
      return fail("recv");
    received_size += (size_t)amount;
  }
  if (memcmp(payload, received, sizeof(payload)) != 0)
  {
    fputs("TCP receive did not preserve payload\n", stderr);
    return 1;
  }
  if (shutdown(client, SHUT_WR) != 0)
    return fail("shutdown write");
  char byte;
  if (recv(accepted, &byte, 1, 0) != 0)
  {
    fputs("peer EOF was not observed after half-close\n", stderr);
    return 1;
  }

  close(accepted);
  close(client);
  close(listener);
  return 0;
}

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    fputs("usage: p2_socket_contract <denied|loopback>\n", stderr);
    return 2;
  }
  if (strcmp(argv[1], "denied") == 0)
    return expect_host_denial();
  if (strcmp(argv[1], "loopback") == 0)
    return loopback_contract();
  fputs("unknown test mode\n", stderr);
  return 2;
}
