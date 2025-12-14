# ORM Examples — Vix.cpp

This section collects runnable examples for the ORM module.

| Example File                                           | Description                               |
| ------------------------------------------------------ | ----------------------------------------- |
| [post_create_user.md](../examples/post_create_user.md) | Create a new user (INSERT)                |
| [put_update_user.md](../examples/put_update_user.md)   | Update existing user data                 |
| [delete_user.md](../examples/delete_user.md)           | Delete a user by ID                       |
| [repository_crud_full.md](./repository_crud_full.md)   | Full CRUD via Repository pattern          |
| [batch_insert_tx.md](./batch_insert_tx.md)             | Transaction + Batch inserts               |
| [migrate_init.md](./migrate_init.md)                   | Initialize migrations and version table   |
| [querybuilder_update.md](./querybuilder_update.md)     | Use QueryBuilder for custom UPDATE        |
| [error_handling.md](./error_handling.md)               | Catch and handle DBError exceptions       |
| [users_crud.md](./users_crud.md)                       | Complete users table demo with Repository |

---

## Quick Build

```bash
cmake -S modules/orm -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Then run any example binary in `build/orm`.

---

## See also

- [Overview](./overview.md)
- [Main documentation index](../README.md)
