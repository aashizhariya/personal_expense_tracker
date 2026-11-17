// ApiRoutes.h
// Class : ApiRoutes -> connects web addresses (URLs) to C++ functions.
// The browser sends an HTTP request, the matching route calls the tracker,
// and the answer goes back as JSON text.  (REST API)
// Concepts: REST API, HTTP methods (GET/POST/PUT/DELETE), JSON, exceptions,
//           lambda functions (short unnamed functions used as handlers).
#ifndef APIROUTES_H
#define APIROUTES_H

#include <string>
#include <functional>
#include <stdexcept>
#include "httplib.h"
#include "ExpenseTracker.h"
#include "BudgetManager.h"
using namespace std;

class ApiRoutes {
private:
    httplib::Server& server;
    ExpenseTracker& tracker;
    BudgetManager& budgets;

    // ---------- Helpers to send answers ----------
    static void sendJson(httplib::Response& res, const string& json, int status) {
        res.status = status;
        res.set_content(json, "application/json");
    }

    static void sendError(httplib::Response& res, const string& message, int status) {
        sendJson(res, "{\"error\":\"" + jsonEscape(message) + "\"}", status);
    }

    // ---------- Helpers to read values sent by the browser ----------
    static string getText(const httplib::Request& req, const string& key) {
        if (!req.has_param(key)) return "";
        return trimText(req.get_param_value(key));
    }

    static int getInt(const httplib::Request& req, const string& key, int defaultValue = 0) {
        string value = getText(req, key);
        if (value.empty()) return defaultValue;
        try {
            size_t used = 0;
            int number = stoi(value, &used);
            if (used != value.size()) throw invalid_argument("extra characters");
            return number;
        } catch (const exception&) {
            throw invalid_argument("'" + key + "' must be a whole number.");
        }
    }

    static double getDouble(const httplib::Request& req, const string& key, double defaultValue = 0) {
        string value = getText(req, key);
        if (value.empty()) return defaultValue;
        try {
            size_t used = 0;
            double number = stod(value, &used);
            if (used != value.size()) throw invalid_argument("extra characters");
            return number;
        } catch (const exception&) {
            throw invalid_argument("'" + key + "' must be a number.");
        }
    }

    // Runs one handler inside try/catch so that every error becomes a
    // proper JSON error message instead of crashing the server.
    //   invalid_argument -> 400 (wrong input)
    //   out_of_range     -> 404 (record not found)
    //   other errors     -> 500 (server / database problem)
    void run(httplib::Response& res, const function<string()>& action, int okStatus = 200) {
        try {
            sendJson(res, action(), okStatus);
        } catch (const invalid_argument& e) {
            sendError(res, e.what(), 400);
        } catch (const out_of_range& e) {
            sendError(res, e.what(), 404);
        } catch (const exception& e) {
            sendError(res, string("Server error: ") + e.what(), 500);
        }
    }

public:
    ApiRoutes(httplib::Server& s, ExpenseTracker& t, BudgetManager& b)
        : server(s), tracker(t), budgets(b) {}

    void registerRoutes() {

        // ---------- GET /api/expenses : list with search, filters, sorting ----------
        server.Get("/api/expenses", [this](const httplib::Request& req, httplib::Response& res) {
            run(res, [&]() {
                ExpenseFilter filter;
                filter.search = getText(req, "search");
                filter.category = getText(req, "category");
                filter.month = getInt(req, "month", 0);
                filter.year = getInt(req, "year", 0);
                filter.minAmount = getDouble(req, "minAmount", -1);
                filter.maxAmount = getDouble(req, "maxAmount", -1);
                if (!getText(req, "sortBy").empty()) filter.sortBy = getText(req, "sortBy");
                if (!getText(req, "order").empty())  filter.order = getText(req, "order");

                vector<Expense> list = tracker.searchAndFilter(filter);

                double total = 0;
                string items;
                for (size_t i = 0; i < list.size(); i++) {
                    total += list[i].getAmount();
                    if (i > 0) items += ",";
                    items += list[i].toJson();
                }
                return "{\"count\":" + to_string(list.size()) +
                       ",\"total\":" + money(total) +
                       ",\"expenses\":[" + items + "]}";
            });
        });

        // ---------- POST /api/expenses : add a new expense ----------
        server.Post("/api/expenses", [this](const httplib::Request& req, httplib::Response& res) {
            run(res, [&]() {
                int id = tracker.addExpense(getDouble(req, "amount"), getText(req, "category"),
                                            getInt(req, "day"), getInt(req, "month"),
                                            getInt(req, "year"), getText(req, "description"));
                return "{\"message\":\"Expense added successfully.\",\"id\":" + to_string(id) + "}";
            }, 201);
        });

        // ---------- PUT /api/expenses/<id> : edit an expense ----------
        server.Put(R"(/api/expenses/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
            run(res, [&]() {
                int id = stoi(req.matches[1].str());
                tracker.updateExpense(id, getDouble(req, "amount"), getText(req, "category"),
                                      getInt(req, "day"), getInt(req, "month"),
                                      getInt(req, "year"), getText(req, "description"));
                return string("{\"message\":\"Expense updated successfully.\"}");
            });
        });

        // ---------- DELETE /api/expenses/<id> : delete an expense ----------
        server.Delete(R"(/api/expenses/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
            run(res, [&]() {
                int id = stoi(req.matches[1].str());
                tracker.deleteExpense(id);
                return string("{\"message\":\"Expense deleted successfully.\"}");
            });
        });

        // ---------- GET /api/analysis/monthly?month=..&year=.. ----------
        server.Get("/api/analysis/monthly", [this](const httplib::Request& req, httplib::Response& res) {
            run(res, [&]() {
                return tracker.monthlyAnalysis(getInt(req, "month"), getInt(req, "year"));
            });
        });

        // ---------- GET /api/analysis/category?month=..&year=.. ----------
        server.Get("/api/analysis/category", [this](const httplib::Request& req, httplib::Response& res) {
            run(res, [&]() {
                return tracker.categoryAnalysis(getInt(req, "month"), getInt(req, "year"));
            });
        });

        // ---------- GET /api/analysis/trend?year=.. (data for the chart) ----------
        server.Get("/api/analysis/trend", [this](const httplib::Request& req, httplib::Response& res) {
            run(res, [&]() {
                return tracker.yearlyTrend(getInt(req, "year"));
            });
        });

        // ---------- GET /api/summary : financial summary 2026-2027 ----------
        server.Get("/api/summary", [this](const httplib::Request&, httplib::Response& res) {
            run(res, [&]() {
                return tracker.financialSummary();
            });
        });

        // ---------- POST /api/budget : set the budget of a month ----------
        server.Post("/api/budget", [this](const httplib::Request& req, httplib::Response& res) {
            run(res, [&]() {
                budgets.setBudget(getInt(req, "month"), getInt(req, "year"), getDouble(req, "amount"));
                return string("{\"message\":\"Budget saved successfully.\"}");
            });
        });

        // ---------- GET /api/budget/compare?month=..&year=.. ----------
        server.Get("/api/budget/compare", [this](const httplib::Request& req, httplib::Response& res) {
            run(res, [&]() {
                int month = getInt(req, "month");
                int year = getInt(req, "year");
                double spent = tracker.getMonthTotal(month, year);   // also validates month/year
                return budgets.compare(month, year, spent);
            });
        });

        // ---------- GET /api/budgets : all saved budgets ----------
        server.Get("/api/budgets", [this](const httplib::Request&, httplib::Response& res) {
            run(res, [&]() {
                return budgets.allBudgetsJson();
            });
        });
    }
};

#endif
