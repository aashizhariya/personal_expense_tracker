// ExpenseTracker.h
// Class 2 : ExpenseTracker -> all the expense features and calculations.
// It checks (validates) the data, uses Database to save/read it, and
// calculates totals, averages, highest/lowest, category shares, etc.
// Concepts: OOP (class with a Database reference), vectors, loops,
//           conditional statements, input validation, exceptions.
#ifndef EXPENSETRACKER_H
#define EXPENSETRACKER_H

#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>
#include "Expense.h"
#include "Database.h"
using namespace std;

class ExpenseTracker {
private:
    Database& db;   // the tracker uses this database to store data

    // ---------- Validation (throws invalid_argument when data is wrong) ----------
    void validateMonthYear(int month, int year) const {
        if (year < START_YEAR || year > END_YEAR)
            throw invalid_argument("Year must be 2026 or 2027.");
        if (month < 1 || month > 12)
            throw invalid_argument("Month must be between 1 and 12.");
    }

    void validateExpense(double amount, const string& category, int day,
                         int month, int year, const string& description) const {
        if (amount <= 0 || amount > 10000000)
            throw invalid_argument("Amount must be greater than 0 and at most 10000000.");

        bool categoryFound = false;
        for (int i = 0; i < NUMBER_OF_CATEGORIES; i++) {
            if (CATEGORY_NAMES[i] == category) categoryFound = true;
        }
        if (!categoryFound)
            throw invalid_argument("Please select a valid category.");

        validateMonthYear(month, year);
        if (day < 1 || day > daysInMonth(month, year))
            throw invalid_argument("Day is not valid for the selected month.");

        if (description.empty())
            throw invalid_argument("Description cannot be empty.");
        if (description.size() > 100)
            throw invalid_argument("Description can have at most 100 characters.");
    }

    // Gets every expense of one month
    vector<Expense> getMonthExpenses(int month, int year) {
        ExpenseFilter filter;
        filter.month = month;
        filter.year = year;
        return db.getExpenses(filter);
    }

public:
    ExpenseTracker(Database& database) : db(database) {}

    // ---------------- Add ----------------
    // Input : expense details.  Output : id of the new expense.
    int addExpense(double amount, const string& category, int day, int month,
                   int year, const string& rawDescription) {
        string description = trimText(rawDescription);
        validateExpense(amount, category, day, month, year, description);
        Expense newExpense(0, amount, category, day, month, year, description);
        return db.insertExpense(newExpense);
    }

    // ---------------- Update ----------------
    void updateExpense(int id, double amount, const string& category, int day,
                       int month, int year, const string& rawDescription) {
        string description = trimText(rawDescription);
        validateExpense(amount, category, day, month, year, description);
        Expense changed(id, amount, category, day, month, year, description);
        if (!db.updateExpense(changed))
            throw out_of_range("No expense found with ID " + to_string(id) + ".");
    }

    // ---------------- Delete ----------------
    void deleteExpense(int id) {
        if (!db.deleteExpense(id))
            throw out_of_range("No expense found with ID " + to_string(id) + ".");
    }

    // ---------------- Search + filter + sort ----------------
    vector<Expense> searchAndFilter(const ExpenseFilter& filter) {
        return db.getExpenses(filter);
    }

    // Total spending of one month (loop + sum)
    double getMonthTotal(int month, int year) {
        validateMonthYear(month, year);
        vector<Expense> list = getMonthExpenses(month, year);
        double total = 0;
        for (size_t i = 0; i < list.size(); i++) {
            total += list[i].getAmount();
        }
        return total;
    }

    // ---------------- Monthly analysis ----------------
    // Output (JSON): count, total, average, highest and lowest expense.
    string monthlyAnalysis(int month, int year) {
        validateMonthYear(month, year);
        vector<Expense> list = getMonthExpenses(month, year);

        int count = (int)list.size();
        double total = 0;
        int highestIndex = -1;
        int lowestIndex = -1;

        for (size_t i = 0; i < list.size(); i++) {
            total += list[i].getAmount();
            if (highestIndex == -1 || list[i].getAmount() > list[highestIndex].getAmount())
                highestIndex = (int)i;
            if (lowestIndex == -1 || list[i].getAmount() < list[lowestIndex].getAmount())
                lowestIndex = (int)i;
        }

        ostringstream out;
        out << "{\"month\":" << month
            << ",\"monthName\":\"" << MONTH_NAMES[month - 1] << "\""
            << ",\"year\":" << year
            << ",\"count\":" << count
            << ",\"total\":" << money(total);
        if (count > 0) {
            out << ",\"average\":" << money(total / count)
                << ",\"highest\":{\"amount\":" << money(list[highestIndex].getAmount())
                << ",\"description\":\"" << jsonEscape(list[highestIndex].getDescription()) << "\"}"
                << ",\"lowest\":{\"amount\":" << money(list[lowestIndex].getAmount())
                << ",\"description\":\"" << jsonEscape(list[lowestIndex].getDescription()) << "\"}";
        }
        out << "}";
        return out.str();
    }

