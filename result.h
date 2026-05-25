#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>

// color codes. reset is super important or everything stays colored forever.
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"

struct ResultSet {
    std::vector<std::string> columns;
    std::vector<std::vector<std::string>> rows;
    std::string message;
    bool success = true;

    //add a row. into the vector. 
    void addRow(const std::vector<std::string>& row) {
        rows.push_back(row);
    }

    void print() const {
        // print message if there is one. green for success, red for errors.
        if (!message.empty()) {
            if (!success) {
                std::cout << RED << message << RESET << "\n";
            } else {
                std::cout << GREEN << message << RESET << "\n";
            }
            return;
        }

        if (columns.empty()) return;

        // Calculate column widths
        std::vector<int> widths;
        for (const std::string& col : columns) {
            widths.push_back(col.size());
        }
        for (const std::vector<std::string>& row : rows) {
            for (int i = 0; i < row.size(); i++) {
                widths[i] = std::max(widths[i], (int)row[i].size());
            }
        }

        // Print top border
        printBorder(widths);

        // column headers in bold cyan. looks really cool.
        std::cout << CYAN << "| " << RESET;
        for (int i = 0; i < columns.size(); i++) {
            std::cout << BOLD << CYAN << std::left << std::setw(widths[i]) << columns[i] << RESET << CYAN << " | " << RESET;
        }
        std::cout << "\n";

        // Print header separator
        printBorder(widths);

        // print rows
        for (const std::vector<std::string>& row : rows) {
            std::cout << CYAN << "| " << RESET;
            for (int i = 0; i < row.size(); i++) {
                std::cout << std::left << std::setw(widths[i]) << row[i] << CYAN << " | " << RESET;
            }
            std::cout << "\n";
        }

        // print bottom.
        printBorder(widths);

        // how many rows? yellow so it stands out.
        std::cout << YELLOW << rows.size() << " row(s) returned." << RESET << "\n";
    }

//also makes it look reallly cool. 
private:
    void printBorder(const std::vector<int>& widths) const {
        std::cout << CYAN << "+";
        for (int w : widths) {
            std::cout << std::string(w + 2, '-') << "+";
        }
        std::cout << RESET << "\n";
    }
};