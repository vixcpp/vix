#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace vix::http::detail
{
  enum class MessageKind : unsigned char { request, response };
  enum class ParseStatus : unsigned char
  {
    need_more,
    headers_complete,
    body_data,
    message_complete,
    protocol_error,
  };
  enum class ParseError : unsigned char
  {
    none,
    malformed_start_line,
    line_too_large,
    malformed_header,
    obsolete_line_folding,
    too_many_headers,
    headers_too_large,
    invalid_host,
    invalid_response_semantics,
    invalid_content_length,
    conflicting_framing,
    unsupported_transfer_coding,
    malformed_chunk,
    forbidden_trailer,
    truncated_message,
    invalid_state,
  };
  enum class BodyMode : unsigned char
  {
    undecided,
    none,
    fixed_length,
    chunked,
    close_delimited,
    tunnel,
  };

  struct Limits
  {
    std::size_t start_line_bytes{8192};
    std::size_t header_bytes{32768};
    std::size_t header_fields{100};
    std::size_t chunk_line_bytes{4096};
    std::size_t chunk_metadata_bytes{65536};
    std::size_t trailer_bytes{8192};
    std::size_t trailer_fields{32};
    std::size_t staged_bytes{65536};
  };

  struct HeaderField
  {
    std::string name;
    std::string value;
  };

  struct MessageHead
  {
    MessageKind kind{MessageKind::request};
    std::string method;
    std::string target;
    std::string version;
    unsigned status_code{};
    std::string reason;
    std::vector<HeaderField> fields;
    std::vector<HeaderField> trailers;
    BodyMode body_mode{BodyMode::undecided};
    std::uint64_t content_length{};
    bool informational{};
    bool tunnel_established{};
  };

  struct ParseStep
  {
    ParseStatus status{ParseStatus::need_more};
    ParseError error{ParseError::none};
    std::size_t body_bytes{};
  };

  // Private incremental HTTP/1.1 parser. append() accepts only the bytes that
  // fit; its return value is the exact accepted prefix. next() writes decoded
  // body bytes directly into caller storage and never accumulates a message
  // body. Bytes after one message remain staged for reset_message().
  class Http1Parser final
  {
  public:
    explicit Http1Parser(Limits limits = {});
    Http1Parser(const Http1Parser &) = delete;
    Http1Parser &operator=(const Http1Parser &) = delete;

    [[nodiscard]] std::size_t append(std::span<const std::byte>);
    [[nodiscard]] ParseStep next(MessageKind, std::string_view response_to_method,
                                 std::span<std::byte> body_output,
                                 bool end_of_input = false);
    [[nodiscard]] bool reset_message() noexcept;
    [[nodiscard]] const MessageHead &head() const noexcept { return head_; }
    [[nodiscard]] std::size_t buffered_bytes() const noexcept;
    [[nodiscard]] bool failed() const noexcept { return failed_; }

  private:
    enum class State : unsigned char
    {
      start_line,
      headers,
      headers_reported,
      fixed_body,
      chunk_size,
      chunk_data,
      chunk_data_crlf,
      trailers,
      close_body,
      complete,
      failed,
      tunnel,
    };
    struct LineResult
    {
      bool ready{};
      bool invalid{};
      bool truncated{};
      std::string value;
    };

    [[nodiscard]] LineResult take_line(std::size_t maximum, bool eof);
    [[nodiscard]] ParseStep fail(ParseError) noexcept;
    [[nodiscard]] bool parse_start_line(std::string_view);
    [[nodiscard]] bool parse_header(std::string_view, bool trailer);
    [[nodiscard]] bool select_framing(std::string_view response_to_method);
    [[nodiscard]] bool consume_chunk_size(std::string_view);
    [[nodiscard]] bool valid_host(std::string_view) const noexcept;
    [[nodiscard]] std::string_view field_values(std::string_view name,
                                                std::size_t &count) const noexcept;
    void consume(std::size_t) noexcept;
    void compact() noexcept;

    Limits limits_;
    std::vector<std::byte> input_;
    std::size_t offset_{};
    MessageHead head_;
    State state_{State::start_line};
    ParseError error_{ParseError::none};
    std::size_t header_bytes_{};
    std::size_t trailer_bytes_{};
    std::size_t chunk_metadata_bytes_{};
    std::uint64_t remaining_{};
    std::uint64_t chunk_remaining_{};
    bool leading_empty_line_seen_{};
    bool failed_{};
  };

  struct OutgoingFraming
  {
    enum class Kind : unsigned char { no_body, fixed_length, chunked };
    Kind kind{Kind::no_body};
    std::uint64_t content_length{};
  };

  // Private canonical serializers used by the feasibility tests and later
  // writer integration. Failure is represented by false; output is unchanged
  // on validation failure.
  [[nodiscard]] bool serialize_request_head(
      std::string_view method, std::string_view target,
      std::span<const HeaderField> fields, OutgoingFraming framing,
      std::string &output);
  // Validates exactly one complete serialized request and returns the method
  // that was parsed from the bytes which will be transmitted.
  [[nodiscard]] bool validated_request_method(
      std::span<const std::byte> serialized_request, std::string &method);
  [[nodiscard]] bool serialize_response_head(
      unsigned status, std::string_view reason,
      std::span<const HeaderField> fields, OutgoingFraming framing,
      std::string &output);
  [[nodiscard]] bool serialize_chunk(std::span<const std::byte> body,
                                     std::span<const HeaderField> trailers,
                                     bool final, std::string &output);
} // namespace vix::http::detail
