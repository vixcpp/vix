#include <vix/requests/requests.hpp>
#include "example_env.hpp"

#include <iostream>
#include <string>
#include <utility>

namespace
{

  class ApiClient
  {
  public:
    ApiClient(std::string baseUrl, std::string token)
        : baseUrl_(std::move(baseUrl))
    {
      session_.headers().set("Accept", "application/json");
      session_.headers().set("User-Agent", "vix-api-client-example/1.0");
      session_.timeout() = std::chrono::seconds(10);

      if (!token.empty())
      {
        session_.headers().set("Authorization", "Bearer " + token);
      }
    }

    vix::requests::Response list_items(int page)
    {
      vix::requests::RequestOptions options;
      options.params.set("page", std::to_string(page));
      options.params.set("per_page", "20");
      return checked(session_.get(baseUrl_ + "/get", options));
    }

    vix::requests::Response create_item(std::string name)
    {
      const std::string body = R"({"name":")" + name + R"(","enabled":true})";
      return checked(session_.post(baseUrl_ + "/post", vix::requests::json_body(body)));
    }

  private:
    vix::requests::Response checked(vix::requests::Response response)
    {
      response.raise_for_status();
      return response;
    }

    std::string baseUrl_;
    vix::requests::Session session_;
  };
}

int main()
{
  ApiClient api(
      vix_examples::requests::env_or("VIX_REQUESTS_BASE_URL", "https://httpbin.org"),
      vix_examples::requests::env_or("VIX_REQUESTS_TOKEN", ""));

  const auto list = api.list_items(1);
  std::cout << "list status: " << list.status_code() << '\n';

  const auto created = api.create_item("Vix item");
  std::cout << "create status: " << created.status_code() << '\n';
  std::cout << created.text() << '\n';
}
