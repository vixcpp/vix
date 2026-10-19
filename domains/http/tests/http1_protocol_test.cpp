#include "Http1.hpp"

#include <array>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

using namespace vix::http::detail;

namespace
{
  [[noreturn]] void fail(const char *expression, int line)
  {
    std::fprintf(stderr, "HTTP-P1 test failure at line %d: %s\n", line, expression);
    std::abort();
  }

#define CHECK(expression) ((expression) ? static_cast<void>(0) : fail(#expression, __LINE__))

  std::span<const std::byte> bytes(std::string_view value)
  {
    return {reinterpret_cast<const std::byte *>(value.data()), value.size()};
  }

  std::string text(std::span<const std::byte> value)
  {
    return {reinterpret_cast<const char *>(value.data()), value.size()};
  }

  ParseStep feed_and_next(Http1Parser &parser, std::string_view input,
                          MessageKind kind = MessageKind::request,
                          std::string_view method = {},
                          std::span<std::byte> output = {}, bool eof = false)
  {
    CHECK(parser.append(bytes(input)) == input.size());
    return parser.next(kind, method, output, eof);
  }

  ParseError parse_error(std::string_view input,
                         MessageKind kind = MessageKind::request,
                         std::string_view method = {})
  {
    Http1Parser parser;
    const auto result = feed_and_next(parser, input, kind, method);
    if (result.status == ParseStatus::protocol_error)
      return result.error;
    if (result.status == ParseStatus::headers_complete)
    {
      std::array<std::byte, 32> body{};
      for (unsigned i = 0; i < 8; ++i)
      {
        const auto next = parser.next(kind, method, body);
        if (next.status == ParseStatus::protocol_error)
          return next.error;
        if (next.status == ParseStatus::need_more ||
            next.status == ParseStatus::message_complete)
          break;
      }
    }
    return ParseError::none;
  }

  void valid_request_and_fixed_body()
  {
    Http1Parser parser;
    const std::string wire = "POST /upload?q=1 HTTP/1.1\r\nHost: example.test\r\n"
                             "Content-Length: 5\r\nX-MiXeD: yes\r\n\r\nhelloNEXT";
    CHECK(parser.append(bytes(wire)) == wire.size());
    auto step = parser.next(MessageKind::request, {}, {});
    CHECK(step.status == ParseStatus::headers_complete);
    CHECK(parser.head().method == "POST");
    CHECK(parser.head().target == "/upload?q=1");
    CHECK(parser.head().body_mode == BodyMode::fixed_length);
    CHECK(parser.head().content_length == 5);
    const auto staged_before_pause = parser.buffered_bytes();
    CHECK(parser.next(MessageKind::request, {}, {}).status == ParseStatus::need_more);
    CHECK(parser.buffered_bytes() == staged_before_pause);
    std::array<std::byte, 3> out{};
    step = parser.next(MessageKind::request, {}, out);
    CHECK(step.status == ParseStatus::body_data && step.body_bytes == 3);
    CHECK(text(std::span<const std::byte>{out.data(), step.body_bytes}) == "hel");
    step = parser.next(MessageKind::request, {}, out);
    CHECK(step.status == ParseStatus::body_data && step.body_bytes == 2);
    CHECK(text(std::span<const std::byte>{out.data(), step.body_bytes}) == "lo");
    CHECK(parser.buffered_bytes() == 4);
    CHECK(parser.next(MessageKind::request, {}, {}).status == ParseStatus::message_complete);
    CHECK(parser.reset_message());
    step = parser.next(MessageKind::request, {}, {});
    CHECK(step.status == ParseStatus::need_more);

    Http1Parser leading_empty;
    const std::string with_empty = "\r\nGET / HTTP/1.1\r\nHost: x\r\n\r\n";
    CHECK(feed_and_next(leading_empty, with_empty).status == ParseStatus::headers_complete);
    CHECK(parse_error("\r\n\r\nGET / HTTP/1.1\r\nHost: x\r\n\r\n") ==
          ParseError::malformed_start_line);
  }

  void every_byte_boundary_split()
  {
    const auto check_splits = [](std::string_view message, MessageKind kind,
                                 std::string_view method)
    {
      for (std::size_t split = 0; split <= message.size(); ++split)
      {
        Http1Parser parser;
        CHECK(parser.append(bytes(message.substr(0, split))) == split);
        std::array<std::byte, 16> output{};
        auto step = parser.next(kind, method, output);
        CHECK(step.status != ParseStatus::protocol_error);
        if (split < message.size())
          CHECK(parser.append(bytes(message.substr(split))) == message.size() - split);
        for (unsigned turn = 0; turn < 100 && step.status != ParseStatus::message_complete; ++turn)
        {
          step = parser.next(kind, method, output);
          CHECK(step.status != ParseStatus::protocol_error);
        }
        CHECK(step.status == ParseStatus::message_complete);
      }
    };
    check_splits("GET / HTTP/1.1\r\nHost: example.test\r\n\r\n",
                 MessageKind::request, {});
    check_splits("HTTP/1.1 204 No Content\r\n\r\n",
                 MessageKind::response, "GET");
    check_splits("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n0\r\n\r\n",
                 MessageKind::response, "GET");

    const std::string request = "GET / HTTP/1.1\r\nHost: example.test\r\n\r\n";
    Http1Parser bytewise;
    ParseStep step{};
    for (const char c : request)
    {
      const std::byte value{static_cast<unsigned char>(c)};
      CHECK(bytewise.append({&value, 1}) == 1);
      step = bytewise.next(MessageKind::request, {}, {});
      CHECK(step.status != ParseStatus::protocol_error);
    }
    CHECK(step.status == ParseStatus::headers_complete);
    CHECK(bytewise.next(MessageKind::request, {}, {}).status == ParseStatus::message_complete);
  }

