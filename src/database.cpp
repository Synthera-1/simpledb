#include "database.h"
#include "lexer.h"
#include "parser.h"
#include <fstream>
#include <stdexcept>
#include <sstream>

// This schema file is the “memory” of the database.
// When you restart, it rebuilds the tables so you don’t lose your stuff.
static const std::string SCHEMA_FILE = "schema.txt";


Database::Database() {
    loadSchema();
}

// the big one. tokenize -> parse -> execute. wraps everything in a try catch so errors
// dont crash the whole program, they just print a message.
ResultSet Database::execute(const std::string& sql) {
    ResultSet result;
    try {
        Lexer lexer(sql);
        std::vector<Token> tokens = lexer.tokenize();
        Parser parser(tokens);
        Statement stmt = parser.parse();

        switch (stmt.type) {
            case StatementType::CREATE_TABLE: return executeCreate(stmt);
            case StatementType::DROP_TABLE:   return executeDrop(stmt);
            case StatementType::INSERT:       return executeInsert(stmt);
            case StatementType::SELECT:       return executeSelect(stmt);
            case StatementType::UPDATE:       return executeUpdate(stmt);
            case StatementType::DELETE:       return executeDelete(stmt);
        }
    } catch (std::exception& e) {
        result.success = false;
        result.message = "Error: " + std::string(e.what());
    }
    return result;
}

// CREATE TABLE. makes a new table and saves the schema so we remember it next time.
ResultSet Database::executeCreate(const Statement& stmt) {
    ResultSet result;
    if (tables.count(stmt.tableName)) {
        result.success = false;
        result.message = "Error: Table '" + stmt.tableName + "' already exists";
        return result;
    }
    tables[stmt.tableName] = Table(stmt.tableName, stmt.columns);
    saveSchema();
    result.message = "Table '" + stmt.tableName + "' created.";
    return result;
}

// DROP TABLE. deletes it from memory and removes the csv file from disk too.
ResultSet Database::executeDrop(const Statement& stmt) {
    ResultSet result;
    if (!tables.count(stmt.tableName)) {
        result.success = false;
        result.message = "Error: Table '" + stmt.tableName + "' does not exist";
        return result;
    }
    tables.erase(stmt.tableName);
    // delete the data file too
    std::remove((stmt.tableName + ".csv").c_str());
    saveSchema();
    result.message = "Table '" + stmt.tableName + "' dropped.";
    return result;
}

ResultSet Database::executeInsert(const Statement& stmt) {
    if (!tables.count(stmt.tableName)) {
        ResultSet r;
        r.success = false;
        r.message = "Error: Table '" + stmt.tableName + "' does not exist";
        return r;
    }
    return tables[stmt.tableName].insert(stmt.values);
}

// SELECT routes to the join handler if theres a join, otherwise just does a normal select.
ResultSet Database::executeSelect(const Statement& stmt) {
    if (!tables.count(stmt.tableName)) {
        ResultSet r;
        r.success = false;
        r.message = "Error: Table '" + stmt.tableName + "' does not exist";
        return r;
    }
    if (!stmt.joinTable.empty()) {
        return executeJoin(stmt);
    }
    return tables[stmt.tableName].select(stmt.selectColumns, stmt.conditions,
                                         stmt.aggregates, stmt.orderBy,
                                         stmt.hasOrderBy, stmt.limit);
}

ResultSet Database::executeUpdate(const Statement& stmt) {
    if (!tables.count(stmt.tableName)) {
        ResultSet r;
        r.success = false;
        r.message = "Error: Table '" + stmt.tableName + "' does not exist";
        return r;
    }
    return tables[stmt.tableName].update(stmt.updateColumn, stmt.updateValue, stmt.conditions);
}

ResultSet Database::executeDelete(const Statement& stmt) {
    if (!tables.count(stmt.tableName)) {
        ResultSet r;
        r.success = false;
        r.message = "Error: Table '" + stmt.tableName + "' does not exist";
        return r;
    }
    return tables[stmt.tableName].deleteRows(stmt.conditions);
}

