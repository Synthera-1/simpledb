#pragma once
#include <string>
#include <stdexcept>

// the code can do 3 types of data things.
enum class DataType {
    INT,
    FLOAT,
    TEXT
};

// converts the type they type into the data type they mean. like if they type "BLOBBBB" i will respond "Unkown data type: BLOBBBB"
inline DataType parseDataType(const std::string& s) {
    if (s == "INT")   return DataType::INT;
    if (s == "FLOAT") return DataType::FLOAT;
    if (s == "TEXT")  return DataType::TEXT;
    throw std::runtime_error("Unknown data type: " + s);
}

// toString methud. 
inline std::string dataTypeToString(DataType dt) {
    switch (dt) {
        case DataType::INT:   return "INT";
        case DataType::FLOAT: return "FLOAT";
        case DataType::TEXT:  return "TEXT";
    }
    return "UNKNOWN";
}

// see if the string actually matches the correct type. 
inline bool validateType(const std::string& value, DataType dt) {
    if (dt == DataType::TEXT) return true;
    try {
        if (dt == DataType::INT)   { std::stoi(value); return true; }
        if (dt == DataType::FLOAT) { std::stof(value); return true; }
    } catch (...) {
        return false;
    }
    return false;
}

// column definition :  name, type, and constraints
struct ColumnDef {
    std::string name;
    DataType    type;
    bool        primaryKey = false;
    bool        notNull    = false;
    bool        unique     = false;
};