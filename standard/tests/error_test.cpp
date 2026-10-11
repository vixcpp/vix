#include <vix/Error.hpp>

#include <cassert>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace
{
  inline constexpr vix::ErrorIdentity os_access_denied =
      vix::ErrorIdentity::define<"vix.os">(1);
  inline constexpr vix::ErrorIdentity files_open_failed =
      vix::ErrorIdentity::define<"vix.files">(1);
  inline constexpr vix::ErrorIdentity config_load_failed =
      vix::ErrorIdentity::define<"vix.config">(1);
  inline constexpr vix::ErrorIdentity process_launch_failed =
      vix::ErrorIdentity::define<"vix.process">(1);
  inline constexpr vix::ErrorIdentity dns_resolution_failed =
      vix::ErrorIdentity::define<"vix.network">(1);
  inline constexpr vix::ErrorIdentity http_request_failed =
      vix::ErrorIdentity::define<"vix.http">(1);
  inline constexpr vix::ErrorIdentity database_query_failed =
      vix::ErrorIdentity::define<"vix.database">(1);

  vix::Error translate_provider_failure()
  {
    vix::Error provider_error{
        os_access_denied,
        "EACCES"};

    return vix::Error{
               files_open_failed,
               "unable to open configuration file",
               std::move(provider_error)}
        .with_context({"path", "/etc/vix/settings.toml"});
  }

  vix::Error translate_domain_failure()
  {
    auto file_error = translate_provider_failure();

    return vix::Error{
               config_load_failed,
               "application configuration could not be loaded",
               std::move(file_error)}
        .with_context({"configuration", "primary"});
  }

  void test_translation_preserves_diagnostic_chain()
  {
    const auto error = translate_domain_failure();

    assert(error.identity() == config_load_failed);
    assert(error.message() == "application configuration could not be loaded");
    assert(error.contexts().size() == 1);
    assert(error.contexts()[0].key == "configuration");

    const vix::Error *file_error = error.cause();
    assert(file_error != nullptr);
    assert(file_error->identity() == files_open_failed);
    assert(file_error->contexts().size() == 1);
    assert(file_error->contexts()[0].key == "path");

    const vix::Error *provider_error = file_error->cause();
    assert(provider_error != nullptr);
    assert(provider_error->identity() == os_access_denied);
    assert(provider_error->message() == "EACCES");
    assert(provider_error->cause() == nullptr);
  }

  void test_context_does_not_change_machine_identity()
  {
    const vix::Error original{files_open_failed, "unable to open file"};
    const auto enriched = original.with_context({"path", "journal.log"});

    assert(original.identity() == files_open_failed);
    assert(original.contexts().empty());
    assert(enriched.identity() == files_open_failed);
    assert(enriched.contexts().size() == 1);
  }

  void test_independent_domain_translations_retain_root_diagnostics()
  {
    const vix::Error process_error{
        process_launch_failed,
        "execve returned ENOENT"};

    const vix::Error database_error{
        database_query_failed,
        "migration command could not run",
        process_error};
    assert(database_error.identity() == database_query_failed);
    assert(database_error.cause()->identity() == process_launch_failed);

    const vix::Error provider_error{
        os_access_denied,
        "getaddrinfo returned EAI_AGAIN"};
    const auto dns_error = vix::Error{
                               dns_resolution_failed,
                               "database host could not be resolved",
                               provider_error}
                               .with_context({"host", "db.internal"});
    const auto http_error = vix::Error{
                                http_request_failed,
                                "catalog request could not complete",
                                dns_error}
                                .with_context({"request_id", "7f3"});

    assert(http_error.identity() == http_request_failed);
    assert(http_error.contexts()[0].key == "request_id");
    assert(http_error.cause()->identity() == dns_resolution_failed);
    assert(http_error.cause()->contexts()[0].value == "db.internal");
    assert(http_error.cause()->cause()->identity() == os_access_denied);
  }

  void test_copied_errors_are_safe_for_concurrent_observation()
  {
    const auto error = translate_domain_failure();
    std::vector<std::thread> readers;

    for (int index = 0; index < 4; ++index)
    {
      readers.emplace_back([error]
                           {
                             for (int attempt = 0; attempt < 1'000; ++attempt)
                             {
                               assert(error.identity() == config_load_failed);
                               assert(error.cause() != nullptr);
                               assert(error.contexts().size() == 1);
                             }
                           });
    }

    for (auto &reader : readers)
    {
      reader.join();
    }
  }
} // namespace

int main()
{
  test_translation_preserves_diagnostic_chain();
  test_context_does_not_change_machine_identity();
  test_independent_domain_translations_retain_root_diagnostics();
  test_copied_errors_are_safe_for_concurrent_observation();

  return 0;
}