  void thousands_of_fragments_and_fixed_seed_partition()
  {
    const std::string body(4096, 'z');
    const std::string wire = "POST /fragmented HTTP/1.1\r\nHost: x\r\nContent-Length: 4096\r\n\r\n" + body;
    const auto consume_available = [](Http1Parser &parser, std::string &received)
    {
      std::array<std::byte, 31> output{};
      for (unsigned i = 0; i < 8; ++i)
      {
        const auto step = parser.next(MessageKind::request, {}, output);
        if (step.status == ParseStatus::body_data)
          received.append(text({output.data(), step.body_bytes}));
        else if (step.status == ParseStatus::headers_complete)
          continue;
        else
        {
          CHECK(step.status == ParseStatus::need_more ||
                step.status == ParseStatus::message_complete);
          return step.status;
        }
      }
      return ParseStatus::need_more;
    };

    Http1Parser byte_fragments;
    std::string bytewise_body;
    for (const char ch : wire)
    {
      const std::byte value{static_cast<unsigned char>(ch)};
      CHECK(byte_fragments.append({&value, 1}) == 1);
      CHECK(consume_available(byte_fragments, bytewise_body) != ParseStatus::protocol_error);
    }
    while (consume_available(byte_fragments, bytewise_body) != ParseStatus::message_complete)
      CHECK(byte_fragments.buffered_bytes() == 0);
    CHECK(bytewise_body == body);

    Http1Parser partitioned;
    std::string partitioned_body;
    std::uint32_t state = 0x5a17c9e3U;
    std::size_t offset = 0;
    while (offset < wire.size())
    {
      state = state * 1664525U + 1013904223U;
      const auto count = std::min<std::size_t>(1U + (state % 23U), wire.size() - offset);
      CHECK(partitioned.append(bytes(std::string_view{wire}.substr(offset, count))) == count);
      offset += count;
      CHECK(consume_available(partitioned, partitioned_body) != ParseStatus::protocol_error);
    }
    while (consume_available(partitioned, partitioned_body) != ParseStatus::message_complete)
      CHECK(partitioned.buffered_bytes() == 0);
    CHECK(partitioned_body == body);
  }

  void eof_at_each_framing_boundary()
  {
    const auto check_truncations = [](std::string_view wire, MessageKind kind,
                                      std::string_view method)
    {
      for (std::size_t length = 0; length < wire.size(); ++length)
      {
        Http1Parser parser;
        CHECK(parser.append(bytes(wire.substr(0, length))) == length);
        std::array<std::byte, 3> output{};
        ParseStep step{};
        for (std::size_t attempt = 0; attempt < wire.size() + 4; ++attempt)
        {
          step = parser.next(kind, method, output, true);
          if (step.status == ParseStatus::protocol_error ||
              step.status == ParseStatus::message_complete)
            break;
        }
        CHECK(step.status == ParseStatus::protocol_error);
        CHECK(step.error == ParseError::truncated_message);
        CHECK(parser.next(kind, method, output, true).error == ParseError::truncated_message);
        CHECK(parser.append(bytes("GET / HTTP/1.1\r\nHost: x\r\n\r\n")) == 0);
      }
      Http1Parser complete;
      CHECK(complete.append(bytes(wire)) == wire.size());
      std::array<std::byte, 3> output{};
      ParseStep step{};
      for (std::size_t attempt = 0; attempt < wire.size() + 4; ++attempt)
      {
        step = complete.next(kind, method, output, true);
        if (step.status == ParseStatus::message_complete ||
            step.status == ParseStatus::protocol_error)
          break;
      }
      CHECK(step.status == ParseStatus::message_complete);
    };

    check_truncations("POST / HTTP/1.1\r\nHost: x\r\nContent-Length: 3\r\n\r\nabc",
                      MessageKind::request, {});
    check_truncations("POST / HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\n\r\n"
                      "2\r\nab\r\n0\r\nX-End: yes\r\n\r\n",
                      MessageKind::request, {});
  }

  void coalesced_messages_preserve_surplus()
  {
    const std::string wire = "GET /a HTTP/1.1\r\nHost: x\r\n\r\n"
                             "GET /b HTTP/1.1\r\nHost: y\r\n\r\n";
    Http1Parser parser;
    CHECK(parser.append(bytes(wire)) == wire.size());
    CHECK(parser.next(MessageKind::request, {}, {}).status == ParseStatus::headers_complete);
    CHECK(parser.next(MessageKind::request, {}, {}).status == ParseStatus::message_complete);
    CHECK(parser.buffered_bytes() == wire.size() - std::string_view{"GET /a HTTP/1.1\r\nHost: x\r\n\r\n"}.size());
    CHECK(parser.reset_message());
    CHECK(parser.next(MessageKind::request, {}, {}).status == ParseStatus::headers_complete);
    CHECK(parser.head().target == "/b");
    CHECK(parser.next(MessageKind::request, {}, {}).status == ParseStatus::message_complete);
  }

