#include "Http1.hpp"

#include <algorithm>
#include <charconv>
#include <limits>
#include <string>
#include <utility>

namespace vix::http::detail
{
  namespace
  {
    unsigned char octet(std::byte value) noexcept
    {
      return std::to_integer<unsigned char>(value);
    }

    unsigned char lower_ascii(unsigned char value) noexcept
    {
      if (value >= static_cast<unsigned char>('A') && value <= static_cast<unsigned char>('Z'))
        return static_cast<unsigned char>(value + ('a' - 'A'));
      return value;
    }

    bool equal_ascii_case_insensitive(std::string_view left,
                                      std::string_view right) noexcept
    {
      if (left.size() != right.size())
        return false;
      for (std::size_t i = 0; i < left.size(); ++i)
      {
        if (lower_ascii(static_cast<unsigned char>(left[i])) !=
            lower_ascii(static_cast<unsigned char>(right[i])))
          return false;
      }
      return true;
    }

    bool token_char(unsigned char c) noexcept
    {
      if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
          (c >= 'a' && c <= 'z'))
        return true;
      constexpr std::string_view punctuation{"!#$%&'*+-.^_`|~"};
      return punctuation.find(static_cast<char>(c)) != std::string_view::npos;
    }

    bool valid_token(std::string_view value) noexcept
    {
      if (value.empty())
        return false;
      for (const char ch : value)
        if (!token_char(static_cast<unsigned char>(ch)))
          return false;
      return true;
    }

    std::string_view trim_ows(std::string_view value) noexcept
    {
      while (!value.empty() && (value.front() == ' ' || value.front() == '\t'))
        value.remove_prefix(1);
      while (!value.empty() && (value.back() == ' ' || value.back() == '\t'))
        value.remove_suffix(1);
      return value;
    }

    bool valid_field_value(std::string_view value) noexcept
    {
      for (const unsigned char c : value)
      {
        if (c == '\t' || c == ' ' || (c >= 0x21 && c <= 0x7e) || c >= 0x80)
          continue;
        return false;
      }
      return true;
    }

    bool parse_decimal(std::string_view text, std::uint64_t &value) noexcept
    {
      if (text.empty())
        return false;
      std::uint64_t parsed = 0;
      for (const char ch : text)
      {
        if (ch < '0' || ch > '9')
          return false;
        const auto digit = static_cast<std::uint64_t>(ch - '0');
        if (parsed > (std::numeric_limits<std::uint64_t>::max() - digit) / 10)
          return false;
        parsed = parsed * 10 + digit;
      }
      value = parsed;
      return true;
    }

    int hex_value(char c) noexcept
    {
      if (c >= '0' && c <= '9') return c - '0';
      if (c >= 'a' && c <= 'f') return c - 'a' + 10;
      if (c >= 'A' && c <= 'F') return c - 'A' + 10;
      return -1;
    }

    bool valid_ipv4(std::string_view value) noexcept
    {
      unsigned parts = 0;
      while (!value.empty())
      {
        const auto dot = value.find('.');
        const auto component = value.substr(0, dot);
        if (component.empty() || component.size() > 3 ||
            (component.size() > 1 && component.front() == '0'))
          return false;
        unsigned number = 0;
        for (const char ch : component)
        {
          if (ch < '0' || ch > '9')
            return false;
          number = number * 10U + static_cast<unsigned>(ch - '0');
        }
        if (number > 255)
          return false;
        ++parts;
        if (dot == std::string_view::npos)
          break;
        value.remove_prefix(dot + 1);
      }
      return parts == 4;
    }

    bool count_ipv6_groups(std::string_view side, bool allow_ipv4,
                           unsigned &groups) noexcept
    {
      if (side.empty() || side.front() == ':' || side.back() == ':')
        return false;
      while (!side.empty())
      {
        const auto colon = side.find(':');
        const auto component = side.substr(0, colon);
        if (component.empty())
          return false;
        if (component.find('.') != std::string_view::npos)
        {
          if (!allow_ipv4 || colon != std::string_view::npos || !valid_ipv4(component))
            return false;
          groups += 2;
        }
        else
        {
          if (component.size() > 4)
            return false;
          for (const char ch : component)
            if (hex_value(ch) < 0)
              return false;
          ++groups;
        }
        if (colon == std::string_view::npos)
          break;
        side.remove_prefix(colon + 1);
      }
      return true;
    }

    bool valid_ipv6(std::string_view value) noexcept
    {
      const auto compression = value.find("::");
      if (compression == std::string_view::npos)
      {
        unsigned groups = 0;
        return count_ipv6_groups(value, true, groups) && groups == 8;
      }
      if (value.find("::", compression + 2) != std::string_view::npos)
        return false;
      unsigned groups = 0;
      const auto left = value.substr(0, compression);
      const auto right = value.substr(compression + 2);
      return (left.empty() || count_ipv6_groups(left, false, groups)) &&
             (right.empty() || count_ipv6_groups(right, true, groups)) && groups < 8;
    }

