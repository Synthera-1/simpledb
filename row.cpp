#include "row.h"
#include <sstream>
#include <stdexcept>

//always double check. here, we literally check twice with two layers of validation before we store anything.
Row::Row(const std::vector<ColumnDef>& cols, const std::vector<std::string>& values) {
    if (values.size() != cols.size()) {
        throw std::runtime_error("Column count doesn't match value count");
    }
    for (int i = 0; i < cols.size(); i++) {
        if (!validateType(values[i], cols[i].type) && values[i] != "NULL") {
            throw std::runtime_error("Invalid value '" + values[i] +
                "' for column '" + cols[i].name + "' of type " +
                dataTypeToString(cols[i].type));
        }
        data[cols[i].name] = values[i];
    }
}

std::string Row::get(const std::string& column) const {
    auto it = data.find(column);
    if (it != data.end()) return it->second;
    return "";
}

void Row::set(const std::string& column, const std::string& value) {
    data[column] = value;
}

//converts the string o an int or floaty. it doesnt crash because of the try catch safety wheels.
int Row::getInt(const std::string& column) const {
    try { return std::stoi(get(column)); }
    catch (...) { return 0; }
}

float Row::getFloat(const std::string& column) const {
    try { return std::stof(get(column)); }
    catch (...) { return 0.0f; }
}

//loops through columns in their order and pulls each value out of the map. 
std::vector<std::string> Row::toVector(const std::vector<ColumnDef>& cols) const {
    std::vector<std::string> result;
    for (const ColumnDef& col : cols) {
        result.push_back(get(col.name));
    }
    return result;
}

//converts a row to a .csv line. so i can save it onto the disk. 
std::string Row::serialize(const std::vector<ColumnDef>& cols) const {
    std::string result;
    for (int i = 0; i < cols.size(); i++) {
        std::string val = get(cols[i].name);
        // Wrap text values in quotes to handle commas inside values
        if (cols[i].type == DataType::TEXT) {
            result += "\"" + val + "\"";
        } else {
            result += val;
        }
        if (i < cols.size() - 1) result += ",";
    }
    return result;
}

//it basically does the opposite of what serialize did. 
Row Row::deserialize(const std::string& line, const std::vector<ColumnDef>& cols) {
    Row row;
    std::vector<std::string> values;
    std::string current;
    bool inQuotes = false;

    for (char c : line) {
        if (c == '"') {
            inQuotes = !inQuotes;
        } else if (c == ',' && !inQuotes) {
            values.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    values.push_back(current);

    for (int i = 0; i < cols.size() && i < values.size(); i++) {
        row.data[cols[i].name] = values[i];
    }
    return row;
}

//it does different things based on the type of the column.
bool Row::matchesCondition(const std::string& column, const std::string& op,
                           const std::string& value, DataType type) const {
    std::string rowVal = get(column);

    // Numeric comparison for INT and FLOAT
    if (type == DataType::INT) {
        int rowInt = getInt(column);
        int valInt = std::stoi(value);
        if (op == "=")  return rowInt == valInt;
        if (op == "!=") return rowInt != valInt;
        if (op == "<")  return rowInt <  valInt;
        if (op == ">")  return rowInt >  valInt;
        if (op == "<=") return rowInt <= valInt;
        if (op == ">=") return rowInt >= valInt;
    } else if (type == DataType::FLOAT) {
        float rowFloat = getFloat(column);
        float valFloat = std::stof(value);
        if (op == "=")  return rowFloat == valFloat;
        if (op == "!=") return rowFloat != valFloat;
        if (op == "<")  return rowFloat <  valFloat;
        if (op == ">")  return rowFloat >  valFloat;
        if (op == "<=") return rowFloat <= valFloat;
        if (op == ">=") return rowFloat >= valFloat;
    } else {
        // TEXT comparison
        if (op == "=")  return rowVal == value;
        if (op == "!=") return rowVal != value;
        if (op == "<")  return rowVal <  value;
        if (op == ">")  return rowVal >  value;
        if (op == "<=") return rowVal <= value;
        if (op == ">=") return rowVal >= value;
    }
    return false;
}