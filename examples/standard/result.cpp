
/**
 * @file result.cpp
 * @brief Demonstrates Vix Result<T>, Result<void>, and error propagation.
 *
 * Build: g++ -std=c++20 -I standard/include examples/standard/result.cpp -o result-example
 */

#include <vix/Result.hpp>
#include <iostream>

namespace example
{
  // Error identities belong to a domain and have stable, nonzero codes.
  inline constexpr auto invalid_quantity = vix::ErrorIdentity::define<"example.order">(1);
  inline constexpr auto insufficient_stock = vix::ErrorIdentity::define<"example.order">(2);

  // Result<int> holds either a successful integer or an Error.
  [[nodiscard]] vix::Result<int> validate_quantity(int quantity)
  {
    if (quantity <= 0)
      return vix::Error{invalid_quantity, "quantity must be positive"};

    return quantity;
  }

  // Result<void> represents success without a value, or an Error.
  [[nodiscard]] vix::Result<void> check_stock(int quantity, int available)
  {
    if (quantity > available)
      return vix::Error{insufficient_stock, "not enough items in stock"};

    return vix::Result<void>::success();
  }

  // Propagate an existing error without changing its identity.
  [[nodiscard]] vix::Result<int> place_order(int requested, int available)
  {
    auto quantity = validate_quantity(requested);
    if (!quantity)
      return quantity.error();

    auto stock = check_stock(quantity.value(), available);
    if (!stock)
      return stock.error();

    // take() transfers the successful value exactly once.
    return quantity.take();
  }
} // namespace example

int main()
{
  constexpr int available = 5;

  for (int requested : {3, 10, 0})
  {
    auto result = example::place_order(requested, available);

    if (!result)
    {
      const auto &error = result.error();
      std::cout << "Rejected " << requested << ": " << error.message() << '\n';

      // Match the identity, not the diagnostic message.
      if (error.identity() == example::insufficient_stock)
        std::cout << "  Try ordering fewer items.\n";

      continue;
    }

    const int accepted = result.take();
    std::cout << "Accepted " << accepted << " items.\n";
  }
}
