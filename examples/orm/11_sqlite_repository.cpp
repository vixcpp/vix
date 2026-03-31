#include <cstdint>
#include <iostream>
#include <string>

#include <vix/orm/orm.hpp>

struct Note
{
  std::int64_t id{};
  std::string title;
};

template <>
struct vix::orm::Mapper<Note>
{
  static Note fromRow(const vix::db::ResultRow &row)
  {
    return Note{
        row.getInt64Or(0, 0),
        row.getStringOr(1, ""),
    };
  }

  static vix::orm::FieldValues toInsertFields(const Note &n)
  {
    return {{"title", n.title}};
  }

  static vix::orm::FieldValues toUpdateFields(const Note &n)
  {
    return {{"title", n.title}};
  }
};

int main()
{
  try
  {
    auto db = vix::db::Database::sqlite("orm_sqlite.db");

    {
      auto conn = db.pool().acquire();
      conn->prepare(
              "CREATE TABLE IF NOT EXISTS notes ("
              "id INTEGER PRIMARY KEY AUTOINCREMENT, "
              "title TEXT NOT NULL)")
          ->exec();
    }

    auto repo = vix::orm::repository<Note>(db, "notes");
    repo.create(Note{0, "Hello SQLite ORM"});

    for (const auto &n : repo.findAll())
    {
      std::cout << n.id << " " << n.title << "\n";
    }

    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "[ERR] " << e.what() << "\n";
    return 1;
  }
}
