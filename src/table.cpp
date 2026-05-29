#include "table.h"
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <sstream>

Table::Table(const std::string& name, const std::vector<ColumnDef>& columns)
    : name(name), columns(columns) {}

// the file is just called tablename.csv and lives in the current folder
std::string Table::getFilePath() const {
    return name + ".csv";
}

int Table::getColumnIndex(const std::string& colName) const {
    for (int i = 0; i < columns.size(); i++) {
        if (columns[i].name == colName) return i;
    }
    return -1;
}

bool Table::columnExists(const std::string& colName) const {
    return getColumnIndex(colName) != -1;
}

DataType Table::getColumnType(const std::string& colName) const {
    int i = getColumnIndex(colName);
    if (i == -1) throw std::runtime_error("Column not found: " + colName);
    return columns[i].type;
}

// goes through all conditions and checks each one. if its AND all must pass, if its OR any one passing is enough.
// the first condition has no logic keyword so we start with true and AND it in.
bool Table::rowMatchesConditions(const Row& row, const std::vector<Condition>& conditions) const {
    if (conditions.empty()) return true;

    bool result = row.matchesCondition(conditions[0].column, conditions[0].op,
                                       conditions[0].value,
                                       getColumnType(conditions[0].column)); // FIX THIS PART TO INCLUDE THE conditions.column SYNTAX

    for (int i = 1; i < conditions.size(); i++) {
        bool next = row.matchesCondition(conditions[i].column, conditions[i].op,
                                         conditions[i].value,
                                         getColumnType(conditions[i].column));
        if (conditions[i].logic == "OR") {
            result = result || next;
        } else {
            // default to AND
            result = result && next;
        }
    }
    return result;
}

// checks NOT NULL and UNIQUE before we let a row in. fills errorMsg if something is wrong.
bool Table::checkConstraints(const std::vector<std::string>& values, std::string& errorMsg) const {
    for (int i = 0; i < columns.size(); i++) {
        // not null check
        if (columns[i].notNull && (values[i] == "NULL" || values[i] == "")) {
            errorMsg = "Column '" + columns[i].name + "' cannot be NULL";
            return false;
        }
        // unique check - gotta scan all existing rows. not fast but it works.
        if (columns[i].unique && values[i] != "NULL") {
            for (const Row& row : rows) {
                if (row.get(columns[i].name) == values[i]) {
                    errorMsg = "Duplicate value '" + values[i] + "' for unique column '" + columns[i].name + "'";
                    return false;
                }
            }
        }
    }
    return true;
}

// INSERT. validates constraints then builds the row and adds it. also saves right away so we dont lose anything.
ResultSet Table::insert(const std::vector<std::string>& values) {
    ResultSet result;
    std::string errorMsg;
    if (!checkConstraints(values, errorMsg)) {
        result.success = false;
        result.message = "Error: " + errorMsg;
        return result;
    }
    try {
        Row newRow(columns, values);
        rows.push_back(newRow);
        saveToFile();
        result.message = "1 row inserted.";
    } catch (std::exception& e) {
        result.success = false;
        result.message = "Error: " + std::string(e.what());
    }
    return result;
}