  void responses_and_no_body_rules()
  {
    {
      Http1Parser parser;
      const std::string wire = "HTTP/1.1 200 OK\r\nContent-Length: 4\r\n\r\ndata";
      CHECK(feed_and_next(parser, wire, MessageKind::response).status == ParseStatus::headers_complete);
      std::array<std::byte, 8> out{};
      const auto body = parser.next(MessageKind::response, {}, out);
      CHECK(body.status == ParseStatus::body_data && text({out.data(), body.body_bytes}) == "data");
      CHECK(parser.next(MessageKind::response, {}, {}).status == ParseStatus::message_complete);
    }
    {
      Http1Parser get_with_body;
      const std::string wire = "GET / HTTP/1.1\r\nHost: x\r\nContent-Length: 1\r\n\r\nx";
      CHECK(feed_and_next(get_with_body, wire).status == ParseStatus::headers_complete);
      CHECK(get_with_body.head().body_mode == BodyMode::fixed_length);
      std::array<std::byte, 1> body{};
      CHECK(get_with_body.next(MessageKind::request, {}, body).status == ParseStatus::body_data);
      CHECK(text(body) == "x");
    }
    const std::array<std::pair<std::string_view, std::string_view>, 5> cases{{
        {"HEAD", "200"}, {"GET", "101"}, {"GET", "204"},
        {"GET", "304"}, {"GET", "100"}}};
    for (const auto &[method, status] : cases)
    {
      Http1Parser parser;
      const bool content_length_forbidden = status == "101" || status == "204" ||
                                            status == "100";
      const std::string wire = "HTTP/1.1 " + std::string{status} + " X\r\n" +
                               (content_length_forbidden ? "" : "Content-Length: 99\r\n") +
                               "\r\nxyz";
      CHECK(feed_and_next(parser, wire, MessageKind::response, method).status == ParseStatus::headers_complete);
      CHECK(parser.head().body_mode == BodyMode::none || parser.head().body_mode == BodyMode::tunnel);
      CHECK(parser.next(MessageKind::response, method, {}).status == ParseStatus::message_complete);
      CHECK(parser.buffered_bytes() == 3);
      if (status == "101")
        CHECK(!parser.reset_message());
    }
    {
      Http1Parser parser;
      const std::string wire = "HTTP/1.1 200 Connected\r\nContent-Length: invalid\r\n"
                               "Transfer-Encoding: not-a-supported-coding\r\n\r\nxyz";
      CHECK(feed_and_next(parser, wire, MessageKind::response, "CONNECT").status == ParseStatus::headers_complete);
      CHECK(parser.head().body_mode == BodyMode::tunnel);
      CHECK(parser.next(MessageKind::response, "CONNECT", {}).status == ParseStatus::message_complete);
      CHECK(parser.buffered_bytes() == 3);
    }
    {
      Http1Parser interim;
      const std::string wire = "HTTP/1.1 100 Continue\r\n\r\n"
                               "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
      CHECK(interim.append(bytes(wire)) == wire.size());
      CHECK(interim.next(MessageKind::response, "POST", {}).status ==
            ParseStatus::headers_complete);
      CHECK(interim.next(MessageKind::response, "POST", {}).status ==
            ParseStatus::message_complete);
      CHECK(interim.head().informational);
      CHECK(interim.reset_message());
      CHECK(interim.next(MessageKind::response, "POST", {}).status ==
            ParseStatus::headers_complete);
      CHECK(interim.head().status_code == 200);
    }
  }

  void chunked_stream_and_trailers()
  {
    const std::string wire = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n"
                             "4;tag=\"x\"\r\nWiki\r\n5\r\npedia\r\n0\r\nX-End: yes\r\n\r\nTAIL";
    Http1Parser parser;
    CHECK(parser.append(bytes(wire)) == wire.size());
    CHECK(parser.next(MessageKind::response, "GET", {}).status == ParseStatus::headers_complete);
    std::array<std::byte, 3> out{};
    std::string body;
    for (unsigned i = 0; i < 20; ++i)
    {
      const auto step = parser.next(MessageKind::response, "GET", out);
      if (step.status == ParseStatus::body_data)
        body.append(text({out.data(), step.body_bytes}));
      else if (step.status == ParseStatus::message_complete)
        break;
      else
        CHECK(step.status == ParseStatus::body_data);
    }
    CHECK(body == "Wikipedia");
    CHECK(parser.head().trailers.size() == 1);
    CHECK(parser.head().trailers[0].name == "X-End");
    CHECK(parser.buffered_bytes() == 4);

    Http1Parser paused;
    const auto body_offset = wire.find("4;tag");
    CHECK(paused.append(bytes(std::string_view{wire}.substr(0, body_offset))) == body_offset);
    CHECK(paused.next(MessageKind::response, "GET", {}).status == ParseStatus::headers_complete);
    CHECK(paused.append(bytes(std::string_view{wire}.substr(body_offset, 6))) == 6);
    const auto before = paused.buffered_bytes();
    CHECK(paused.next(MessageKind::response, "GET", {}).status == ParseStatus::need_more);
    CHECK(paused.buffered_bytes() == before);
  }

  void close_delimited_and_eof()
  {
    Http1Parser parser;
    const std::string wire = "HTTP/1.1 200 OK\r\nConnection: close\r\n\r\nbody";
    CHECK(parser.append(bytes(wire)) == wire.size());
    CHECK(parser.next(MessageKind::response, "GET", {}).status == ParseStatus::headers_complete);
    CHECK(parser.head().body_mode == BodyMode::close_delimited);
    std::array<std::byte, 8> out{};
    const auto staged = parser.buffered_bytes();
    CHECK(parser.next(MessageKind::response, "GET", {}, true).status == ParseStatus::need_more);
    CHECK(parser.buffered_bytes() == staged);
    auto step = parser.next(MessageKind::response, "GET", out, true);
    CHECK(step.status == ParseStatus::body_data && step.body_bytes == 4);
    CHECK(text({out.data(), step.body_bytes}) == "body");
    CHECK(parser.next(MessageKind::response, "GET", out, true).status == ParseStatus::message_complete);

    Http1Parser fixed;
    CHECK(feed_and_next(fixed, "POST / HTTP/1.1\r\nHost: x\r\nContent-Length: 4\r\n\r\nab",
                        MessageKind::request).status == ParseStatus::headers_complete);
    CHECK(fixed.next(MessageKind::request, {}, out, true).status == ParseStatus::body_data);
    CHECK(fixed.next(MessageKind::request, {}, out, true).error == ParseError::truncated_message);

    Http1Parser chunked;
    CHECK(feed_and_next(chunked, "POST / HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\n\r\n2\r\na",
                        MessageKind::request).status == ParseStatus::headers_complete);
    CHECK(chunked.next(MessageKind::request, {}, out, true).status == ParseStatus::body_data);
    CHECK(chunked.next(MessageKind::request, {}, out, true).error == ParseError::truncated_message);
  }

