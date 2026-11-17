// BudgetManager.h
// Class : BudgetManager -> sets monthly budgets and compares them with spending.
// Concepts: class, encapsulation, conditional statements, percentage calculation,
//           input validation.
#ifndef BUDGETMANAGER_H
#define BUDGETMANAGER_H

#include <string>
#include <sstream>
#include <stdexcept>
#include "Database.h"
using namespace std;

class BudgetManager {
private:
    Database& db;

public:
    BudgetManager(Database& database) : db(database) {}

    // Saves the budget of one month (replaces the old one if it exists).
    // Input : month, year, amount.  Output : nothing (throws on wrong input).
    void setBudget(int month, int year, double amount) {
        if (year < START_YEAR || year > END_YEAR)
            throw invalid_argument("Year must be 2026 or 2027.");
        if (month < 1 || month > 12)
            throw invalid_argument("Month must be between 1 and 12.");
        if (amount <= 0 || amount > 10000000)
            throw invalid_argument("Budget must be greater than 0 and at most 10000000.");
        db.setBudget(month, year, amount);
    }

    // Compares the budget of a month with the money actually spent.
    // Input : month, year and total spent.  Output (JSON): budget, spent,
    // percentage used, remaining amount and a status message.
    string compare(int month, int year, double spent) {
        double budget = db.getBudget(month, year);

        ostringstream out;
        out << "{\"month\":\"" << MONTH_NAMES[month - 1] << "\",\"year\":" << year;

        if (budget == 0) {
            out << ",\"budgetSet\":false}";
            return out.str();
        }

        double difference = budget - spent;
        double percentUsed = (spent / budget) * 100;

        string status;
        if (spent > budget)            status = "BUDGET EXCEEDED! Reduce spending.";
        else if (percentUsed >= 80)    status = "WARNING - more than 80% of the budget is used.";
        else                           status = "Good - you are within budget.";

        out << ",\"budgetSet\":true"
            << ",\"budget\":" << money(budget)
            << ",\"spent\":" << money(spent)
            << ",\"percentUsed\":" << money(percentUsed)
            << ",\"difference\":" << money(difference >= 0 ? difference : -difference)
            << ",\"overBudget\":" << (difference < 0 ? "true" : "false")
            << ",\"status\":\"" << status << "\"}";
        return out.str();
    }

    // Returns all saved budgets as JSON (for the budget table on the website).
    string allBudgetsJson() {
        vector<Budget> list = db.getAllBudgets();
        ostringstream out;
        out << "{\"budgets\":[";
        for (size_t i = 0; i < list.size(); i++) {
            if (i > 0) out << ",";
            out << "{\"month\":\"" << MONTH_NAMES[list[i].month - 1] << "\""
                << ",\"year\":" << list[i].year
                << ",\"amount\":" << money(list[i].amount) << "}";
        }
        out << "]}";
        return out.str();
    }
};

#endif
