// Utils.h
// Constants and small helper functions used by every other file.
// Concept demonstrated: constants, arrays, functions, string handling.
#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <sstream>
#include <iomanip>
using namespace std;

// ---------- Constants (same as the original console project) ----------
const int START_YEAR = 2026;
const int END_YEAR = 2027;
const int NUMBER_OF_YEARS = 2;
const int NUMBER_OF_CATEGORIES = 7;

const string CATEGORY_NAMES[NUMBER_OF_CATEGORIES] = {
    "Food", "Travel", "Education", "Entertainment",
    "Shopping", "Bills", "Other"
};

const string MONTH_NAMES[12] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

// ---------- Date helpers ----------
inline bool isLeapYear(int year) {
    if (year % 400 == 0) return true;
    if (year % 100 == 0) return false;
    return (year % 4 == 0);
}

inline int daysInMonth(int month, int year) {
    switch (month) {
        case 2:
            return isLeapYear(year) ? 29 : 28;
        case 4: case 6: case 9: case 11:
            return 30;
        default:
            return 31;
    }
}

// Writes a number with two digits, e.g. 5 -> "05"
inline string pad2(int n) {
    return (n < 10 ? "0" : "") + to_string(n);
}

// ---------- Category helper ----------
// Returns the position of a category in CATEGORY_NAMES (last one = "Other")
inline int getCategoryIndex(const string& name) {
    for (int i = 0; i < NUMBER_OF_CATEGORIES; i++) {
        if (CATEGORY_NAMES[i] == name) return i;
    }
    return NUMBER_OF_CATEGORIES - 1;
}

// ---------- Text helpers ----------
// Removes spaces from the start and end of a string
inline string trimText(const string& text) {
    size_t start = text.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(start, end - start + 1);
}

// Converts a number to text with exactly 2 decimal places
inline string money(double value) {
    ostringstream out;
    out << fixed << setprecision(2) << value;
    return out.str();
}

// Makes a text safe to put inside a JSON string (handles " and \ and new lines)
inline string jsonEscape(const string& text) {
    string result;
    for (size_t i = 0; i < text.size(); i++) {
        char c = text[i];
        if (c == '"')       result += "\\\"";
        else if (c == '\\') result += "\\\\";
        else if (c == '\n') result += "\\n";
        else if (c == '\r') result += "\\r";
        else if (c == '\t') result += "\\t";
        else if ((unsigned char)c < 0x20) result += ' ';
        else result += c;
    }
    return result;
}

#endif
