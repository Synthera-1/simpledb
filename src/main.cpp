#include <iostream>
#include <string>
#include "database.h"
#include "result.h"

// Main REPL loop (aka “type SQL, get results”).
// Type EXIT to quit.
// Multi-line queries work because we wait for a semicolon.
int main() {

    Database db;

    // yellow ascii art title. looks sick.
    std::cout << YELLOW << R"(
 ______   __                          __            _______   _______ 
/      \ |  \                        |  \          |       \ |       \
|  $$$$$$\ \$$ ______ ____    ______  | $$  ______  | $$$$$$$\| $$$$$$$\
| $$___\$$|  \|      \    \  /      \ | $$ /      \ | $$  | $$| $$__/ $$
 \$$    \ | $$| $$$$$$\$$$$\|  $$$$$$\| $$|  $$$$$$\| $$  | $$| $$    $$
 _\$$$$$$\| $$| $$ | $$ | $$| $$  | $$| $$| $$    $$| $$  | $$| $$$$$$$\
|  \__| $$| $$| $$ | $$ | $$| $$__/ $$| $$| $$$$$$$$| $$__/ $$| $$__/ $$
 \$$    $$| $$| $$ | $$ | $$| $$    $$| $$ \$$     \| $$    $$| $$    $$
  \$$$$$$  \$$ \$$  \$$  \$$| $$$$$$$  \$$  \$$$$$$$ \$$$$$$$  \$$$$$$$
                            | $$
                            | $$
                             \$$
)" << RESET << "\n";
    std::cout << "type your SQL and end with ';' to run it. type EXIT to quit.\n";
    std::cout << CYAN << "------------------------------------------------------------" << RESET << "\n";

    std::string input;
    std::string line;

    while (true) {
        std::cout << CYAN << "> " << RESET;
        std::getline(std::cin, line);

        // trim leading/trailing whitespace
        int start = 0;
        while (start < line.size() && isspace(line[start])) start++;
        int end = line.size() - 1;
        while (end > start && isspace(line[end])) end--;
        line = line.substr(start, end - start + 1);

        if (line == "EXIT" || line == "exit") {
            std::cout << YELLOW << "bye!" << RESET << "\n";
            break;
        }

        // clear the screen
        if (line == "CLEAR" || line == "clear") {
            std::cout << "\033[2J\033[1;1H";
            continue;
        }

        if (line.empty()) continue;

        input += " " + line;

        // wait until we see a semicolon before running. that way you can write
        // a query on multiple lines which is pretty cool honestly.
        if (input.find(';') == std::string::npos) continue;

        // remove the semicolon before sending to the database
        int semiPos = input.find(';');
        std::string query = input.substr(0, semiPos);
        input.clear();

        ResultSet result = db.execute(query);
        result.print();
        std::cout << "\n";
    }

    return 0;
}