  void framing_ambiguity_and_header_regressions()
  {
    const std::pair<std::string_view, ParseError> cases[]{
        {"POST / HTTP/1.1\r\nHost: x\r\nContent-Length: 3\r\nContent-Length: 3\r\n\r\nabc",
         ParseError::none},
        {"POST / HTTP/1.1\r\nHost: x\r\nContent-Length: 3, 3\r\n\r\nabc",
         ParseError::none},
        {"POST / HTTP/1.1\r\nHost: x\r\nContent-Length: 3, 4\r\n\r\nabc",
         ParseError::invalid_content_length},
        {"POST / HTTP/1.1\r\nHost: x\r\nContent-Length: -1\r\n\r\n",
         ParseError::invalid_content_length},
        {"POST / HTTP/1.1\r\nHost: x\r\nContent-Length: 18446744073709551616\r\n\r\n",
         ParseError::invalid_content_length},
        {"POST / HTTP/1.1\r\nHost: x\r\nContent-Length: 3\r\nTransfer-Encoding: chunked\r\n\r\n",
         ParseError::conflicting_framing},
        {"POST / HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: gzip, chunked\r\n\r\n",
         ParseError::unsupported_transfer_coding},
        {"POST / HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked, chunked\r\n\r\n",
         ParseError::unsupported_transfer_coding},
        {"POST / HTTP/1.0\r\nTransfer-Encoding: chunked\r\n\r\n",
         ParseError::unsupported_transfer_coding},
        {"POST / HTTP/1.1\r\nHost: x\r\nContent-Length: 3,\r\n\r\n",
         ParseError::invalid_content_length},
        {"GET / HTTP/1.1\r\n\r\n", ParseError::invalid_host},
        {"GET / HTTP/1.1\r\nHost: x\r\nHost: y\r\n\r\n", ParseError::invalid_host},
        {"GET / HTTP/1.1\r\nHost: \r\n\r\n", ParseError::none},
        {"GET / HTTP/1.1\r\nHost: bad host\r\n\r\n", ParseError::invalid_host},
        {"GET / HTTP/1.1\r\nHost: [bad:value]\r\n\r\n", ParseError::invalid_host},
        {"GET / HTTP/1.1\r\nHost: 256.0.0.1\r\n\r\n", ParseError::invalid_host},
        {"GET / HTTP/1.1\r\nBad Field: x\r\nHost: x\r\n\r\n", ParseError::malformed_header},
        {"GET / HTTP/1.1\r\nHost : x\r\n\r\n", ParseError::malformed_header},
        {"GET / HTTP/1.1\r\nHost: x\r\n folded\r\n\r\n", ParseError::obsolete_line_folding},
        {"GET / HTTP/1.1\nHost: x\n\n", ParseError::malformed_start_line},
        {"GET /bad%Q0 HTTP/1.1\r\nHost: x\r\n\r\n", ParseError::malformed_start_line},
        {"GET /bad[target] HTTP/1.1\r\nHost: x\r\n\r\n", ParseError::malformed_start_line},
        {"GET http://@/path HTTP/1.1\r\nHost: x\r\n\r\n", ParseError::malformed_start_line},
        {"GET * HTTP/1.1\r\nHost: x\r\n\r\n", ParseError::malformed_start_line},
        {"HTTP/1.1 200\r\n\r\n", ParseError::malformed_start_line},
    };
    for (const auto &[wire, expected] : cases)
    {
      const auto actual = parse_error(wire);
      if (expected == ParseError::none)
        CHECK(actual == ParseError::none);
      else
        CHECK(actual == expected);
    }
    CHECK(parse_error("GET / HTTP/1.1\r\nHost: [2001:db8::1]:443\r\n\r\n") ==
          ParseError::none);
    CHECK(parse_error("OPTIONS * HTTP/1.1\r\nHost: x\r\n\r\n") == ParseError::none);
    CHECK(parse_error("CONNECT [2001:db8::1]:443 HTTP/1.1\r\nHost: x\r\n\r\n") ==
          ParseError::none);
    CHECK(parse_error("GET http://example.test/path HTTP/1.1\r\nHost: example.test\r\n\r\n") ==
          ParseError::none);
    CHECK(parse_error("GET / HTTP/1.1\r\nHost: \r\n\r\n") == ParseError::none);
    CHECK(parse_error("HTTP/1.1 200\r\n\r\n", MessageKind::response, "GET") ==
          ParseError::malformed_start_line);
    CHECK(parse_error("HTTP/1.1 200 \r\n\r\n", MessageKind::response, "GET") ==
          ParseError::none);
    Http1Parser folded_response;
    const std::string folded = "HTTP/1.1 200 OK\r\nX-Value: first\r\n second\r\n\r\n";
    CHECK(feed_and_next(folded_response, folded, MessageKind::response, "GET").status ==
          ParseStatus::headers_complete);
    CHECK(folded_response.head().fields.size() == 1);
    CHECK(folded_response.head().fields[0].value == "first second");
    CHECK(parse_error("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\nx\r\n",
                      MessageKind::response, "GET") ==
          ParseError::malformed_chunk);
    CHECK(parse_error("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n1\r\naXX",
                      MessageKind::response, "GET") ==
          ParseError::malformed_chunk);
    CHECK(parse_error("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n0\r\nContent-Length: 1\r\n\r\n",
                      MessageKind::response, "GET") ==
          ParseError::forbidden_trailer);
    CHECK(parse_error("HTTP/1.1 204 No Content\r\nTransfer-Encoding: chunked\r\n\r\n",
                      MessageKind::response, "GET") ==
          ParseError::unsupported_transfer_coding);
    CHECK(parse_error("HTTP/1.1 100 Continue\r\nContent-Length: 0\r\n\r\n",
                      MessageKind::response, "GET") ==
          ParseError::invalid_response_semantics);
    CHECK(parse_error("HTTP/1.1 103 Early Hints\r\nContent-Length: 7\r\n\r\n",
                      MessageKind::response, "GET") ==
          ParseError::invalid_response_semantics);
    CHECK(parse_error("HTTP/1.1 204 No Content\r\nContent-Length: 0\r\n\r\n",
                      MessageKind::response, "GET") ==
          ParseError::invalid_response_semantics);
    CHECK(parse_error("HTTP/1.1 204 No Content\r\nContent-Length: 7\r\n\r\n",
                      MessageKind::response, "GET") ==
          ParseError::invalid_response_semantics);
    CHECK(parse_error("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n"
                      "0\r\nBad Field: x\r\n\r\n",
                      MessageKind::response, "GET") == ParseError::malformed_header);
    CHECK(parse_error("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n"
                      "0\r\nX-Trace: safe\n\r\n",
                      MessageKind::response, "GET") == ParseError::malformed_header);
  }

