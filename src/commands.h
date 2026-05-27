#pragma once
#include <string>
#include <unordered_map>
#include "table.h"
#include "result.h"

// This class handles all the special commands that aren't SQL.
// like HELP, SHOW TABLES, MEMORY, DESCRIBE, and TRUNCATE.
// they're all just helper things to make the database easier to use.
class CommandHandler {
public:
    // checks if the input is a command (not SQL).
    // returns true if its something like HELP, SHOW TABLES, etc.
    static bool isCommand(const std::string& input);

    // runs the command. you pass in the command string and the database tables.
    // it returns a ResultSet with the output formatted nicely.
    static ResultSet executeCommand(const std::string& input, 
                                   const std::unordered_map<std::string, Table>& tables);

private:
    // each command gets its own function
    static ResultSet showHelp();
    static ResultSet showTables(const std::unordered_map<std::string, Table>& tables);
    static ResultSet showMemory(const std::unordered_map<std::string, Table>& tables);
    static ResultSet describeTable(const std::string& tableName,
                                  const std::unordered_map<std::string, Table>& tables);
    
    // note: TRUNCATE is actually SQL so it stays in the database class.
    // we're just handling the info commands here.
};