// SELECT. filters rows by conditions, then handles aggregates or regular column output. 
// then sorts if there is an ORDER BY, then cuts it down if there is a LIMIT.
ResultSet Table::select(const std::vector<std::string>& selectCols,
                        const std::vector<Condition>& conditions,
                        const std::vector<AggregateFunc>& aggregates,
                        const OrderBy& orderBy, bool hasOrderBy,
                        int limit) {
    ResultSet result;

    // grab matching rows first
    std::vector<Row> matching;
    for (const Row& row : rows) {
        if (rowMatchesConditions(row, conditions)) {
            matching.push_back(row);
        }
    }

    // aggregate mode. COUNT SUM AVG MIN MAX. completely different output from regular select.
    if (!aggregates.empty())            //logic error. fix this part too.
        for (const AggregateFunc& agg : aggregates) {
            result.columns.push_back(agg.func + "(" + agg.column + ")");
        }
        std::vector<std::string> aggRow;
        for (const AggregateFunc& agg : aggregates) {
            if (agg.func == "COUNT") {
                if (agg.column == "*") {
                    aggRow.push_back(std::to_string(matching.size()));
                } else {
                    // count non-null values
                    int count = 0;
                    for (const Row& row : matching) {
                        if (row.get(agg.column) != "NULL" && row.get(agg.column) != "") count++;
                    }
                    aggRow.push_back(std::to_string(count));
                }
            } else if (agg.func == "SUM") {
                float sum = 0;
                for (const Row& row : matching) sum += row.getFloat(agg.column);
                aggRow.push_back(std::to_string(sum));
            } else if (agg.func == "AVG") {
                if (matching.empty()) { aggRow.push_back("NULL"); continue; }
                float sum = 0;
                for (const Row& row : matching) sum += row.getFloat(agg.column);
                aggRow.push_back(std::to_string(sum / matching.size()));
            } else if (agg.func == "MIN") {
                if (matching.empty()) { aggRow.push_back("NULL"); continue; }
                std::string minVal = matching[0].get(agg.column);
                for (const Row& row : matching) {
                    if (row.get(agg.column) < minVal) minVal = row.get(agg.column);
                }
                aggRow.push_back(minVal);
            } else if (agg.func == "MAX") {
                if (matching.empty()) { aggRow.push_back("NULL"); continue; }
                std::string maxVal = matching[0].get(agg.column);
                for (const Row& row : matching) {
                    if (row.get(agg.column) > maxVal) maxVal = row.get(agg.column);
                }
                aggRow.push_back(maxVal);
            }
        }
        result.addRow(aggRow);
        return result;
    }

    // regular select. figure out which columns to show.
    std::vector<std::string> outputCols;
    if (selectCols.empty()) {
        // SELECT * - show all columns
        for (const ColumnDef& col : columns) { //here too
            outputCols.push_back(col.name);
        }
    } else {
        for (const std::string& col : selectCols) {
            if (!columnExists(col)) {
                result.success = false;
                result.message = "Error: Unknown column '" + col + "'";
                return result;
            }
            outputCols.push_back(col);
        }
    }
    result.columns = outputCols;

    // sort if we need to. uses a lambda to compare rows by the order by column.
    if (hasOrderBy) {
        DataType sortType = getColumnType(orderBy.column);
        std::sort(matching.begin(), matching.end(), [&](const Row& a, const Row& b) {
            if (sortType == DataType::INT) {
                return orderBy.ascending ? a.getInt(orderBy.column) < b.getInt(orderBy.column)
                                         : a.getInt(orderBy.column) > b.getInt(orderBy.column);
            } else if (sortType == DataType::FLOAT) {
                return orderBy.ascending ? a.getFloat(orderBy.column) < b.getFloat(orderBy.column)
                                         : a.getFloat(orderBy.column) > b.getFloat(orderBy.column);
            } else {
                return orderBy.ascending ? a.get(orderBy.column) < b.get(orderBy.column)
                                         : a.get(orderBy.column) > b.get(orderBy.column);
            }
        });
    }

    // apply limit if there is one
    int count = 0;
    for (const Row& row : matching) {
        if (limit != -1 && count >= limit) break;
        std::vector<std::string> rowData;
        for (const std::string& col : outputCols) {
            rowData.push_back(row.get(col));
        }
        result.addRow(rowData);
        count++;
    }

    return result;
}

// UPDATE. loops through rows, checks conditions, updates the value if it matches. 
ResultSet Table::update(const std::string& column, const std::string& value,
                        const std::vector<Condition>& conditions) {
    ResultSet result;
    if (!columnExists(column)) {
        result.success = false;
        result.message = "Error: Unknown column '" + column + "'";
        return result;
    }

    DataType colType = getColumnType(column);
    if (!validateType(value, colType) && value != "NULL") {
        result.success = false;
        result.message = "Error: Invalid value '" + value + "' for column type";
        return result;
    }

    int updatedCount = 0;
    for (Row& row : rows) {
        if (rowMatchesConditions(row, conditions)) {
            row.set(column, value);
            updatedCount++;
        }
    }
    saveToFile();
    result.message = std::to_string(updatedCount) + " row(s) updated.";
    return result;
}

// DELETE. builds a new vector without the matching rows. then swaps it in. 
ResultSet Table::deleteRows(const std::vector<Condition>& conditions) {
    ResultSet result;
    std::vector<Row> remaining;
    int deletedCount = 0;

    for (const Row& row : rows) {
        if (rowMatchesConditions(row, conditions)) {
            deletedCount++;
        } else {
            remaining.push_back(row);
        }
    }

    rows = remaining;
    saveToFile();
    result.message = std::to_string(deletedCount) + " row(s) deleted.";
    return result;
}

// writes all rows to a csv file. first line is the column names as a header.
void Table::saveToFile() const {
    std::ofstream file(getFilePath());
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file for writing: " + getFilePath());
    }
    // header row
    for (int i = 0; i < columns.size(); i++) {
        file << columns[i].name;
        if (i < columns.size() - 1) file << ",";
    }
    file << "\n";
    // data rows
    for (const Row& row : rows) {
        file << row.serialize(columns) << "\n";
    }
    file.close();
}

// reads the csv file back in. skips the header line, then deserializes each row.
void Table::loadFromFile() {
    std::ifstream file(getFilePath());
    if (!file.is_open()) return; // file doesnt exist yet, thats okay

    std::string line;
    bool firstLine = true;
    while (std::getline(file, line)) {
        if (firstLine) { firstLine = false; continue; } // skip header
        if (line.empty()) continue;
        rows.push_back(Row::deserialize(line, columns));
    }
    file.close();
}
