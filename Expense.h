// Expense.h
// Class 1 : Expense -> stores ONE expense (same idea as the console version).
// Concepts: class, object, encapsulation (private data + getters), constructor.
#ifndef EXPENSE_H
#define EXPENSE_H

#include <string>
#include <sstream>
#include "Utils.h"
using namespace std;

class Expense {
private:
    int id;
    double amount;
    string category;
    int day;
    int month;
    int year;
    string description;

public:
    // Default constructor (used when we need an empty object)
    Expense() {
        id = 0; amount = 0; category = ""; day = 1; month = 1;
        year = START_YEAR; description = "";
    }

    // Parameterised constructor: fills all data members
    Expense(int expenseId, double expenseAmount, string expenseCategory,
            int expenseDay, int expenseMonth, int expenseYear,
            string expenseDescription) {
        id = expenseId;
        amount = expenseAmount;
        category = expenseCategory;
        day = expenseDay;
        month = expenseMonth;
        year = expenseYear;
        description = expenseDescription;
    }

    // Getters (read-only access to private data)
    int getId() const { return id; }
    double getAmount() const { return amount; }
    string getCategory() const { return category; }
    int getDay() const { return day; }
    int getMonth() const { return month; }
    int getYear() const { return year; }
    string getDescription() const { return description; }

    // Returns date as DD-MM-YYYY (for showing in the table)
    string getDateString() const {
        return pad2(day) + "-" + pad2(month) + "-" + to_string(year);
    }

    // Returns date as YYYY-MM-DD (the format of an HTML date input)
    string getIsoDate() const {
        return to_string(year) + "-" + pad2(month) + "-" + pad2(day);
    }

    // Converts this expense into JSON text so the browser can read it
    string toJson() const {
        ostringstream out;
        out << "{\"id\":" << id
            << ",\"amount\":" << money(amount)
            << ",\"category\":\"" << jsonEscape(category) << "\""
            << ",\"day\":" << day
            << ",\"month\":" << month
            << ",\"year\":" << year
            << ",\"date\":\"" << getDateString() << "\""
            << ",\"isoDate\":\"" << getIsoDate() << "\""
            << ",\"description\":\"" << jsonEscape(description) << "\"}";
        return out.str();
    }
};

#endif
