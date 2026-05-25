#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "datatype.h"

class Row {
public:
    std::unordered_map<std::string, std::string> data;

    Row() = default;
    Row(const std::vector<ColumnDef>& cols, const std::vector<std::string>& values); //classic constructor. super important

    std::string get(const std::string& column) const;
    void set(const std::string& column, const std::string& value);

    // Type-aware getters
    int getInt(const std::string& column) const;
    float getFloat(const std::string& column) const;

    // Convert row to a printable vector of strings in column order
    std::vector<std::string> toVector(const std::vector<ColumnDef>& cols) const;

    // Serialize/deserialize for file storage
    std::string serialize(const std::vector<ColumnDef>& cols) const;
    static Row deserialize(const std::string& line, const std::vector<ColumnDef>& cols);

    // Compare a column value against a condition value
    bool matchesCondition(const std::string& column, const std::string& op,
                          const std::string& value, DataType type) const;
};