// JOIN. this was annoying to write. it loops through the left table then for each row
// it scans the right table looking for a match on the join columns. classic nested loop join.
// left join means we keep left rows even if there is no match on the right.
ResultSet Database::executeJoin(const Statement& stmt) {
    ResultSet result;

    if (!tables.count(stmt.joinTable)) {
        result.success = false;
        result.message = "Error: Table '" + stmt.joinTable + "' does not exist";
        return result;
    }

    Table& leftTable  = tables[stmt.tableName];
    Table& rightTable = tables[stmt.joinTable];

    // the join columns look like "tablename.columnname" so we split on the dot
    auto splitDot = [](const std::string& s) -> std::pair<std::string, std::string> {
        int dotPos = s.find('.');
        return { s.substr(0, dotPos), s.substr(dotPos + 1) };
    };

    auto [leftTbl,  leftCol]  = splitDot(stmt.joinLeftCol);
    auto [rightTbl, rightCol] = splitDot(stmt.joinRightCol);

    // figure out which is which. the parser puts left table first but lets be safe.
    std::string leftJoinCol  = (leftTbl  == stmt.tableName) ? leftCol  : rightCol;
    std::string rightJoinCol = (rightTbl == stmt.joinTable) ? rightCol : leftCol;

    // set up output columns. prefix with tablename.columnname to avoid name collisions.
    for (const ColumnDef& col : leftTable.columns) {
        result.columns.push_back(stmt.tableName + "." + col.name);
    }
    for (const ColumnDef& col : rightTable.columns) {
        result.columns.push_back(stmt.joinTable + "." + col.name);
    }

    // the actual join loop
    for (const Row& leftRow : leftTable.rows) {
        bool foundMatch = false;

        for (const Row& rightRow : rightTable.rows) {
            if (leftRow.get(leftJoinCol) == rightRow.get(rightJoinCol)) {
                foundMatch = true;
                std::vector<std::string> combined;
                for (const ColumnDef& col : leftTable.columns) {
                    combined.push_back(leftRow.get(col.name));
                }
                for (const ColumnDef& col : rightTable.columns) {
                    combined.push_back(rightRow.get(col.name));
                }
                result.addRow(combined);
            }
        }

        // left join: if no match was found on the right, still include the left row with NULLs
        if (!foundMatch && stmt.isLeftJoin) {
            std::vector<std::string> combined;
            for (const ColumnDef& col : leftTable.columns) {
                combined.push_back(leftRow.get(col.name));
            }
            for (int i = 0; i < rightTable.columns.size(); i++) {
                combined.push_back("NULL");
            }
            result.addRow(combined);
        }
    }

    return result;
}

// saves the schema to a text file. one table per block. format is just plain text, easy to read.
// TABLE tablename
// colname TYPE primaryKey notNull unique
// ...
// END
void Database::saveSchema() const {
    std::ofstream file(SCHEMA_FILE);
    if (!file.is_open()) throw std::runtime_error("Could not save schema");

    for (const auto& pair : tables) {
        const Table& table = pair.second;
        file << "TABLE " << table.name << "\n";
        for (const ColumnDef& col : table.columns) {
            file << col.name << " "
                 << dataTypeToString(col.type) << " "
                 << col.primaryKey << " "
                 << col.notNull << " "
                 << col.unique << "\n";
        }
        file << "END\n";
    }
    file.close();
}

// reads the schema file back in and recreates all the Table objects.
// then tells each table to load its rows from its own csv file.
void Database::loadSchema() {
    std::ifstream file(SCHEMA_FILE);
    if (!file.is_open()) return; // no schema yet, fresh start

    std::string line;
    while (std::getline(file, line)) {
        if (line.substr(0, 6) == "TABLE ") {
            std::string tableName = line.substr(6);
            std::vector<ColumnDef> cols;

            while (std::getline(file, line) && line != "END") {
                std::istringstream ss(line);
                ColumnDef col;
                std::string typeStr;
                int pk, nn, uq;
                ss >> col.name >> typeStr >> pk >> nn >> uq;
                col.type       = parseDataType(typeStr);
                col.primaryKey = pk;
                col.notNull    = nn;
                col.unique     = uq;
                cols.push_back(col);
            }

            Table t(tableName, cols);
            t.loadFromFile(); // load the actual row data from csv
            tables[tableName] = t;
        }
    }
    file.close();
}
