// Database.h
// Class : Database -> the ONLY class that talks to SQLite.
// Concepts: database connectivity, SQL, CRUD, prepared statements,
//           exception handling, constructor/destructor.
#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <stdexcept>
#include <sqlite3.h>
#include "Expense.h"
using namespace std;

// Holds the search / filter / sort options sent from the website.
// A value of 0 (month, year), "" (text) or -1 (amounts) means "do not filter".
struct ExpenseFilter {
    string search;      // text to look for in description or category
    string category;    // exact category
    int month;          // 1-12, or 0 for all
    int year;           // 2026/2027, or 0 for all
    double minAmount;   // -1 means no minimum
    double maxAmount;   // -1 means no maximum
    string sortBy;      // "date", "amount" or "category"
    string order;       // "asc" or "desc"

    ExpenseFilter() {
        search = ""; category = ""; month = 0; year = 0;
        minAmount = -1; maxAmount = -1;
        sortBy = "date"; order = "desc";
    }
};

// One row of the budgets table
struct Budget {
    int month;
    int year;
    double amount;
};

class Database {
private:
    sqlite3* db;   // connection to the database file

    // Runs an SQL command that returns no rows (e.g. CREATE TABLE)
    void run(const string& sql) {
        char* errorText = nullptr;
        if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errorText) != SQLITE_OK) {
            string message = errorText ? errorText : "Unknown database error";
            sqlite3_free(errorText);
            throw runtime_error(message);
        }
    }

    // Compiles an SQL command that contains ? placeholders
    sqlite3_stmt* prepare(const string& sql) {
        sqlite3_stmt* statement = nullptr;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &statement, nullptr) != SQLITE_OK) {
            throw runtime_error(string("SQL error: ") + sqlite3_errmsg(db));
        }
        return statement;
    }

    // Converts the current row of a SELECT into an Expense object
    Expense readRow(sqlite3_stmt* s) {
        int id = sqlite3_column_int(s, 0);
        double amount = sqlite3_column_double(s, 1);
        const unsigned char* cat = sqlite3_column_text(s, 2);
        int day = sqlite3_column_int(s, 3);
        int month = sqlite3_column_int(s, 4);
        int year = sqlite3_column_int(s, 5);
        const unsigned char* desc = sqlite3_column_text(s, 6);
        string category = cat ? reinterpret_cast<const char*>(cat) : "";
        string description = desc ? reinterpret_cast<const char*>(desc) : "";
        return Expense(id, amount, category, day, month, year, description);
    }

    // Creates the two tables if they do not exist yet
    void createTables() {
        run("CREATE TABLE IF NOT EXISTS expenses ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "amount REAL NOT NULL,"
            "category TEXT NOT NULL,"
            "day INTEGER NOT NULL,"
            "month INTEGER NOT NULL,"
            "year INTEGER NOT NULL,"
            "description TEXT NOT NULL);");
        run("CREATE TABLE IF NOT EXISTS budgets ("
            "year INTEGER NOT NULL,"
            "month INTEGER NOT NULL,"
            "amount REAL NOT NULL,"
            "PRIMARY KEY (year, month));");
    }

