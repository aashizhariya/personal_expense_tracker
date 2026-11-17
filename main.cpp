// main.cpp
// Personal Expense Tracker and Monthly Financial Analyzer  (Web version)
// Tracking period : 01 January 2026 to 31 December 2027
// Language        : C++17
// Storage         : SQLite file "expenses.db" (data stays after restart)
//
// Program flow:
//   1. Open the database (creates the tables on first run).
//   2. Create the tracker and budget manager objects.
//   3. Register the API routes and serve the website files.
//   4. Wait for requests from the browser.
#include "httplib.h"
#include <iostream>
#include "Database.h"
#include "ExpenseTracker.h"
#include "BudgetManager.h"
#include "ApiRoutes.h"
using namespace std;

int main() {
    try {
        Database database("expenses.db");
        ExpenseTracker tracker(database);
        BudgetManager budgetManager(database);

        httplib::Server server;

        // The website (HTML, CSS, JS) is served from the frontend folder.
        // Run the program from the backend folder so "../frontend" is found.
        if (!server.set_mount_point("/", "../frontend")) {
            cout << "Warning: frontend folder not found. Run from the backend folder.\n";
        }

        ApiRoutes routes(server, tracker, budgetManager);
        routes.registerRoutes();

        cout << "Expense Tracker is running.\n";
        cout << "Open http://localhost:8080 in your browser.\n";
        cout << "Press Ctrl+C to stop.\n";

        if (!server.listen("localhost", 8080)) {
            cout << "Could not start the server. Is port 8080 already in use?\n";
            return 1;
        }
    } catch (const exception& e) {
        cout << "Fatal error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
