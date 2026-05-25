#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "row.h"
#include "datatype.h"
#include "parser.h"
#include "result.h"

// a table is just a name, a list of column definitions, and a bunch of rows.
// it also knows how to save itself to a file and load itself back.
class Table {
public:
    std::string name;
    std::vector<ColumnDef> columns;
    std::vector<Row> rows;

    Table() = default;
    Table(const std::string& name, const std::vector<ColumnDef>& columns);

    // the main sql operations. each one returns a ResultSet so the caller can print it.
    ResultSet insert(const std::vector<std::string>& values);
    ResultSet select(const std::vector<std::string>& selectCols,
                     const std::vector<Condition>& conditions,
                     const std::vector<AggregateFunc>& aggregates,
                     const OrderBy& orderBy, bool hasOrderBy,
                     int limit);
    ResultSet update(const std::string& column, const std::string& value,
                     const std::vector<Condition>& conditions);
    ResultSet deleteRows(const std::vector<Condition>& conditions);

    // saves/loads the table from a csv file on disk
    void saveToFile() const;
    void loadFromFile();

    // helpers
    int getColumnIndex(const std::string& colName) const;
    bool columnExists(const std::string& colName) const;
    DataType getColumnType(const std::string& colName) const;

private:
    // checks if a row passes all the where conditions
    bool rowMatchesConditions(const Row& row, const std::vector<Condition>& conditions) const;

    // checks unique and not null constraints before inserting
    bool checkConstraints(const std::vector<std::string>& values, std::string& errorMsg) const;

    // the file path where this table lives on disk
    std::string getFilePath() const;
};