public:
    // Constructor: opens (or creates) the database file and the tables
    Database(const string& fileName) {
        db = nullptr;
        if (sqlite3_open(fileName.c_str(), &db) != SQLITE_OK) {
            throw runtime_error("Cannot open database file: " + fileName);
        }
        createTables();
    }

    // Destructor: closes the connection automatically
    ~Database() {
        sqlite3_close(db);
    }

    // A Database object must not be copied (two objects, one connection)
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // ---------------- CREATE ----------------
    // Inserts a new expense. Returns the new id given by SQLite.
    int insertExpense(const Expense& e) {
        sqlite3_stmt* s = prepare(
            "INSERT INTO expenses (amount, category, day, month, year, description) "
            "VALUES (?, ?, ?, ?, ?, ?);");
        sqlite3_bind_double(s, 1, e.getAmount());
        sqlite3_bind_text(s, 2, e.getCategory().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(s, 3, e.getDay());
        sqlite3_bind_int(s, 4, e.getMonth());
        sqlite3_bind_int(s, 5, e.getYear());
        sqlite3_bind_text(s, 6, e.getDescription().c_str(), -1, SQLITE_TRANSIENT);
        int result = sqlite3_step(s);
        string message = sqlite3_errmsg(db);
        sqlite3_finalize(s);
        if (result != SQLITE_DONE) throw runtime_error("Insert failed: " + message);
        return (int)sqlite3_last_insert_rowid(db);
    }

    // ---------------- UPDATE ----------------
    // Returns true if a row was changed, false if the id does not exist.
    bool updateExpense(const Expense& e) {
        sqlite3_stmt* s = prepare(
            "UPDATE expenses SET amount = ?, category = ?, day = ?, month = ?, "
            "year = ?, description = ? WHERE id = ?;");
        sqlite3_bind_double(s, 1, e.getAmount());
        sqlite3_bind_text(s, 2, e.getCategory().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(s, 3, e.getDay());
        sqlite3_bind_int(s, 4, e.getMonth());
        sqlite3_bind_int(s, 5, e.getYear());
        sqlite3_bind_text(s, 6, e.getDescription().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(s, 7, e.getId());
        int result = sqlite3_step(s);
        string message = sqlite3_errmsg(db);
        sqlite3_finalize(s);
        if (result != SQLITE_DONE) throw runtime_error("Update failed: " + message);
        return sqlite3_changes(db) > 0;
    }

    // ---------------- DELETE ----------------
    bool deleteExpense(int id) {
        sqlite3_stmt* s = prepare("DELETE FROM expenses WHERE id = ?;");
        sqlite3_bind_int(s, 1, id);
        int result = sqlite3_step(s);
        string message = sqlite3_errmsg(db);
        sqlite3_finalize(s);
        if (result != SQLITE_DONE) throw runtime_error("Delete failed: " + message);
        return sqlite3_changes(db) > 0;
    }

    // ---------------- READ (search + filter + sort) ----------------
    // The WHERE part is built step by step: every filter that is used adds
    // one "AND ..." condition, so all filters work together.
    vector<Expense> getExpenses(const ExpenseFilter& f) {
        string sql = "SELECT id, amount, category, day, month, year, description "
                     "FROM expenses WHERE 1 = 1";
        string pattern = "%" + f.search + "%";

        if (!f.search.empty())   sql += " AND (description LIKE ? OR category LIKE ?)";
        if (!f.category.empty()) sql += " AND category = ?";
        if (f.month != 0)        sql += " AND month = ?";
        if (f.year != 0)         sql += " AND year = ?";
        if (f.minAmount >= 0)    sql += " AND amount >= ?";
        if (f.maxAmount >= 0)    sql += " AND amount <= ?";

        // Sorting: only fixed words are added (never raw user text), so it is safe.
        string direction = (f.order == "asc") ? "ASC" : "DESC";
        if (f.sortBy == "amount") {
            sql += " ORDER BY amount " + direction + ", id " + direction;
        } else if (f.sortBy == "category") {
            sql += " ORDER BY category " + direction + ", id " + direction;
        } else {
            sql += " ORDER BY year " + direction + ", month " + direction +
                   ", day " + direction + ", id " + direction;
        }

        sqlite3_stmt* s = prepare(sql);

        // Bind the values in the same order in which the ? marks were added
        int index = 1;
        if (!f.search.empty()) {
            sqlite3_bind_text(s, index++, pattern.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(s, index++, pattern.c_str(), -1, SQLITE_TRANSIENT);
        }
        if (!f.category.empty())
            sqlite3_bind_text(s, index++, f.category.c_str(), -1, SQLITE_TRANSIENT);
        if (f.month != 0) sqlite3_bind_int(s, index++, f.month);
        if (f.year != 0)  sqlite3_bind_int(s, index++, f.year);
        if (f.minAmount >= 0) sqlite3_bind_double(s, index++, f.minAmount);
        if (f.maxAmount >= 0) sqlite3_bind_double(s, index++, f.maxAmount);

        vector<Expense> list;
        while (sqlite3_step(s) == SQLITE_ROW) {
            list.push_back(readRow(s));
        }
        sqlite3_finalize(s);
        return list;
    }

    // ---------------- Budgets ----------------
    // INSERT OR REPLACE: adds a new budget or overwrites the old one.
    void setBudget(int month, int year, double amount) {
        sqlite3_stmt* s = prepare(
            "INSERT OR REPLACE INTO budgets (year, month, amount) VALUES (?, ?, ?);");
        sqlite3_bind_int(s, 1, year);
        sqlite3_bind_int(s, 2, month);
        sqlite3_bind_double(s, 3, amount);
        int result = sqlite3_step(s);
        string message = sqlite3_errmsg(db);
        sqlite3_finalize(s);
        if (result != SQLITE_DONE) throw runtime_error("Budget save failed: " + message);
    }

    // Returns the budget of a month, or 0 if no budget was set.
    double getBudget(int month, int year) {
        sqlite3_stmt* s = prepare(
            "SELECT amount FROM budgets WHERE year = ? AND month = ?;");
        sqlite3_bind_int(s, 1, year);
        sqlite3_bind_int(s, 2, month);
        double amount = 0;
        if (sqlite3_step(s) == SQLITE_ROW) {
            amount = sqlite3_column_double(s, 0);
        }
        sqlite3_finalize(s);
        return amount;
    }

    vector<Budget> getAllBudgets() {
        sqlite3_stmt* s = prepare(
            "SELECT year, month, amount FROM budgets ORDER BY year, month;");
        vector<Budget> list;
        while (sqlite3_step(s) == SQLITE_ROW) {
            Budget b;
            b.year = sqlite3_column_int(s, 0);
            b.month = sqlite3_column_int(s, 1);
            b.amount = sqlite3_column_double(s, 2);
            list.push_back(b);
        }
        sqlite3_finalize(s);
        return list;
    }
};

#endif