  void http10_host_and_205_semantics()
  {
    CHECK(parse_error("GET / HTTP/1.0\r\n\r\n") == ParseError::none);
    CHECK(parse_error("GET / HTTP/1.0\r\nHost: example.test\r\n\r\n") ==
          ParseError::none);
    CHECK(parse_error("GET / HTTP/1.0\r\nHost:\r\n\r\n") == ParseError::none);
    CHECK(parse_error("GET / HTTP/1.0\r\nHost: bad host\r\n\r\n") ==
          ParseError::invalid_host);
    CHECK(parse_error("GET / HTTP/1.0\r\nHost: example.test\r\n"
                      "Host: other.test\r\n\r\n") == ParseError::invalid_host);
    CHECK(parse_error("GET / HTTP/1.1\r\nHost: bad host\r\n\r\n") ==
          ParseError::invalid_host);

    Http1Parser zero_length;
    CHECK(zero_length.append(bytes("HTTP/1.1 205 Reset Content\r\n"
                                   "Content-Length: 0\r\n\r\n")) != 0);
    CHECK(zero_length.next(MessageKind::response, "GET", {}).status ==
          ParseStatus::headers_complete);
    CHECK(zero_length.next(MessageKind::response, "GET", {}).status ==
          ParseStatus::message_complete);

    CHECK(parse_error("HTTP/1.1 205 Reset Content\r\nContent-Length: 1\r\n\r\nx",
                      MessageKind::response, "GET") ==
          ParseError::invalid_response_semantics);
    CHECK(parse_error("HTTP/1.1 205 Reset Content\r\nTransfer-Encoding: chunked\r\n\r\n"
                      "1\r\nx\r\n0\r\n\r\n",
                      MessageKind::response, "GET") ==
          ParseError::invalid_response_semantics);
    Http1Parser empty_chunked;
    const auto zero_chunk = "HTTP/1.1 205 Reset Content\r\n"
                            "Transfer-Encoding: chunked\r\n\r\n0\r\n\r\n";
    CHECK(empty_chunked.append(bytes(zero_chunk)) == std::char_traits<char>::length(zero_chunk));
    auto step = empty_chunked.next(MessageKind::response, "GET", {});
    while (step.status == ParseStatus::headers_complete || step.status == ParseStatus::need_more)
      step = empty_chunked.next(MessageKind::response, "GET", {});
    CHECK(step.status == ParseStatus::message_complete);

    Http1Parser empty_close_delimited;
    CHECK(empty_close_delimited.append(bytes("HTTP/1.1 205 Reset Content\r\n\r\n")) != 0);
    CHECK(empty_close_delimited.next(MessageKind::response, "GET", {}).status ==
          ParseStatus::headers_complete);
    CHECK(empty_close_delimited.next(MessageKind::response, "GET", {}, true).status ==
          ParseStatus::message_complete);
    Http1Parser nonempty_close_delimited;
    CHECK(nonempty_close_delimited.append(bytes("HTTP/1.1 205 Reset Content\r\n\r\nx")) != 0);
    CHECK(nonempty_close_delimited.next(MessageKind::response, "GET", {}, true).status ==
          ParseStatus::headers_complete);
    CHECK(nonempty_close_delimited.next(MessageKind::response, "GET", {}, true).error ==
          ParseError::invalid_response_semantics);
  }

  void repeated_informational_responses_are_bounded_and_make_progress()
  {
    std::string wire;
    for (unsigned i = 0; i < 128; ++i)
      wire += "HTTP/1.1 103 Early Hints\r\nX-Hint: value\r\n\r\n";
    wire += "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
    CHECK(wire.size() < Limits{}.staged_bytes);

    Http1Parser parser;
    CHECK(parser.append(bytes(wire)) == wire.size());
    unsigned informational_count = 0;
    for (unsigned step_count = 0; step_count < 512; ++step_count)
    {
      auto step = parser.next(MessageKind::response, "GET", {});
      if (step.status == ParseStatus::headers_complete)
        step = parser.next(MessageKind::response, "GET", {});
      CHECK(step.status != ParseStatus::protocol_error);
      CHECK(step.status == ParseStatus::message_complete);
      if (!parser.head().informational)
      {
        CHECK(parser.head().status_code == 200);
        CHECK(informational_count == 128);
        CHECK(parser.buffered_bytes() == 0);
        return;
      }
      ++informational_count;
      CHECK(parser.reset_message());
    }
    CHECK(false);
  }

