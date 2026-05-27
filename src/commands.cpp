#include "commands.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <iomanip>

// checks if something looks like a command instead of SQL.
// if it starts with HELP, SHOW, MEMORY, DESCRIBE, or TRUNCATE its a command.
bool CommandHandler::isCommand(const std::string& input) {
    std::string upper = input;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    
    return upper.substr(0, 4) == "HELP" ||
           upper.substr(0, 4) == "SHOW" ||
           upper.substr(0, 6) == "MEMORY" ||
           upper.substr(0, 8) == "DESCRIBE" ||
           upper.substr(0, 8) == "TRUNCATE";
}

// the main execute function. looks at what command it is and calls the right one.
ResultSet CommandHandler::executeCommand(const std::string& input,
                                        const std::unordered_map<std::string, Table>& tables) {
    std::string upper = input;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    if (upper.substr(0, 4) == "HELP") {
        return showHelp();
    }
    else if (upper.substr(0, 4) == "SHOW") {
        return showTables(tables);
    }
    else if (upper.substr(0, 6) == "MEMORY") {
        return showMemory(tables);
    }
    else if (upper.substr(0, 8) == "DESCRIBE") {
        // DESCRIBE table_name
        std::istringstream ss(input);
        std::string cmd, tableName;
        ss >> cmd >> tableName;
        return describeTable(tableName, tables);
    }
    else if (upper.substr(0, 8) == "TRUNCATE") {
        // TRUNCATE is handled by the database since it modifies tables
        // so if we get here something went wrong
        ResultSet r;
        r.success = false;
        r.message = "Error: TRUNCATE must be passed to the database, not command handler";
        return r;
    }

    ResultSet r;
    r.success = false;
    r.message = "Error: unknown command";
    return r;
}

// HELP command - shows all the commands and how to use them with examples
ResultSet CommandHandler::showHelp() {
    ResultSet result;
    result.success = true;
    result.message = 
        "=== SimpleDB Commands ===\n\n"
        "HELP\n"
        "  Shows this help message.\n\n"
        "SHOW TABLES\n"
        "  Lists all tables in the database with their row count and column count.\n"
        "  Example: SHOW TABLES\n\n"
        "DESCRIBE table_name\n"
        "  Shows the schema of a table (column names, types, and constraints).\n"
        "  Example: DESCRIBE students\n\n"
        "MEMORY\n"
        "  Shows memory usage stats: how many tables, total rows, total values,\n"
        "  and estimated RAM usage.\n"
        "  Example: MEMORY\n\n"
        "TRUNCATE table_name;\n"
        "  Deletes all rows from a table but keeps the table structure.\n"
        "  Much faster than DROP and then CREATE.\n"
        "  Example: TRUNCATE students;\n\n"
        "=== SQL Commands ===\n"
        "CREATE TABLE, INSERT, SELECT, UPDATE, DELETE, DROP TABLE\n"
        "All SQL commands must end with a semicolon (;)\n";
    
    return result;
}

// SHOW TABLES command - lists all tables with info about them
ResultSet CommandHandler::showTables(const std::unordered_map<std::string, Table>& tables) {
    ResultSet result;
    result.success = true;
    result.columns = {"Table Name", "Rows", "Columns"};

    if (tables.empty()) {
        result.message = "No tables found.";
        return result;
    }

    for (const auto& pair : tables) {
        const Table& table = pair.second;
        std::vector<std::string> row;
        row.push_back(table.name);
        row.push_back(std::to_string(table.rows.size()));
        row.push_back(std::to_string(table.columns.size()));
        result.addRow(row);
    }

    return result;
}

// MEMORY command - shows how much memory is being used
ResultSet CommandHandler::showMemory(const std::unordered_map<std::string, Table>& tables) {
    ResultSet result;
    result.success = true;
    result.columns = {"Statistic", "Value"};

    int totalTables = tables.size();
    int totalRows = 0;
    int totalValues = 0;
    int estimatedBytes = 0;

    for (const auto& pair : tables) {
        const Table& table = pair.second;
        totalRows += table.rows.size();
        totalValues += table.rows.size() * table.columns.size();
        
        // rough estimate: each value is about 20 bytes on average (including overhead)
        // and each table schema is about 100 bytes
        estimatedBytes += 100;
        for (const Row& row : table.rows) {
            for (const ColumnDef& col : table.columns) {
                estimatedBytes += 20;
            }
        }
    }

    result.addRow({"Total Tables", std::to_string(totalTables)});
    result.addRow({"Total Rows", std::to_string(totalRows)});
    result.addRow({"Total Values", std::to_string(totalValues)});
    result.addRow({"Estimated RAM (bytes)", std::to_string(estimatedBytes)});
    result.addRow({"Estimated RAM (KB)", std::to_string(estimatedBytes / 1024)});

    return result;
}

// DESCRIBE command - shows the schema of a specific table
ResultSet CommandHandler::describeTable(const std::string& tableName,
                                       const std::unordered_map<std::string, Table>& tables) {
    ResultSet result;
    result.success = true;
    result.columns = {"Column Name", "Type", "Primary Key", "Not Null", "Unique"};

    auto it = tables.find(tableName);
    if (it == tables.end()) {
        result.success = false;
        result.message = "Error: Table '" + tableName + "' does not exist";
        return result;
    }

    const Table& table = it->second;

    for (const ColumnDef& col : table.columns) {
        std::vector<std::string> row;
        row.push_back(col.name);
        row.push_back(dataTypeToString(col.type));
        row.push_back(col.primaryKey ? "YES" : "NO");
        row.push_back(col.notNull ? "YES" : "NO");
        row.push_back(col.unique ? "YES" : "NO");
        result.addRow(row);
    }

    return result;
}