    bool valid_ip_literal(std::string_view value) noexcept
    {
      if (!value.empty() && lower_ascii(static_cast<unsigned char>(value.front())) == 'v')
      {
        const auto dot = value.find('.');
        if (dot < 2 || dot + 1 >= value.size())
          return false;
        for (std::size_t i = 1; i < dot; ++i)
          if (hex_value(value[i]) < 0)
            return false;
        for (std::size_t i = dot + 1; i < value.size(); ++i)
        {
          const auto c = static_cast<unsigned char>(value[i]);
          if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
                (c >= 'a' && c <= 'z') || c == '-' || c == '.' || c == '_' ||
                c == '~' || c == ':' || std::string_view{"!$&'()*+,;="}.find(
                    static_cast<char>(c)) != std::string_view::npos))
            return false;
        }
        return true;
      }
      const auto zone = value.find("%25");
      if (zone == std::string_view::npos)
        return valid_ipv6(value);
      if (zone == 0 || zone + 3 >= value.size() ||
          value.find("%25", zone + 3) != std::string_view::npos ||
          !valid_ipv6(value.substr(0, zone)))
        return false;
      for (std::size_t i = zone + 3; i < value.size(); ++i)
      {
        const auto c = static_cast<unsigned char>(value[i]);
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
              (c >= 'a' && c <= 'z') || c == '-' || c == '.' || c == '_' || c == '~'))
          return false;
      }
      return true;
    }

    bool valid_reg_name(std::string_view value) noexcept
    {
      if (value.empty())
        return false;
      for (std::size_t i = 0; i < value.size(); ++i)
      {
        const auto c = static_cast<unsigned char>(value[i]);
        if (c == '%')
        {
          if (i + 2 >= value.size() || hex_value(value[i + 1]) < 0 ||
              hex_value(value[i + 2]) < 0)
            return false;
          i += 2;
          continue;
        }
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
              (c >= 'a' && c <= 'z') || c == '-' || c == '.' || c == '_' ||
              c == '~' || std::string_view{"!$&'()*+,;="}.find(static_cast<char>(c)) !=
                  std::string_view::npos))
          return false;
      }
      return true;
    }

    bool valid_host_value(std::string_view value) noexcept
    {
      // RFC 9112 permits an empty Host value when the target URI has no
      // authority component; routing policy may still reject that request.
      if (value.empty())
        return true;
      for (const unsigned char c : value)
        if (c <= 0x20 || c == 0x7f)
          return false;

      std::string_view host = value;
      std::string_view port;
      if (value.front() == '[')
      {
        const auto close = value.find(']');
        if (close == std::string_view::npos || close == 1 ||
            !valid_ip_literal(value.substr(1, close - 1)))
          return false;
        host = value.substr(0, close + 1);
        if (close + 1 < value.size())
        {
          if (value[close + 1] != ':')
            return false;
          port = value.substr(close + 2);
        }
      }
      else
      {
        const auto colon = value.find(':');
        if (colon != std::string_view::npos)
        {
          if (value.find(':', colon + 1) != std::string_view::npos)
            return false;
          host = value.substr(0, colon);
          port = value.substr(colon + 1);
        }
        if (host.empty())
          return false;
        const bool numeric_dotted =
            std::all_of(host.begin(), host.end(), [](char c)
                        { return (c >= '0' && c <= '9') || c == '.'; }) &&
            host.find('.') != std::string_view::npos;
        if ((numeric_dotted && !valid_ipv4(host)) || !valid_reg_name(host))
          return false;
      }
      if (!port.empty())
      {
        std::uint64_t number = 0;
        if (!parse_decimal(port, number) || number > 65535)
          return false;
      }
      else if (value.back() == ':' ||
               (value.front() == '[' && value.back() == ':' &&
                value.find(']') < value.size() - 1))
        return false;
      return !host.empty();
    }

    bool valid_uri_chars(std::string_view value, bool allow_slash,
                         bool allow_question, bool allow_colon,
                         bool allow_at) noexcept
    {
      constexpr std::string_view sub_delimiters{"!$&'()*+,;="};
      for (std::size_t i = 0; i < value.size(); ++i)
      {
        const auto c = static_cast<unsigned char>(value[i]);
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '.' || c == '_' || c == '~' ||
            sub_delimiters.find(static_cast<char>(c)) != std::string_view::npos ||
            (allow_colon && c == ':') || (allow_at && c == '@') ||
            (allow_slash && c == '/') || (allow_question && c == '?'))
          continue;
        if (c == '%' && i + 2 < value.size() && hex_value(value[i + 1]) >= 0 &&
            hex_value(value[i + 2]) >= 0)
        {
          i += 2;
          continue;
        }
        return false;
      }
      return true;
    }

    bool valid_request_target(std::string_view target,
                              std::string_view method) noexcept
    {
      if (target == "*")
        return method == "OPTIONS";
      if (method == "CONNECT")
      {
        std::string_view host;
        std::string_view port;
        if (!target.empty() && target.front() == '[')
        {
          const auto close = target.find(']');
          if (close == std::string_view::npos || close + 1 >= target.size() ||
              target[close + 1] != ':')
            return false;
          host = target.substr(0, close + 1);
          if (!valid_host_value(host))
            return false;
          port = target.substr(close + 2);
        }
        else
        {
          const auto colon = target.rfind(':');
          if (colon == std::string_view::npos || target.find(':') != colon)
            return false;
          host = target.substr(0, colon);
          if (!valid_host_value(host))
            return false;
          port = target.substr(colon + 1);
        }
        std::uint64_t number = 0;
        return parse_decimal(port, number) && number != 0 && number <= 65535;
      }
      if (!target.empty() && target.front() == '/')
        return valid_uri_chars(target, true, true, true, true);

      const auto colon = target.find(':');
      if (colon == std::string_view::npos || colon == 0 ||
          target.substr(colon + 1, 2) != "//")
        return false;
      for (std::size_t i = 0; i < colon; ++i)
      {
        const auto c = static_cast<unsigned char>(target[i]);
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
              (i != 0 && ((c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.'))))
          return false;
      }
      const auto authority_begin = colon + 3;
      const auto suffix_begin = target.find_first_of("/?", authority_begin);
      const auto authority = target.substr(
          authority_begin,
          (suffix_begin == std::string_view::npos ? target.size() : suffix_begin) -
              authority_begin);
      if (authority.empty())
        return false;
      const auto userinfo = authority.rfind('@');
      const auto host_port = userinfo == std::string_view::npos
                                 ? authority
                                 : authority.substr(userinfo + 1);
      if ((userinfo != std::string_view::npos &&
           !valid_uri_chars(authority.substr(0, userinfo), false, false, true, false)) ||
          host_port.empty() || !valid_host_value(host_port))
        return false;
      if (suffix_begin == std::string_view::npos)
        return true;
      return valid_uri_chars(target.substr(suffix_begin), true, true, true, true);
    }

    bool valid_quoted(std::string_view value, std::size_t &position) noexcept
    {
      if (position >= value.size() || value[position] != '"')
        return false;
      ++position;
      while (position < value.size())
      {
        const auto c = static_cast<unsigned char>(value[position++]);
        if (c == '"')
          return true;
        if (c == '\\')
        {
          if (position == value.size())
            return false;
          const auto escaped = static_cast<unsigned char>(value[position++]);
          if (!(escaped == '\t' || escaped == ' ' || escaped == 0x21 ||
                (escaped >= 0x23 && escaped <= 0x5b) ||
                (escaped >= 0x5d && escaped <= 0x7e) || escaped >= 0x80))
            return false;
          continue;
        }
        if (!(c == '\t' || c == ' ' || c == 0x21 ||
              (c >= 0x23 && c <= 0x5b) ||
              (c >= 0x5d && c <= 0x7e) || c >= 0x80))
          return false;
      }
      return false;
    }

    bool valid_chunk_extensions(std::string_view text) noexcept
    {
      std::size_t position = 0;
      while (position < text.size())
      {
        while (position < text.size() &&
               (text[position] == ' ' || text[position] == '\t'))
          ++position;
        if (position == text.size() || text[position++] != ';')
          return false;
        while (position < text.size() &&
               (text[position] == ' ' || text[position] == '\t'))
          ++position;
        const auto name_begin = position;
        while (position < text.size() && token_char(static_cast<unsigned char>(text[position])))
          ++position;
        if (position == name_begin)
          return false;
        while (position < text.size() &&
               (text[position] == ' ' || text[position] == '\t'))
          ++position;
        if (position < text.size() && text[position] == '=')
        {
          ++position;
          while (position < text.size() &&
                 (text[position] == ' ' || text[position] == '\t'))
            ++position;
          const auto value_begin = position;
          if (position < text.size() && text[position] == '"')
          {
            if (!valid_quoted(text, position))
              return false;
          }
          else
          {
            while (position < text.size() &&
                   token_char(static_cast<unsigned char>(text[position])))
              ++position;
            if (position == value_begin)
              return false;
          }
          while (position < text.size() &&
                 (text[position] == ' ' || text[position] == '\t'))
            ++position;
        }
      }
      return true;
    }

    bool forbidden_trailer(std::string_view name) noexcept
    {
      constexpr std::string_view names[]{
          "transfer-encoding", "content-length", "host", "trailer",
          "connection", "te", "upgrade", "authorization",
          "proxy-authorization", "content-encoding", "content-type",
          "content-range"};
      return std::any_of(std::begin(names), std::end(names),
                         [name](std::string_view forbidden)
                         { return equal_ascii_case_insensitive(name, forbidden); });
    }

    bool append_field(std::string &out, const HeaderField &field)
    {
      if (!valid_token(field.name) || !valid_field_value(field.value))
        return false;
      out.append(field.name);
      out.append(": ");
      out.append(field.value);
      out.append("\r\n");
      return true;
    }

    bool add_bounded(std::size_t &total, std::size_t amount,
                     std::size_t limit) noexcept
    {
      if (amount > limit - total)
        return false;
      total += amount;
      return true;
    }

    bool has_field(std::span<const HeaderField> fields, std::string_view wanted) noexcept
    {
      return std::any_of(fields.begin(), fields.end(),
                         [wanted](const HeaderField &field)
                         { return equal_ascii_case_insensitive(field.name, wanted); });
    }

    bool serialize_head(std::string_view start_line,
                        std::span<const HeaderField> fields,
                        OutgoingFraming framing,
                        bool request,
                        std::string &output)
    {
      constexpr std::size_t max_start_line = 8192;
      constexpr std::size_t max_header_bytes = 32768;
      constexpr std::size_t max_header_fields = 100;
      if (framing.kind == OutgoingFraming::Kind::no_body &&
          framing.content_length != 0)
        return false;
      if (framing.kind == OutgoingFraming::Kind::chunked &&
          framing.content_length != 0)
        return false;
      if (framing.kind != OutgoingFraming::Kind::no_body &&
          framing.kind != OutgoingFraming::Kind::fixed_length &&
          framing.kind != OutgoingFraming::Kind::chunked)
        return false;
      const auto framing_fields =
          framing.kind == OutgoingFraming::Kind::no_body ? 0U : 1U;
      if (start_line.empty() || start_line.size() > max_start_line ||
          fields.size() > max_header_fields - framing_fields)
        return false;
      if (has_field(fields, "content-length") || has_field(fields, "transfer-encoding"))
        return false;
      if (request && !has_field(fields, "host"))
        return false;
      if (request)
      {
        std::size_t host_count = 0;
        std::string_view host_value;
        for (const auto &field : fields)
          if (equal_ascii_case_insensitive(field.name, "host"))
          {
            ++host_count;
            host_value = field.value;
          }
        if (host_count != 1 || !valid_host_value(trim_ows(host_value)))
          return false;
      }

      std::size_t encoded_size = 0;
      if (!add_bounded(encoded_size, start_line.size(), max_header_bytes) ||
          !add_bounded(encoded_size, 2, max_header_bytes))
        return false;
      for (const auto &field : fields)
      {
        if (!valid_token(field.name) || !valid_field_value(field.value))
          return false;
        std::size_t field_size = 0;
        if (!add_bounded(field_size, field.name.size(), max_header_bytes) ||
            !add_bounded(field_size, 2, max_header_bytes) ||
            !add_bounded(field_size, field.value.size(), max_header_bytes) ||
            !add_bounded(field_size, 2, max_header_bytes) ||
            !add_bounded(encoded_size, field_size, max_header_bytes))
          return false;
      }
      if (framing.kind == OutgoingFraming::Kind::fixed_length)
      {
        const auto length_size = std::to_string(framing.content_length).size();
        if (!add_bounded(encoded_size, 16, max_header_bytes) ||
            !add_bounded(encoded_size, length_size, max_header_bytes) ||
            !add_bounded(encoded_size, 2, max_header_bytes))
          return false;
      }
      else if (framing.kind == OutgoingFraming::Kind::chunked &&
               !add_bounded(encoded_size, 28, max_header_bytes))
        return false;
      if (!add_bounded(encoded_size, 2, max_header_bytes))
        return false;

      std::string encoded;
      encoded.reserve(encoded_size);
      encoded.append(start_line);
      encoded.append("\r\n");
      for (const auto &field : fields)
      {
        if (!append_field(encoded, field))
          return false;
      }
      if (framing.kind == OutgoingFraming::Kind::fixed_length)
      {
        encoded.append("Content-Length: ");
        encoded.append(std::to_string(framing.content_length));
        encoded.append("\r\n");
      }
      else if (framing.kind == OutgoingFraming::Kind::chunked)
        encoded.append("Transfer-Encoding: chunked\r\n");
      encoded.append("\r\n");
      output.swap(encoded);
      return true;
    }
  } // namespace

  Http1Parser::Http1Parser(Limits limits) : limits_(limits)
  {
    const Limits defaults{};
    if (limits_.start_line_bytes == 0 || limits_.start_line_bytes > defaults.start_line_bytes ||
        limits_.header_bytes == 0 || limits_.header_bytes > defaults.header_bytes ||
        limits_.header_fields == 0 || limits_.header_fields > defaults.header_fields ||
        limits_.chunk_line_bytes == 0 || limits_.chunk_line_bytes > defaults.chunk_line_bytes ||
        limits_.chunk_metadata_bytes == 0 ||
        limits_.chunk_metadata_bytes > defaults.chunk_metadata_bytes ||
        limits_.trailer_bytes == 0 || limits_.trailer_bytes > defaults.trailer_bytes ||
        limits_.trailer_fields == 0 || limits_.trailer_fields > defaults.trailer_fields ||
        limits_.staged_bytes < limits_.start_line_bytes + 2 ||
        limits_.staged_bytes < limits_.header_bytes + 2 ||
        limits_.staged_bytes < limits_.chunk_line_bytes + 2 ||
        limits_.staged_bytes < limits_.trailer_bytes + 2 ||
        limits_.staged_bytes > defaults.staged_bytes)
    {
      limits_ = Limits{};
      failed_ = true;
      state_ = State::failed;
      error_ = ParseError::invalid_state;
    }
    input_.reserve(std::min<std::size_t>(limits_.staged_bytes, 65536));
  }

  std::size_t Http1Parser::buffered_bytes() const noexcept
  {
    return input_.size() - offset_;
  }

  void Http1Parser::compact() noexcept
  {
    if (offset_ == 0)
      return;
    if (offset_ == input_.size())
    {
      input_.clear();
      offset_ = 0;
      return;
    }
    input_.erase(input_.begin(), input_.begin() + static_cast<std::ptrdiff_t>(offset_));
    offset_ = 0;
  }

  std::size_t Http1Parser::append(std::span<const std::byte> bytes)
  {
    if (failed_ || state_ == State::tunnel)
      return 0;
    compact();
    const auto room = limits_.staged_bytes - input_.size();
    const auto accepted = std::min(room, bytes.size());
    input_.insert(input_.end(), bytes.begin(),
                  bytes.begin() + static_cast<std::ptrdiff_t>(accepted));
    return accepted;
  }

  void Http1Parser::consume(std::size_t count) noexcept
  {
    offset_ += count;
    if (offset_ == input_.size())
    {
      input_.clear();
      offset_ = 0;
    }
  }

  Http1Parser::LineResult Http1Parser::take_line(std::size_t maximum, bool eof)
  {
    LineResult result;
    const auto available = buffered_bytes();
    for (std::size_t i = 0; i < available; ++i)
    {
      const auto c = octet(input_[offset_ + i]);
      if (c == '\n')
      {
        if (i == 0 || octet(input_[offset_ + i - 1]) != '\r')
        {
          result.invalid = true;
          return result;
        }
        const auto length = i - 1;
        if (length > maximum)
        {
          result.invalid = true;
          return result;
        }
        result.ready = true;
        result.value.reserve(length);
        for (std::size_t j = 0; j < length; ++j)
          result.value.push_back(static_cast<char>(octet(input_[offset_ + j])));
        consume(i + 1);
        return result;
      }
      if (c == '\r' && i + 1 < available && octet(input_[offset_ + i + 1]) != '\n')
      {
        result.invalid = true;
        return result;
      }
      if (i > maximum)
      {
        result.invalid = true;
        return result;
      }
    }
    if (eof)
    {
      result.invalid = true;
      result.truncated = true;
    }
    return result;
  }

  ParseStep Http1Parser::fail(ParseError error) noexcept
  {
    failed_ = true;
    state_ = State::failed;
    error_ = error;
    return {ParseStatus::protocol_error, error, 0};
  }

  bool Http1Parser::parse_start_line(std::string_view line)
  {
    if (head_.kind == MessageKind::request)
    {
      const auto first = line.find(' ');
      const auto second = first == std::string_view::npos
                              ? first
                              : line.find(' ', first + 1);
      if (first == std::string_view::npos || second == std::string_view::npos ||
          line.find(' ', second + 1) != std::string_view::npos)
        return false;
      const auto method = line.substr(0, first);
      const auto target = line.substr(first + 1, second - first - 1);
      const auto version = line.substr(second + 1);
      if (!valid_token(method) || target.empty() ||
          (version != "HTTP/1.1" && version != "HTTP/1.0"))
        return false;
      for (std::size_t i = 0; i < target.size(); ++i)
      {
        const auto c = static_cast<unsigned char>(target[i]);
        if (c <= 0x20 || c >= 0x7f || c == '#')
          return false;
        if (c == '%')
        {
          if (i + 2 >= target.size() || hex_value(target[i + 1]) < 0 ||
              hex_value(target[i + 2]) < 0)
            return false;
          i += 2;
        }
      }
      if (!valid_request_target(target, method))
        return false;
      head_.method.assign(method);
      head_.target.assign(target);
      head_.version.assign(version);
      return true;
    }

    if (line.size() < 13 || (line.substr(0, 8) != "HTTP/1.1" &&
                             line.substr(0, 8) != "HTTP/1.0") ||
        line[8] != ' ' || line[9] < '0' || line[9] > '9' ||
        line[10] < '0' || line[10] > '9' || line[11] < '0' || line[11] > '9' ||
        line[12] != ' ')
      return false;
    if (!valid_field_value(line.substr(13)))
      return false;
    head_.version.assign(line.substr(0, 8));
    head_.status_code = static_cast<unsigned>(line[9] - '0') * 100U +
                        static_cast<unsigned>(line[10] - '0') * 10U +
                        static_cast<unsigned>(line[11] - '0');
    if (head_.status_code < 100 || head_.status_code > 599)
      return false;
    if (line.size() > 12)
      head_.reason.assign(line.substr(13));
    return true;
  }

  bool Http1Parser::parse_header(std::string_view line, bool trailer)
  {
    if (!line.empty() && (line.front() == ' ' || line.front() == '\t'))
    {
      auto &fields = trailer ? head_.trailers : head_.fields;
      if (!fields.empty() && head_.kind == MessageKind::response)
      {
        const auto continuation = trim_ows(line);
        if (!valid_field_value(continuation))
          return false;
        fields.back().value.push_back(' ');
        fields.back().value.append(continuation);
        return true;
      }
      error_ = ParseError::obsolete_line_folding;
      return false;
    }
    const auto colon = line.find(':');
    if (colon == std::string_view::npos)
      return false;
    const auto name = line.substr(0, colon);
    const auto value = trim_ows(line.substr(colon + 1));
    if (!valid_token(name) || !valid_field_value(value))
      return false;
    if (trailer && forbidden_trailer(name))
    {
      error_ = ParseError::forbidden_trailer;
      return false;
    }
    auto &fields = trailer ? head_.trailers : head_.fields;
    const auto maximum = trailer ? limits_.trailer_fields : limits_.header_fields;
    if (fields.size() >= maximum)
    {
      error_ = ParseError::too_many_headers;
      return false;
    }
    fields.push_back({std::string{name}, std::string{value}});
    return true;
  }

  bool Http1Parser::valid_host(std::string_view value) const noexcept
  {
    return valid_host_value(value);
  }

  std::string_view Http1Parser::field_values(std::string_view name,
                                              std::size_t &count) const noexcept
  {
    std::string_view first;
    count = 0;
    for (const auto &field : head_.fields)
      if (equal_ascii_case_insensitive(field.name, name))
      {
        if (count == 0)
          first = field.value;
        ++count;
      }
    return first;
  }

  bool Http1Parser::select_framing(std::string_view response_to_method)
  {
    std::size_t host_count = 0;
    const auto host = field_values("host", host_count);
    if (head_.kind == MessageKind::request && host_count > 1)
    {
      error_ = ParseError::invalid_host;
      return false;
    }
    if (head_.kind == MessageKind::request && host_count == 1 && !valid_host(host))
    {
      error_ = ParseError::invalid_host;
      return false;
    }
    if (head_.kind == MessageKind::request && head_.version == "HTTP/1.1")
    {
      if (host_count != 1)
      {
        error_ = ParseError::invalid_host;
        return false;
      }
    }

    std::size_t content_length_count = 0;
    std::size_t transfer_encoding_count = 0;
    (void)field_values("content-length", content_length_count);
    (void)field_values("transfer-encoding", transfer_encoding_count);
    if (head_.kind == MessageKind::response && response_to_method == "CONNECT" &&
        head_.status_code >= 200 && head_.status_code < 300)
    {
      // RFC 9112 section 6.3 makes the connection a tunnel after this header
      // section and requires the recipient to ignore both framing fields.
      head_.body_mode = BodyMode::tunnel;
      head_.tunnel_established = true;
      state_ = State::headers_reported;
      return true;
    }
    if (head_.kind == MessageKind::response && content_length_count != 0 &&
        ((head_.status_code >= 100 && head_.status_code < 200) ||
         head_.status_code == 204))
    {
      // RFC 9110 §8.6 forbids Content-Length on 1xx and 204 responses.
      // Reject even Content-Length: 0; these are semantic prohibitions, not
      // merely body-length constraints.
      error_ = ParseError::invalid_response_semantics;
      return false;
    }
    if (head_.version == "HTTP/1.0" && transfer_encoding_count != 0)
    {
      error_ = ParseError::unsupported_transfer_coding;
      return false;
    }
    if (head_.kind == MessageKind::response && transfer_encoding_count != 0 &&
        ((head_.status_code >= 100 && head_.status_code < 200) ||
         head_.status_code == 204))
    {
      // Such a Transfer-Encoding is forbidden by RFC 9112 section 6.1.
      error_ = ParseError::unsupported_transfer_coding;
      return false;
    }
    if (content_length_count != 0 && transfer_encoding_count != 0)
    {
      error_ = ParseError::conflicting_framing;
      return false;
    }

    std::uint64_t content_length = 0;
    bool has_length = false;
    for (const auto &field : head_.fields)
    {
      if (!equal_ascii_case_insensitive(field.name, "content-length"))
        continue;
      std::string_view values = field.value;
      while (true)
      {
        const auto comma = values.find(',');
        const auto item = trim_ows(values.substr(0, comma));
        std::uint64_t parsed = 0;
        if (!parse_decimal(item, parsed) || (has_length && parsed != content_length))
        {
          error_ = ParseError::invalid_content_length;
          return false;
        }
        if (!has_length)
        {
          content_length = parsed;
          has_length = true;
        }
        if (comma == std::string_view::npos)
          break;
        values.remove_prefix(comma + 1);
      }
    }

    std::size_t codings = 0;
    std::string_view coding_value;
    for (const auto &field : head_.fields)
    {
      if (!equal_ascii_case_insensitive(field.name, "transfer-encoding"))
        continue;
      std::string_view values = field.value;
      while (true)
      {
        const auto comma = values.find(',');
        const auto item = trim_ows(values.substr(0, comma));
        if (item.empty())
        {
          error_ = ParseError::unsupported_transfer_coding;
          return false;
        }
        ++codings;
        if (codings == 1)
          coding_value = item;
        else
        {
          error_ = ParseError::unsupported_transfer_coding;
          return false;
        }
        if (comma == std::string_view::npos)
          break;
        values.remove_prefix(comma + 1);
      }
    }

    if (head_.kind == MessageKind::response)
    {
      if (head_.status_code == 205 && has_length && content_length != 0)
      {
        error_ = ParseError::invalid_response_semantics;
        return false;
      }
      const bool head_response = response_to_method == "HEAD";
      const bool no_body_status = (head_.status_code >= 100 && head_.status_code < 200) ||
                                  head_.status_code == 204 || head_.status_code == 304;
      if (head_.status_code == 101)
      {
        head_.body_mode = BodyMode::tunnel;
        head_.tunnel_established = true;
        head_.informational = true;
        state_ = State::headers_reported;
        return true;
      }
      if (head_response || no_body_status)
      {
        head_.body_mode = BodyMode::none;
        head_.informational = head_.status_code >= 100 && head_.status_code < 200;
        state_ = State::headers_reported;
        return true;
      }
    }

    if (transfer_encoding_count != 0)
    {
      if (codings != 1 || !equal_ascii_case_insensitive(coding_value, "chunked"))
      {
        error_ = ParseError::unsupported_transfer_coding;
        return false;
      }
      head_.body_mode = BodyMode::chunked;
      state_ = State::headers_reported;
      return true;
    }
    if (has_length)
    {
      head_.content_length = content_length;
      head_.body_mode = content_length == 0 ? BodyMode::none : BodyMode::fixed_length;
      remaining_ = content_length;
      state_ = State::headers_reported;
      return true;
    }
    if (head_.kind == MessageKind::request)
    {
      head_.body_mode = BodyMode::none;
      state_ = State::headers_reported;
      return true;
    }
    head_.body_mode = BodyMode::close_delimited;
    state_ = State::headers_reported;
    return true;
  }

  bool Http1Parser::consume_chunk_size(std::string_view line)
  {
    const auto semicolon = line.find(';');
    const auto digits = line.substr(0, semicolon);
    if (digits.empty())
      return false;
    std::uint64_t size = 0;
    for (const char ch : digits)
    {
      const int value = hex_value(ch);
      if (value < 0 || size > (std::numeric_limits<std::uint64_t>::max() -
                               static_cast<std::uint64_t>(value)) / 16)
        return false;
      size = size * 16 + static_cast<std::uint64_t>(value);
    }
    if (semicolon != std::string_view::npos &&
        !valid_chunk_extensions(line.substr(semicolon)))
      return false;
    if (head_.kind == MessageKind::response && head_.status_code == 205 && size != 0)
    {
      error_ = ParseError::invalid_response_semantics;
      return false;
    }
    chunk_remaining_ = size;
    state_ = size == 0 ? State::trailers : State::chunk_data;
    return true;
  }

  ParseStep Http1Parser::next(MessageKind kind, std::string_view response_to_method,
                              std::span<std::byte> body_output, bool end_of_input)
  {
    if (failed_)
      return {ParseStatus::protocol_error, error_, 0};
    if (state_ == State::complete || state_ == State::tunnel)
      return {ParseStatus::message_complete, ParseError::none, 0};
    if (state_ == State::start_line)
      head_.kind = kind;

    for (;;)
    {
      if (state_ == State::start_line)
      {
        auto line = take_line(limits_.start_line_bytes, end_of_input);
        if (line.invalid)
          return fail(line.truncated ? ParseError::truncated_message :
                      buffered_bytes() > limits_.start_line_bytes
                          ? ParseError::line_too_large
                          : ParseError::malformed_start_line);
        if (!line.ready)
          return {ParseStatus::need_more, ParseError::none, 0};
        if (line.value.empty() && kind == MessageKind::request &&
            !leading_empty_line_seen_)
        {
          leading_empty_line_seen_ = true;
          continue;
        }
        if (!parse_start_line(line.value))
          return fail(ParseError::malformed_start_line);
        state_ = State::headers;
        continue;
      }
      if (state_ == State::headers)
      {
        auto line = take_line(limits_.header_bytes, end_of_input);
        if (line.invalid)
          return fail(line.truncated ? ParseError::truncated_message :
                      buffered_bytes() > limits_.header_bytes
                          ? ParseError::headers_too_large
                          : ParseError::malformed_header);
        if (!line.ready)
          return {ParseStatus::need_more, ParseError::none, 0};
        header_bytes_ += line.value.size() + 2;
        if (header_bytes_ > limits_.header_bytes)
          return fail(ParseError::headers_too_large);
        if (line.value.empty())
        {
          if (!select_framing(response_to_method))
            return fail(error_ == ParseError::none ? ParseError::malformed_header : error_);
          return {ParseStatus::headers_complete, ParseError::none, 0};
        }
        if (!parse_header(line.value, false))
          return fail(error_ == ParseError::none ? ParseError::malformed_header : error_);
        continue;
      }
      if (state_ == State::headers_reported)
      {
        if (head_.body_mode == BodyMode::none)
          state_ = State::complete;
        else if (head_.body_mode == BodyMode::fixed_length)
          state_ = State::fixed_body;
        else if (head_.body_mode == BodyMode::chunked)
          state_ = State::chunk_size;
        else if (head_.body_mode == BodyMode::close_delimited)
          state_ = State::close_body;
        else if (head_.body_mode == BodyMode::tunnel)
          state_ = State::tunnel;
        continue;
      }
      if (state_ == State::fixed_body || state_ == State::chunk_data ||
          state_ == State::close_body)
      {
        if (head_.kind == MessageKind::response && head_.status_code == 205 &&
            buffered_bytes() != 0)
          return fail(ParseError::invalid_response_semantics);
        if (buffered_bytes() != 0 && !body_output.empty())
        {
          auto count = std::min(buffered_bytes(), body_output.size());
          if (state_ == State::fixed_body)
            count = static_cast<std::size_t>(std::min<std::uint64_t>(count, remaining_));
          else if (state_ == State::chunk_data)
            count = static_cast<std::size_t>(std::min<std::uint64_t>(count, chunk_remaining_));
          for (std::size_t i = 0; i < count; ++i)
            body_output[i] = input_[offset_ + i];
          consume(count);
          if (state_ == State::fixed_body)
          {
            remaining_ -= count;
            if (remaining_ == 0)
              state_ = State::complete;
          }
          else if (state_ == State::chunk_data)
          {
            chunk_remaining_ -= count;
            if (chunk_remaining_ == 0)
              state_ = State::chunk_data_crlf;
          }
          return {ParseStatus::body_data, ParseError::none, count};
        }
        if (state_ == State::close_body && end_of_input && buffered_bytes() == 0)
        {
          state_ = State::complete;
          continue;
        }
        if (end_of_input && state_ != State::close_body && buffered_bytes() == 0)
          return fail(ParseError::truncated_message);
        return {ParseStatus::need_more, ParseError::none, 0};
      }
      if (state_ == State::chunk_size)
      {
        auto line = take_line(limits_.chunk_line_bytes, end_of_input);
        if (line.invalid)
          return fail(line.truncated ? ParseError::truncated_message :
                      buffered_bytes() > limits_.chunk_line_bytes
                          ? ParseError::line_too_large
                          : ParseError::malformed_chunk);
        if (!line.ready)
          return {ParseStatus::need_more, ParseError::none, 0};
        if (line.value.size() + 2 > limits_.chunk_metadata_bytes - chunk_metadata_bytes_)
          return fail(ParseError::line_too_large);
        chunk_metadata_bytes_ += line.value.size() + 2;
        if (!consume_chunk_size(line.value))
          return fail(error_ == ParseError::none ? ParseError::malformed_chunk : error_);
        continue;
      }
      if (state_ == State::chunk_data_crlf)
      {
        if (buffered_bytes() < 2)
        {
          if (end_of_input)
            return fail(ParseError::truncated_message);
          return {ParseStatus::need_more, ParseError::none, 0};
        }
        if (octet(input_[offset_]) != '\r' || octet(input_[offset_ + 1]) != '\n')
          return fail(ParseError::malformed_chunk);
        if (chunk_metadata_bytes_ > limits_.chunk_metadata_bytes - 2)
          return fail(ParseError::line_too_large);
        chunk_metadata_bytes_ += 2;
        consume(2);
        state_ = State::chunk_size;
        continue;
      }
      if (state_ == State::trailers)
      {
        auto line = take_line(limits_.trailer_bytes, end_of_input);
        if (line.invalid)
          return fail(line.truncated ? ParseError::truncated_message :
                      buffered_bytes() > limits_.trailer_bytes
                          ? ParseError::headers_too_large
                          : ParseError::malformed_header);
        if (!line.ready)
          return {ParseStatus::need_more, ParseError::none, 0};
        trailer_bytes_ += line.value.size() + 2;
        if (trailer_bytes_ > limits_.trailer_bytes)
          return fail(ParseError::headers_too_large);
        if (line.value.empty())
        {
          state_ = State::complete;
          continue;
        }
        if (!parse_header(line.value, true))
          return fail(error_ == ParseError::none ? ParseError::malformed_header : error_);
        if (head_.trailers.size() > limits_.trailer_fields)
          return fail(ParseError::too_many_headers);
        continue;
      }
      if (state_ == State::complete || state_ == State::tunnel)
        return {ParseStatus::message_complete, ParseError::none, 0};
      return fail(ParseError::invalid_state);
    }
  }

  bool Http1Parser::reset_message() noexcept
  {
    if (state_ != State::complete || head_.tunnel_established)
      return false;
    head_ = MessageHead{};
    state_ = State::start_line;
    error_ = ParseError::none;
    header_bytes_ = 0;
    trailer_bytes_ = 0;
    remaining_ = 0;
    chunk_remaining_ = 0;
    chunk_metadata_bytes_ = 0;
    leading_empty_line_seen_ = false;
    failed_ = false;
    return true;
  }

  bool serialize_request_head(std::string_view method, std::string_view target,
                              std::span<const HeaderField> fields,
                              OutgoingFraming framing, std::string &output)
  {
    if (!valid_token(method) || target.empty() ||
        !valid_request_target(target, method))
      return false;
    // Absolute-form requests must identify the same authority in Host. This
    // private serializer intentionally rejects userinfo and non-HTTP schemes
    // rather than guessing at equivalence rules.
    const auto scheme_end = target.find(':');
    if (scheme_end != std::string_view::npos &&
        target.substr(scheme_end + 1, 2) == "//")
    {
      const auto scheme = target.substr(0, scheme_end);
      if (!equal_ascii_case_insensitive(scheme, "http") &&
          !equal_ascii_case_insensitive(scheme, "https"))
        return false;
      const auto authority_begin = scheme_end + 3;
      const auto authority_end = target.find_first_of("/?", authority_begin);
      const auto authority = target.substr(
          authority_begin,
          (authority_end == std::string_view::npos ? target.size() : authority_end) -
              authority_begin);
      if (authority.find('@') != std::string_view::npos)
        return false;

      std::string_view host_value;
      std::size_t host_count = 0;
      for (const auto &field : fields)
        if (equal_ascii_case_insensitive(field.name, "host"))
        {
          host_value = trim_ows(field.value);
          ++host_count;
        }
      const auto split_authority = [](std::string_view value,
                                      std::string_view &host,
                                      std::string_view &port) noexcept
      {
        host = value;
        port = {};
        if (value.empty())
          return false;
        if (value.front() == '[')
        {
          const auto close = value.find(']');
          if (close == std::string_view::npos)
            return false;
          host = value.substr(0, close + 1);
          if (close + 1 < value.size())
          {
            if (value[close + 1] != ':')
              return false;
            port = value.substr(close + 2);
          }
          return true;
        }
        const auto colon = value.find(':');
        if (colon != std::string_view::npos)
        {
          if (value.find(':', colon + 1) != std::string_view::npos)
            return false;
          host = value.substr(0, colon);
          port = value.substr(colon + 1);
        }
        return !host.empty();
      };
      const auto equal_authority_host = [](std::string_view left,
                                           std::string_view right) noexcept
      {
        // Percent escapes and IPv6 zone identifiers are deliberately compared
        // byte-for-byte; this avoids equating distinct escaped/zone values.
        if (left.find('%') != std::string_view::npos ||
            right.find('%') != std::string_view::npos)
          return left == right;
        return equal_ascii_case_insensitive(left, right);
      };
      std::string_view target_host;
      std::string_view target_port;
      std::string_view header_host;
      std::string_view header_port;
      if (host_count != 1 || !split_authority(authority, target_host, target_port) ||
          !split_authority(host_value, header_host, header_port) ||
          !equal_authority_host(target_host, header_host))
        return false;
      if (target_port.empty() != header_port.empty())
        return false;
      if (!target_port.empty())
      {
        std::uint64_t target_number = 0;
        std::uint64_t header_number = 0;
        if (!parse_decimal(target_port, target_number) ||
            !parse_decimal(header_port, header_number) ||
            target_number != header_number)
          return false;
      }
    }
    constexpr std::size_t max_start_line = 8192;
    std::size_t start_size = 0;
    if (!add_bounded(start_size, method.size(), max_start_line) ||
        !add_bounded(start_size, 1, max_start_line) ||
        !add_bounded(start_size, target.size(), max_start_line) ||
        !add_bounded(start_size, 9, max_start_line))
      return false;
    std::string start;
    start.reserve(start_size);
    start.append(method);
    start.push_back(' ');
    start.append(target);
    start.append(" HTTP/1.1");
    return serialize_head(start, fields, framing, true, output);
  }

  bool validated_request_method(std::span<const std::byte> serialized_request,
                                std::string &method)
  {
    if (serialized_request.empty() || serialized_request.size() > 8192)
      return false;
    Http1Parser parser;
    if (parser.append(serialized_request) != serialized_request.size())
      return false;
    std::array<std::byte, 8192> discarded_body{};
    bool captured = false;
    for (std::size_t turn = 0; turn < serialized_request.size() + 4; ++turn)
    {
      const auto step = parser.next(MessageKind::request, {}, discarded_body, true);
      if (step.status == ParseStatus::headers_complete)
      {
        method = parser.head().method;
        captured = true;
        continue;
      }
      if (step.status == ParseStatus::body_data)
        continue;
      if (step.status == ParseStatus::message_complete)
        return captured && parser.buffered_bytes() == 0;
      return false;
    }
    return false;
  }

  bool serialize_response_head(unsigned status, std::string_view reason,
                               std::span<const HeaderField> fields,
                               OutgoingFraming framing, std::string &output)
  {
    if (status < 100 || status > 599 || !valid_field_value(reason))
      return false;
    constexpr std::size_t max_start_line = 8192;
    constexpr std::size_t response_prefix_size = 13;
    if (reason.size() > max_start_line - response_prefix_size)
      return false;
    if (((status >= 100 && status < 200) || status == 204) &&
        framing.kind != OutgoingFraming::Kind::no_body)
      return false;
    if (status == 304 && framing.kind != OutgoingFraming::Kind::no_body)
      return false;
    if (status == 205 &&
        (framing.kind == OutgoingFraming::Kind::chunked ||
         (framing.kind == OutgoingFraming::Kind::fixed_length &&
          framing.content_length != 0)))
      return false;
    std::string start{"HTTP/1.1 "};
    start.push_back(static_cast<char>('0' + (status / 100) % 10));
    start.push_back(static_cast<char>('0' + (status / 10) % 10));
    start.push_back(static_cast<char>('0' + status % 10));
    start.push_back(' ');
    start.append(reason);
    return serialize_head(start, fields, framing, false, output);
  }

  bool serialize_chunk(std::span<const std::byte> body,
                       std::span<const HeaderField> trailers,
                       bool final, std::string &output)
  {
    constexpr std::size_t max_chunk_bytes = 16384;
    constexpr std::size_t max_trailer_bytes = 8192;
    constexpr std::size_t max_trailer_fields = 32;
    if (body.size() > max_chunk_bytes || trailers.size() > max_trailer_fields ||
        (!final && !trailers.empty()))
      return false;
    if (!final && body.empty())
    {
      output.clear();
      return true;
    }
    std::string encoded;
    if (!body.empty())
    {
      char digits[2 * sizeof(std::size_t) + 1]{};
      const auto conversion = std::to_chars(digits, digits + sizeof(digits), body.size(), 16);
      if (conversion.ec != std::errc{})
        return false;
      encoded.append(digits, conversion.ptr);
      encoded.append("\r\n");
      for (const auto byte : body)
        encoded.push_back(static_cast<char>(octet(byte)));
      encoded.append("\r\n");
    }
    if (final)
    {
      std::size_t trailer_bytes = 0;
      for (const auto &field : trailers)
      {
        if (!valid_token(field.name) || !valid_field_value(field.value) ||
            forbidden_trailer(field.name))
          return false;
        std::size_t field_size = 0;
        if (!add_bounded(field_size, field.name.size(), max_trailer_bytes) ||
            !add_bounded(field_size, 2, max_trailer_bytes) ||
            !add_bounded(field_size, field.value.size(), max_trailer_bytes) ||
            !add_bounded(field_size, 2, max_trailer_bytes) ||
            !add_bounded(trailer_bytes, field_size, max_trailer_bytes))
          return false;
      }
      if (trailer_bytes > max_trailer_bytes - 5)
        return false;
      encoded.append("0\r\n");
      for (const auto &field : trailers)
      {
        if (!append_field(encoded, field))
          return false;
      }
      encoded.append("\r\n");
    }
    output.swap(encoded);
    return true;
  }
} // namespace vix::http::detail