    // ---------------- Category-wise analysis ----------------
    // Output (JSON): amount and percentage share of every category.
    string categoryAnalysis(int month, int year) {
        validateMonthYear(month, year);
        vector<Expense> list = getMonthExpenses(month, year);

        double categoryTotals[NUMBER_OF_CATEGORIES] = {0};
        double monthTotal = 0;
        for (size_t i = 0; i < list.size(); i++) {
            int index = getCategoryIndex(list[i].getCategory());
            categoryTotals[index] += list[i].getAmount();
            monthTotal += list[i].getAmount();
        }

        int highestCategory = 0;
        for (int i = 1; i < NUMBER_OF_CATEGORIES; i++) {
            if (categoryTotals[i] > categoryTotals[highestCategory]) highestCategory = i;
        }

        ostringstream out;
        out << "{\"month\":" << month
            << ",\"monthName\":\"" << MONTH_NAMES[month - 1] << "\""
            << ",\"year\":" << year
            << ",\"total\":" << money(monthTotal);
        if (monthTotal > 0) {
            out << ",\"highestCategory\":\"" << CATEGORY_NAMES[highestCategory] << "\""
                << ",\"highestAmount\":" << money(categoryTotals[highestCategory]);
        }
        out << ",\"categories\":[";
        for (int i = 0; i < NUMBER_OF_CATEGORIES; i++) {
            double share = (monthTotal > 0) ? (categoryTotals[i] / monthTotal) * 100 : 0;
            if (i > 0) out << ",";
            out << "{\"name\":\"" << CATEGORY_NAMES[i] << "\""
                << ",\"amount\":" << money(categoryTotals[i])
                << ",\"share\":" << money(share) << "}";
        }
        out << "]}";
        return out.str();
    }

    // ---------------- Yearly trend (data for the monthly chart) ----------------
    // Output (JSON): 12 monthly totals and 12 monthly budgets of one year.
    string yearlyTrend(int year) {
        validateMonthYear(1, year);
        ExpenseFilter filter;
        filter.year = year;
        vector<Expense> list = db.getExpenses(filter);

        double totals[12] = {0};
        for (size_t i = 0; i < list.size(); i++) {
            totals[list[i].getMonth() - 1] += list[i].getAmount();
        }

        ostringstream out;
        out << "{\"year\":" << year << ",\"totals\":[";
        for (int m = 0; m < 12; m++) {
            if (m > 0) out << ",";
            out << money(totals[m]);
        }
        out << "],\"budgets\":[";
        for (int m = 1; m <= 12; m++) {
            if (m > 1) out << ",";
            out << money(db.getBudget(m, year));
        }
        out << "]}";
        return out.str();
    }

