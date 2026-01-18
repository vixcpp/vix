# Example — error_handling.cpp

```cpp
#include <vix/orm/orm.hpp>
#include <iostream>

using namespace vix::orm;

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    try
    {
        // Intentionally wrong DB name to show error
        auto raw = make_mysql_conn("tcp://127.0.0.1:3306", "root", "", "db_does_not_exist");
        std::cout << "[INFO] This message may not be reached if connection fails.\n";
    }
    catch (const DBError &e)
    {
        std::cerr << "[DBError] " << e.what() << "\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "[std::exception] " << e.what() << "\n";
    }
    return 0;
}


```
