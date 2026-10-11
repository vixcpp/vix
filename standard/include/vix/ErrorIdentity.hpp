#ifndef VIX_ERROR_IDENTITY_HPP
#define VIX_ERROR_IDENTITY_HPP

#include <cstddef>
#include <cstdint>
#include <exception>
#include <string_view>

namespace vix::detail
{
  // A structural literal wrapper permits ErrorIdentity::define<"domain">()
  // while ensuring that the stored domain belongs to a template parameter
  // object with static storage duration.
  template <std::size_t N>
  struct static_domain_literal
  {
    char value[N]{};

    consteval static_domain_literal(const char (&literal)[N])
    {
      for (std::size_t index = 0; index < N; ++index)
      {
        value[index] = literal[index];
      }
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept
    {
      return N;
    }
  };
} // namespace vix::detail

namespace vix
{
  /**
   * @brief Stable programmatic identity for an operational failure.
   *
   * An identity is a domain key and a non-zero code local to that domain.
   * It intentionally has no success state. Domains define identities as
   * constexpr values through define<"domain">(code); published identities
   * must retain their meaning and must not be repurposed.
   */
  class ErrorIdentity
  {
  public:
    ErrorIdentity() = delete;

    /**
     * @brief Define an identity from a static domain literal and a local code.
     *
     * The immediate function rejects an empty domain and code zero during
     * constant evaluation. The domain literal is retained through a template
     * parameter object and therefore has static program lifetime.
     */
    template <detail::static_domain_literal Domain>
    [[nodiscard]] static consteval ErrorIdentity define(std::uint32_t code)
    {
      static_assert(Domain.size() > 1,
                    "ErrorIdentity domain must not be empty.");

      if (code == 0)
      {
        // A call to this non-constexpr terminal function makes the invalid
        // consteval expression ill-formed without requiring language
        // exceptions in otherwise valid exception-disabled builds.
        std::terminate();
      }

      return ErrorIdentity(
          std::string_view{Domain.value, Domain.size() - 1},
          code,
          validated_tag{});
    }

    [[nodiscard]] constexpr std::string_view domain() const noexcept
    {
      return domain_;
    }

    [[nodiscard]] constexpr std::uint32_t code() const noexcept
    {
      return code_;
    }

    friend constexpr bool operator==(ErrorIdentity,
                                     ErrorIdentity) noexcept = default;

  private:
    struct validated_tag
    {
    };

    constexpr ErrorIdentity(std::string_view domain,
                            std::uint32_t code,
                            validated_tag) noexcept
        : domain_(domain),
          code_(code)
    {
    }

    std::string_view domain_;
    std::uint32_t code_;
  };
} // namespace vix

#endif // VIX_ERROR_IDENTITY_HPP
