/**
 * @file ErrorIdentity.hpp
 * @author Gaspard Kirira
 *
 * Copyright 2026, Gaspard Kirira.
 * All rights reserved.
 * https://github.com/vixcpp/vix
 *
 * Use of this source code is governed by an MIT license
 * that can be found in the LICENSE file.
 *
 * Vix.cpp
 */

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
   * @brief Stable identity for an operational error.
   *
   * @details
   * ErrorIdentity identifies an error using two values:
   * - A domain string identifying the error namespace.
   * - A non-zero numeric code unique within that domain.
   *
   * Applications can compare identities without relying on
   * human-readable error messages.
   *
   * Identities are defined at compile time using define().
   *
   * @par Example
   * @code{.cpp}
   * inline constexpr auto not_found =
   *     vix::ErrorIdentity::define<"example.files">(1);
   *
   * static_assert(not_found.domain() == "example.files");
   * static_assert(not_found.code() == 1);
   * @endcode
   *
   * @par Stability
   * Once an error identity is published, its domain, code
   * and meaning must remain stable.
   *
   * @note
   * ErrorIdentity represents errors only. It does not
   * contain a success state.
   */
  class ErrorIdentity
  {
  public:
    /**
     * @brief Default construction is not supported.
     *
     * Use define() to create an error identity.
     */
    ErrorIdentity() = delete;

    /**
     * @brief Defines an error identity at compile time.
     *
     * @tparam Domain Non-empty static string literal
     *                identifying the error domain.
     *
     * @param code Non-zero error code within the domain.
     *
     * @return An ErrorIdentity containing the specified
     *         domain and code.
     *
     * @par Example
     * @code{.cpp}
     * constexpr auto invalid_argument =
     *     vix::ErrorIdentity::define<"example">(2);
     * @endcode
     *
     * @note
     * An empty domain or a zero code makes the
     * definition invalid at compile time.
     *
     * The stored domain remains valid for the
     * lifetime of the program.
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

    /**
     * @brief Returns the error domain.
     *
     * @return A string view identifying the domain.
     *
     * @note
     * The returned view refers to static storage and
     * does not require ownership or manual cleanup.
     */
    [[nodiscard]] constexpr std::string_view domain() const noexcept
    {
      return domain_;
    }

    /**
     * @brief Returns the error code.
     *
     * @return The non-zero code within the domain.
     *
     * @note
     * Codes are meaningful within their domain.
     * Different domains may use the same numeric code.
     */
    [[nodiscard]] constexpr std::uint32_t code() const noexcept
    {
      return code_;
    }

    /**
     * @brief Compares two error identities.
     *
     * @return true if both identities have the same
     *         domain and code, false otherwise.
     */
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
