#pragma once
#include <string>
#include <unordered_map>
#include "table.h"
#include "parser.h"
#include "result.h"

// the database is the big boss. it holds all the tables and runs every sql statement.
// its basically the thing that connects the parser output to the actual table operations.
class Database {
public:
    Database();

    // this is the main one. you give it a sql string and it does everything. lex, parse, execute.
    ResultSet execute(const std::string& sql);

private:
    // all the tables live here. key is the table name.
    std::unordered_map<std::string, Table> tables;

    // one method per statement type. execute() routes to the right one.
    ResultSet executeCreate(const Statement& stmt);
    ResultSet executeDrop(const Statement& stmt);
    ResultSet executeInsert(const Statement& stmt);
    ResultSet executeSelect(const Statement& stmt);
    ResultSet executeUpdate(const Statement& stmt);
    ResultSet executeDelete(const Statement& stmt);

    // join helper. does the actual joining of two tables for SELECT with JOIN.
    ResultSet executeJoin(const Statement& stmt);

    // saves and loads the list of tables (their schemas) so they survive restart
    void saveSchema() const;
    void loadSchema();
};
