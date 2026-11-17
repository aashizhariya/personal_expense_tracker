# Build with:  make        (needs libsqlite3-dev and httplib.h in this folder)
CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread

tracker: main.cpp Utils.h Expense.h Database.h ExpenseTracker.h BudgetManager.h ApiRoutes.h httplib.h
	$(CXX) $(CXXFLAGS) main.cpp -o tracker -lsqlite3

clean:
	rm -f tracker
