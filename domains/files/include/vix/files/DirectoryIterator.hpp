#ifndef VIX_FILES_DIRECTORY_ITERATOR_HPP
#define VIX_FILES_DIRECTORY_ITERATOR_HPP

#include <memory>
#include <optional>

#include <vix/Result.hpp>

#include <vix/files/FileInfo.hpp>

namespace vix::files
{
  class DirectoryIterator
  {
  public:
    DirectoryIterator() = delete;
    DirectoryIterator(const DirectoryIterator &) = delete;
    DirectoryIterator &operator=(const DirectoryIterator &) = delete;
    DirectoryIterator(DirectoryIterator &&) noexcept;
    DirectoryIterator &operator=(DirectoryIterator &&) = delete;
    ~DirectoryIterator();

    [[nodiscard]] bool is_open() const noexcept;
    [[nodiscard]] vix::Result<std::optional<DirectoryEntry>> next();
    [[nodiscard]] vix::Result<void> close();

  private:
    struct State;
    explicit DirectoryIterator(std::unique_ptr<State>) noexcept;
    std::unique_ptr<State> state_;
    friend vix::Result<DirectoryIterator> iterate(const Path &);
  };
} // namespace vix::files

#endif