    // ---------------- Financial summary (2026 - 2027) ----------------
    // Output (JSON): overall totals, monthly table and insights.
    string financialSummary() {
        vector<Expense> all = db.getExpenses(ExpenseFilter());
        if (all.empty()) return "{\"count\":0}";

        double overallTotal = 0;
        double yearTotals[NUMBER_OF_YEARS] = {0, 0};
        double categoryTotals[NUMBER_OF_CATEGORIES] = {0};
        double monthTotals[NUMBER_OF_YEARS][12] = {};
        int monthCounts[NUMBER_OF_YEARS][12] = {};
        int biggestIndex = 0;

        // One pass over all expenses fills every array
        for (size_t i = 0; i < all.size(); i++) {
            double amount = all[i].getAmount();
            int yearIndex = all[i].getYear() - START_YEAR;
            overallTotal += amount;
            yearTotals[yearIndex] += amount;
            categoryTotals[getCategoryIndex(all[i].getCategory())] += amount;
            monthTotals[yearIndex][all[i].getMonth() - 1] += amount;
            monthCounts[yearIndex][all[i].getMonth() - 1]++;
            if (amount > all[biggestIndex].getAmount()) biggestIndex = (int)i;
        }

        // Monthly table (only months that have expenses)
        ostringstream rows;
        int activeMonths = 0, monthsWithBudget = 0, monthsOverBudget = 0;
        double activeTotal = 0, highestMonthTotal = 0, lowestMonthTotal = 0;
        int highestMonth = 0, highestYear = 0, lowestMonth = 0, lowestYear = 0;

        for (int y = 0; y < NUMBER_OF_YEARS; y++) {
            for (int m = 0; m < 12; m++) {
                if (monthCounts[y][m] == 0) continue;

                int year = START_YEAR + y;
                double total = monthTotals[y][m];
                double budget = db.getBudget(m + 1, year);
                string status = "Not set";
                if (budget > 0) {
                    monthsWithBudget++;
                    if (total > budget) { status = "Over budget"; monthsOverBudget++; }
                    else                { status = "Within budget"; }
                }

                if (activeMonths > 0) rows << ",";
                rows << "{\"month\":\"" << MONTH_NAMES[m] << "\",\"year\":" << year
                     << ",\"count\":" << monthCounts[y][m]
                     << ",\"total\":" << money(total)
                     << ",\"budget\":" << money(budget)
                     << ",\"status\":\"" << status << "\"}";

                activeMonths++;
                activeTotal += total;
                if (activeMonths == 1 || total > highestMonthTotal) {
                    highestMonthTotal = total; highestMonth = m; highestYear = year;
                }
                if (activeMonths == 1 || total < lowestMonthTotal) {
                    lowestMonthTotal = total; lowestMonth = m; lowestYear = year;
                }
            }
        }

        int topCategory = 0;
        for (int i = 1; i < NUMBER_OF_CATEGORIES; i++) {
            if (categoryTotals[i] > categoryTotals[topCategory]) topCategory = i;
        }

        string comparison = "Spending in 2026 and 2027 is equal.";
        if (yearTotals[1] > yearTotals[0]) comparison = "Spending in 2027 is higher than in 2026.";
        else if (yearTotals[1] < yearTotals[0]) comparison = "Spending in 2026 is higher than in 2027.";

        ostringstream out;
        out << "{\"count\":" << all.size()
            << ",\"overall\":" << money(overallTotal)
            << ",\"year2026\":" << money(yearTotals[0])
            << ",\"year2027\":" << money(yearTotals[1])
            << ",\"categoryTotals\":[";
        for (int i = 0; i < NUMBER_OF_CATEGORIES; i++) {
            if (i > 0) out << ",";
            out << "{\"name\":\"" << CATEGORY_NAMES[i] << "\",\"amount\":"
                << money(categoryTotals[i]) << "}";
        }
        out << "],\"months\":[" << rows.str() << "]"
            << ",\"insights\":{"
            << "\"averagePerMonth\":" << money(activeTotal / activeMonths)
            << ",\"highestMonth\":{\"name\":\"" << MONTH_NAMES[highestMonth] << " " << highestYear
            << "\",\"total\":" << money(highestMonthTotal) << "}"
            << ",\"lowestMonth\":{\"name\":\"" << MONTH_NAMES[lowestMonth] << " " << lowestYear
            << "\",\"total\":" << money(lowestMonthTotal) << "}"
            << ",\"topCategory\":{\"name\":\"" << CATEGORY_NAMES[topCategory]
            << "\",\"amount\":" << money(categoryTotals[topCategory])
            << ",\"percent\":" << money(categoryTotals[topCategory] / overallTotal * 100) << "}"
            << ",\"biggest\":{\"amount\":" << money(all[biggestIndex].getAmount())
            << ",\"description\":\"" << jsonEscape(all[biggestIndex].getDescription())
            << "\",\"date\":\"" << all[biggestIndex].getDateString() << "\"}"
            << ",\"monthsWithBudget\":" << monthsWithBudget
            << ",\"monthsOverBudget\":" << monthsOverBudget
            << ",\"yearComparison\":\"" << comparison << "\"}}";
        return out.str();
    }
};

#endif