  void bounded_input_and_sticky_errors()
  {
    Limits limits;
    limits.staged_bytes = 16;
    limits.start_line_bytes = 8;
    limits.header_bytes = 8;
    limits.chunk_line_bytes = 8;
    limits.trailer_bytes = 8;
    Http1Parser parser{limits};
    const std::string input = "GET / HTTP/1.1\r\nHost: x\r\n\r\n";
    const auto accepted = parser.append(bytes(input));
    CHECK(accepted == 16);
    CHECK(parser.append(bytes(std::string_view{input}.substr(accepted))) == 0);
    CHECK(parser.next(MessageKind::request, {}, {}).error == ParseError::line_too_large);
    const auto buffered = parser.buffered_bytes();
    CHECK(parser.next(MessageKind::request, {}, {}).error == ParseError::line_too_large);
    CHECK(parser.buffered_bytes() == buffered);
    CHECK(parser.append(bytes("x")) == 0);

    Limits field_limits;
    field_limits.header_fields = 1;
    Http1Parser too_many_fields{field_limits};
    CHECK(feed_and_next(too_many_fields,
                        "GET / HTTP/1.1\r\nHost: x\r\nX: y\r\n\r\n").error ==
          ParseError::too_many_headers);

    Limits header_byte_limits;
    header_byte_limits.header_bytes = 10;
    Http1Parser too_many_header_bytes{header_byte_limits};
    CHECK(feed_and_next(too_many_header_bytes,
                        "GET / HTTP/1.1\r\nHost: x\r\n\r\n").error ==
          ParseError::headers_too_large);

    Limits chunk_line_limits;
    chunk_line_limits.chunk_line_bytes = 8;
    Http1Parser too_long_chunk_line{chunk_line_limits};
    CHECK(feed_and_next(too_long_chunk_line,
                        "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n"
                        "123456789\r\n",
                        MessageKind::response, "GET").status == ParseStatus::headers_complete);
    CHECK(too_long_chunk_line.next(MessageKind::response, "GET", {}).error ==
          ParseError::line_too_large);

    Limits metadata_limits;
    metadata_limits.chunk_metadata_bytes = 5;
    Http1Parser too_much_chunk_metadata{metadata_limits};
    const std::string chunked = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n"
                                "1\r\na\r\n0\r\n\r\n";
    CHECK(too_much_chunk_metadata.append(bytes(chunked)) == chunked.size());
    CHECK(too_much_chunk_metadata.next(MessageKind::response, "GET", {}).status ==
          ParseStatus::headers_complete);
    std::array<std::byte, 4> body{};
    CHECK(too_much_chunk_metadata.next(MessageKind::response, "GET", body).status ==
          ParseStatus::body_data);
    CHECK(too_much_chunk_metadata.next(MessageKind::response, "GET", body).error ==
          ParseError::line_too_large);
    CHECK(parse_error("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n"
                      "FFFFFFFFFFFFFFFFF\r\n",
                      MessageKind::response, "GET") == ParseError::malformed_chunk);
  }

  void serialization_round_trips_and_injection_rejection()
  {
    const HeaderField request_fields[]{{"Host", "example.test"}, {"Accept", "text/plain"}};
    std::string encoded;
    CHECK(serialize_request_head("POST", "/items", request_fields,
                                 {OutgoingFraming::Kind::fixed_length, 3}, encoded));
    Http1Parser request_parser;
    CHECK(request_parser.append(bytes(encoded + "abc")) == encoded.size() + 3);
    CHECK(request_parser.next(MessageKind::request, {}, {}).status == ParseStatus::headers_complete);
    CHECK(request_parser.head().content_length == 3);

    const HeaderField response_fields[]{{"Content-Type", "text/plain"}};
    CHECK(serialize_response_head(201, "Created", response_fields,
                                 {OutgoingFraming::Kind::chunked, 0}, encoded));
    Http1Parser response_parser;
    CHECK(response_parser.append(bytes(encoded)) == encoded.size());
    CHECK(response_parser.next(MessageKind::response, "POST", {}).status == ParseStatus::headers_complete);
    CHECK(response_parser.head().body_mode == BodyMode::chunked);
    CHECK(serialize_response_head(200, {}, response_fields,
                                  {OutgoingFraming::Kind::no_body, 0}, encoded));
    Http1Parser empty_reason_parser;
    CHECK(empty_reason_parser.append(bytes(encoded)) == encoded.size());
    CHECK(empty_reason_parser.next(MessageKind::response, "GET", {}).status ==
          ParseStatus::headers_complete);

    const HeaderField injected[]{{"X-Test\r\nInjected", "yes"}};
    CHECK(!serialize_response_head(200, "OK", injected,
                                  {OutgoingFraming::Kind::no_body, 0}, encoded));
    CHECK(!serialize_response_head(304, "Not Modified", response_fields,
                                  {OutgoingFraming::Kind::fixed_length, 1}, encoded));
    CHECK(!serialize_response_head(205, "Reset Content", response_fields,
                                   {OutgoingFraming::Kind::fixed_length, 1}, encoded));
    CHECK(!serialize_response_head(205, "Reset Content", response_fields,
                                   {OutgoingFraming::Kind::chunked, 0}, encoded));
    CHECK(serialize_response_head(205, "Reset Content", response_fields,
                                  {OutgoingFraming::Kind::fixed_length, 0}, encoded));
    const HeaderField contradiction[]{{"Content-Length", "1"}};
    CHECK(!serialize_request_head("GET", "/", contradiction,
                                  {OutgoingFraming::Kind::chunked, 0}, encoded));
    const std::byte payload[]{std::byte{'a'}, std::byte{'b'}};
    CHECK(serialize_chunk(payload, {}, true, encoded));
    CHECK(encoded == "2\r\nab\r\n0\r\n\r\n");
    CHECK(!serialize_chunk({}, injected, true, encoded));
    std::vector<std::byte> oversized_chunk(16385);
    CHECK(!serialize_chunk(oversized_chunk, {}, false, encoded));
    CHECK(!serialize_request_head("GET", "bad target", request_fields,
                                  {OutgoingFraming::Kind::no_body, 0}, encoded));
    CHECK(!serialize_request_head("GET", "*", request_fields,
                                  {OutgoingFraming::Kind::no_body, 0}, encoded));
    CHECK(!serialize_request_head("GET", "/bad[target]", request_fields,
                                  {OutgoingFraming::Kind::no_body, 0}, encoded));
    CHECK(!serialize_request_head("GET", "http://@/path", request_fields,
                                  {OutgoingFraming::Kind::no_body, 0}, encoded));
    const HeaderField matching_absolute[]{{"Host", "EXAMPLE.test:080"}};
    CHECK(serialize_request_head("GET", "http://example.test:80/path", matching_absolute,
                                 {OutgoingFraming::Kind::no_body, 0}, encoded));
    const HeaderField mismatching_absolute[]{{"Host", "other.test"}};
    encoded = "unchanged";
    CHECK(!serialize_request_head("GET", "http://example.test/path",
                                  mismatching_absolute,
                                  {OutgoingFraming::Kind::no_body, 0}, encoded));
    CHECK(encoded == "unchanged");
    const HeaderField mismatching_port[]{{"Host", "example.test:81"}};
    CHECK(!serialize_request_head("GET", "http://example.test:80/path",
                                  mismatching_port,
                                  {OutgoingFraming::Kind::no_body, 0}, encoded));
    const HeaderField unsupported_absolute[]{{"Host", "example.test"}};
    CHECK(!serialize_request_head("GET", "ftp://example.test/path",
                                  unsupported_absolute,
                                  {OutgoingFraming::Kind::no_body, 0}, encoded));
    const HeaderField ipv6_absolute[]{{"Host", "[2001:db8::1]:443"}};
    CHECK(serialize_request_head("GET", "https://[2001:DB8::1]:443/path",
                                 ipv6_absolute,
                                 {OutgoingFraming::Kind::no_body, 0}, encoded));
    const HeaderField invalid_host[]{{"Host", "bad host"}};
    CHECK(!serialize_request_head("GET", "/", invalid_host,
                                  {OutgoingFraming::Kind::no_body, 0}, encoded));
    const HeaderField empty_host[]{{"Host", ""}};
    CHECK(serialize_request_head("GET", "/", empty_host,
                                 {OutgoingFraming::Kind::no_body, 0}, encoded));
    CHECK(!serialize_request_head("GET", "/", request_fields,
                                  {OutgoingFraming::Kind::no_body, 1}, encoded));

    encoded = "unchanged";
    const std::string oversized_target(8180, 'a');
    CHECK(!serialize_request_head("GET", oversized_target, request_fields,
                                  {OutgoingFraming::Kind::no_body, 0}, encoded));
    CHECK(encoded == "unchanged");
    const std::string oversized_reason(8180, 'r');
    CHECK(!serialize_response_head(200, oversized_reason, response_fields,
                                   {OutgoingFraming::Kind::no_body, 0}, encoded));
    CHECK(encoded == "unchanged");
    const HeaderField oversized_header[]{{"X-Large", std::string(32760, 'v')}};
    CHECK(!serialize_response_head(200, "OK", oversized_header,
                                   {OutgoingFraming::Kind::no_body, 0}, encoded));
    CHECK(encoded == "unchanged");
    const HeaderField oversized_trailer[]{{"X-Large", std::string(8190, 'v')}};
    CHECK(!serialize_chunk({}, oversized_trailer, true, encoded));
    CHECK(encoded == "unchanged");
    CHECK(!serialize_chunk({}, oversized_trailer, false, encoded));
    CHECK(encoded == "unchanged");
  }

