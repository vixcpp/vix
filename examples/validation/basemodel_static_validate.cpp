#include <iostream>
#include <string>

#include <vix/validation/BaseModel.hpp>

struct ProductInput : vix::validation::BaseModel<ProductInput>
{
  std::string title;
  std::string currency;

  static vix::validation::Schema<ProductInput> schema()
  {
    return vix::validation::schema<ProductInput>()
        .field("title", &ProductInput::title,
               vix::validation::field<std::string>()
                   .required()
                   .length_min(3)
                   .length_max(80))
        .field("currency", &ProductInput::currency,
               vix::validation::field<std::string>()
                   .required()
                   .in_set({"USD", "EUR", "UGX"}, "currency must be USD/EUR/UGX"));
  }
};

int main()
{
  ProductInput p;
  p.title = "TV";
  p.currency = "BTC";

  // Static validation (no need to call p.validate())
  auto r = ProductInput::validate(p);

  std::cout << "ok=" << static_cast<bool>(r.ok()) << "\n";

  // Cached schema access
  const auto &sc = ProductInput::schema();
  (void)sc;

  if (!r.ok())
  {
    for (const auto &e : r.errors.all())
    {
      std::cout << " - field=" << e.field
                << " code=" << vix::validation::to_string(e.code)
                << " message=" << e.message << "\n";
    }
    return 1;
  }

  std::cout << "valid product input\n";
  return 0;
}