  struct CorpusOutcome
  {
    ParseStatus status{};
    ParseError error{};
    bool failed{};
    std::size_t buffered{};
    std::string body;
    std::string method;
    std::string target;
    unsigned status_code{};
    BodyMode body_mode{};
    std::uint64_t content_length{};
    std::vector<std::string> fields;
    std::vector<std::string> trailers;
  };

  CorpusOutcome parse_corpus_input(std::string_view wire, MessageKind kind,
                                  std::string_view method, std::uint32_t partition_seed)
  {
    Http1Parser parser;
    CorpusOutcome result;
    result.status = ParseStatus::need_more;
    std::array<std::byte, 17> output{};
    std::size_t offset = 0;
    std::uint32_t state = partition_seed;
    std::size_t iterations = 0;
    bool complete = false;
    const auto drain = [&]() -> bool
    {
      while (++iterations <= wire.size() * 3U + 128U)
      {
        const auto step = parser.next(kind, method, output);
        result.status = step.status;
        result.error = step.error;
        if (step.status == ParseStatus::body_data)
          result.body.append(text({output.data(), step.body_bytes}));
        else if (step.status == ParseStatus::headers_complete)
          continue;
        else if (step.status == ParseStatus::protocol_error ||
                 step.status == ParseStatus::message_complete)
          return true;
        else
          return false;
        CHECK(parser.buffered_bytes() <= Limits{}.staged_bytes);
      }
      CHECK(false && "parser exceeded bounded progress budget");
      return true;
    };

    while (offset < wire.size())
    {
      std::size_t count = wire.size() - offset;
      if (partition_seed != 0)
      {
        state ^= state << 13U;
        state ^= state >> 17U;
        state ^= state << 5U;
        count = std::min<std::size_t>(1U + state % 29U, count);
      }
      const auto fragment = bytes(wire.substr(offset, count));
      CHECK(parser.append(fragment) == count);
      offset += count;
      if (!complete && drain())
      {
        if (result.status == ParseStatus::protocol_error)
          break;
        complete = true;
      }
    }
    if (result.status != ParseStatus::protocol_error &&
        result.status != ParseStatus::message_complete)
      (void)drain();

    result.failed = parser.failed();
    result.buffered = parser.buffered_bytes();
    const auto &head = parser.head();
    result.method = head.method;
    result.target = head.target;
    result.status_code = head.status_code;
    result.body_mode = head.body_mode;
    result.content_length = head.content_length;
    CHECK(head.fields.size() <= Limits{}.header_fields);
    CHECK(head.trailers.size() <= Limits{}.trailer_fields);
    for (const auto &field : head.fields)
      result.fields.push_back(field.name + "\n" + field.value);
    for (const auto &field : head.trailers)
      result.trailers.push_back(field.name + "\n" + field.value);
    CHECK(result.buffered <= Limits{}.staged_bytes);
    if (result.status == ParseStatus::protocol_error)
    {
      CHECK(result.failed);
      const auto repeated = parser.next(kind, method, output);
      CHECK(repeated.status == ParseStatus::protocol_error);
      CHECK(repeated.error == result.error);
      CHECK(parser.append(bytes("x")) == 0);
    }
    return result;
  }

  void fixed_seed_bounded_mutation_property_corpus()
  {
    struct Seed
    {
      std::string wire;
      MessageKind kind;
      std::string_view method;
    };
    const std::array<Seed, 8> seeds{{
        {"GET / HTTP/1.1\r\nHost: example.test\r\n\r\n", MessageKind::request, {}},
        {"POST /p HTTP/1.1\r\nHost: x\r\nContent-Length: 7\r\n\r\npayload", MessageKind::request, {}},
        {"POST /c HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\n\r\n3\r\nabc\r\n0\r\nX-End: yes\r\n\r\n", MessageKind::request, {}},
        {"GET http://example.test:80/a HTTP/1.1\r\nHost: example.test:80\r\n\r\n", MessageKind::request, {}},
        {"HTTP/1.1 200 OK\r\nContent-Length: 4\r\n\r\ndata", MessageKind::response, "GET"},
        {"HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n2\r\nok\r\n0\r\n\r\n", MessageKind::response, "GET"},
        {"HTTP/1.1 204 No Content\r\n\r\n", MessageKind::response, "GET"},
        {"HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n", MessageKind::response, "HEAD"},
    }};
    constexpr std::array<std::string_view, 10> replacements{{
        "0", "1", "-1", "18446744073709551615", "18446744073709551616",
        "0, 0", "chunked, gzip", "gzip, chunked", "x", "\r\nInjected: yes"}};
    constexpr std::string_view alphabet = "\r\n:;,-0123456789 ABCxyz\t";
    std::uint32_t state = 0xC0FFEE11U;
    std::size_t mutation_count = 0;
    for (const auto &seed : seeds)
    {
      for (unsigned index = 0; index < 32; ++index)
      {
        state ^= state << 13U;
        state ^= state >> 17U;
        state ^= state << 5U;
        std::string candidate = seed.wire;
        const auto kind = index % 5U;
        if (kind == 0U && !candidate.empty())
        {
          const auto position = static_cast<std::size_t>(state) % candidate.size();
          candidate[position] = alphabet[(state >> 8U) % alphabet.size()];
        }
        else if (kind == 1U && !candidate.empty())
        {
          const auto position = static_cast<std::size_t>(state) % candidate.size();
          candidate.erase(position, 1);
        }
        else if (kind == 2U)
        {
          const auto position = static_cast<std::size_t>(state) % (candidate.size() + 1U);
          candidate.insert(position, 1, alphabet[(state >> 8U) % alphabet.size()]);
        }
        else if (kind == 3U)
        {
          const auto marker = candidate.find("Content-Length: ");
          const auto chunk = candidate.find("\r\n3\r\n");
          if (marker != std::string::npos)
          {
            const auto begin = marker + std::string_view{"Content-Length: "}.size();
            const auto end = candidate.find("\r\n", begin);
            candidate.replace(begin, end - begin,
                              replacements[(state >> 8U) % replacements.size()]);
          }
          else if (chunk != std::string::npos)
            candidate.replace(chunk + 2U, 1, replacements[(state >> 8U) % replacements.size()]);
          else
          {
            const auto host = candidate.find("Host: ");
            if (host != std::string::npos)
            {
              const auto begin = host + 6U;
              const auto end = candidate.find("\r\n", begin);
              candidate.replace(begin, end - begin,
                                replacements[(state >> 8U) % replacements.size()]);
            }
          }
        }
        else
        {
          const auto position = static_cast<std::size_t>(state) % (candidate.size() + 1U);
          candidate.insert(position, replacements[(state >> 8U) % replacements.size()]);
        }
        CHECK(candidate.size() <= 2048U);
        const auto whole = parse_corpus_input(candidate, seed.kind, seed.method, 0);
        const auto fragmented = parse_corpus_input(candidate, seed.kind, seed.method,
                                                   0x9E3779B9U ^ state);
        CHECK(whole.status == fragmented.status);
        CHECK(whole.error == fragmented.error);
        CHECK(whole.failed == fragmented.failed);
        CHECK(whole.body == fragmented.body);
        CHECK(whole.method == fragmented.method);
        CHECK(whole.target == fragmented.target);
        CHECK(whole.status_code == fragmented.status_code);
        CHECK(whole.body_mode == fragmented.body_mode);
        CHECK(whole.content_length == fragmented.content_length);
        CHECK(whole.fields == fragmented.fields);
        CHECK(whole.trailers == fragmented.trailers);
        if (whole.status != ParseStatus::protocol_error)
          CHECK(whole.buffered == fragmented.buffered);
        ++mutation_count;
      }
    }
    CHECK(mutation_count == 256U);

    constexpr std::array<std::string_view, 6> must_reject{{
        "POST / HTTP/1.1\r\nHost: x\r\nContent-Length: 18446744073709551616\r\n\r\nx",
        "POST / HTTP/1.1\r\nHost: x\r\nContent-Length: 1\r\nContent-Length: 2\r\n\r\nx",
        "POST / HTTP/1.1\r\nHost: x\r\nContent-Length: 1\r\nTransfer-Encoding: chunked\r\n\r\nx",
        "POST / HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\n\r\nZ\r\nx\r\n0\r\n\r\n",
        "POST / HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\n\r\n1\r\nxX\r\n0\r\n\r\n",
        "GET / HTTP/1.1\r\nHost : x\r\n\r\n",
    }};
    for (const auto malformed : must_reject)
    {
      const auto outcome = parse_corpus_input(malformed, MessageKind::request, {}, 0);
      CHECK(outcome.status == ParseStatus::protocol_error);
      CHECK(outcome.failed);
    }
  }
} // namespace

int main()
{
  valid_request_and_fixed_body();
  every_byte_boundary_split();
  thousands_of_fragments_and_fixed_seed_partition();
  eof_at_each_framing_boundary();
  coalesced_messages_preserve_surplus();
  responses_and_no_body_rules();
  chunked_stream_and_trailers();
  close_delimited_and_eof();
  framing_ambiguity_and_header_regressions();
  http10_host_and_205_semantics();
  repeated_informational_responses_are_bounded_and_make_progress();
  bounded_input_and_sticky_errors();
  serialization_round_trips_and_injection_rejection();
  fixed_seed_bounded_mutation_property_corpus();
  return 0;
}
