// Banking Management System — Sqrock IT Solutions C++ Internship (Phase 1, Task 2)
//
// A console banking application demonstrating Core C++, OOP, the STL and file
// handling. Create accounts, deposit/withdraw/transfer, and view a transaction
// history. Includes PIN validation and balance safeguards. Data persists to
// text files.
//
// Build:  g++ -std=c++17 -o bank banking.cpp
// Run:    ./bank
//
// Author: Ajvi Dylgjeri

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <ctime>

using namespace std;

class Account {
public:
    int number;
    string name, pin, type;
    double balance;

    Account() : number(0), balance(0) {}
    Account(int n, string nm, string p, string t, double b)
        : number(n), name(nm), pin(p), type(t), balance(b) {}

    string serialize() const {
        stringstream ss;
        ss << number << '|' << name << '|' << pin << '|' << type << '|' << balance;
        return ss.str();
    }
};

const string ACC_FILE = "accounts.txt";
const string TXN_FILE = "transactions.txt";

vector<string> split(const string& s, char d) {
    vector<string> out; string cur; stringstream ss(s);
    while (getline(ss, cur, d)) out.push_back(cur);
    return out;
}
string prompt(const string& m) { cout << m; string s; getline(cin, s); return s; }
double promptNum(const string& m) {
    while (true) { cout << m; string s; getline(cin, s);
        try { return stod(s); } catch (...) { cout << "Enter a valid number.\n"; } }
}
string now() {
    time_t t = time(nullptr); char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
    return string(buf);
}

vector<Account> loadAccounts() {
    vector<Account> v; ifstream f(ACC_FILE); string line;
    while (getline(f, line)) { if (line.empty()) continue;
        auto p = split(line, '|');
        if (p.size() >= 5) v.push_back(Account(stoi(p[0]), p[1], p[2], p[3], stod(p[4])));
    }
    return v;
}
void saveAccounts(const vector<Account>& v) {
    ofstream f(ACC_FILE); for (auto& a : v) f << a.serialize() << '\n';
}
void logTxn(int number, const string& kind, double amount, double balance) {
    ofstream f(TXN_FILE, ios::app);
    f << number << '|' << now() << '|' << kind << '|' << amount << '|' << balance << '\n';
}
void showHistory(int number) {
    ifstream f(TXN_FILE); string line; bool any = false;
    cout << fixed << setprecision(2);
    cout << "\nTransaction history for account " << number << ":\n";
    while (getline(f, line)) {
        auto p = split(line, '|');
        if (p.size() >= 5 && stoi(p[0]) == number) {
            cout << "  " << p[1] << " | " << setw(18) << left << p[2]
                 << " amount " << stod(p[3]) << " | balance " << stod(p[4]) << "\n";
            any = true;
        }
    }
    if (!any) cout << "  (no transactions yet)\n";
}

int nextNumber(const vector<Account>& v) {
    int m = 500100; for (auto& a : v) m = max(m, a.number + 1); return m;
}
Account* findAcc(vector<Account>& v, int num) {
    for (auto& a : v) if (a.number == num) return &a;
    return nullptr;
}

void session(Account* a, vector<Account>& accs) {
    cout << "\nWelcome, " << a->name << "!\n";
    cout << fixed << setprecision(2);
    while (true) {
        cout << "\n--- ACCOUNT MENU (" << a->name << ") ---\n"
             << "1. Balance\n2. Deposit\n3. Withdraw\n4. Transfer\n5. History\n0. Logout\n";
        string c = prompt("Choose: ");
        if (c == "1") {
            cout << "Balance: " << a->balance << "\n";
        } else if (c == "2") {
            double amt = promptNum("Deposit amount: ");
            if (amt <= 0) { cout << "Must be positive.\n"; continue; }
            a->balance += amt; saveAccounts(accs); logTxn(a->number, "DEPOSIT", amt, a->balance);
            cout << "Deposited. New balance: " << a->balance << "\n";
        } else if (c == "3") {
            double amt = promptNum("Withdraw amount: ");
            if (amt <= 0) { cout << "Must be positive.\n"; continue; }
            if (amt > a->balance) { cout << "Insufficient funds.\n"; continue; }
            a->balance -= amt; saveAccounts(accs); logTxn(a->number, "WITHDRAW", amt, a->balance);
            cout << "Withdrawn. New balance: " << a->balance << "\n";
        } else if (c == "4") {
            string rs = prompt("Recipient account number: ");
            int rn; try { rn = stoi(rs); } catch (...) { cout << "Invalid number.\n"; continue; }
            Account* r = findAcc(accs, rn);
            if (!r) { cout << "Recipient not found.\n"; continue; }
            if (r->number == a->number) { cout << "Cannot transfer to self.\n"; continue; }
            double amt = promptNum("Transfer amount: ");
            if (amt <= 0) { cout << "Must be positive.\n"; continue; }
            if (amt > a->balance) { cout << "Insufficient funds.\n"; continue; }
            a->balance -= amt; r->balance += amt; saveAccounts(accs);
            logTxn(a->number, "TRANSFER-OUT", amt, a->balance);
            logTxn(r->number, "TRANSFER-IN", amt, r->balance);
            cout << "Transferred " << amt << " to " << r->name << ". Balance: " << a->balance << "\n";
        } else if (c == "5") {
            showHistory(a->number);
        } else if (c == "0") {
            break;
        } else cout << "Invalid option.\n";
    }
}

int main() {
    vector<Account> accs = loadAccounts();
    cout << "=====================================\n";
    cout << "    SQROCK BANKING MANAGEMENT SYSTEM\n";
    cout << "=====================================\n";
    while (true) {
        cout << "\n1. Create account\n2. Login\n3. Admin: list accounts\n0. Exit\n";
        string c = prompt("Choose: ");
        if (c == "1") {
            string name = prompt("Holder name: ");
            if (name.empty()) { cout << "Name required.\n"; continue; }
            string type = prompt("Type (Savings/Current): ");
            if (type != "Savings" && type != "Current") type = "Savings";
            string pin = prompt("Set a 4-digit PIN: ");
            double opening = promptNum("Opening deposit: ");
            int num = nextNumber(accs);
            accs.push_back(Account(num, name, pin, type, opening));
            saveAccounts(accs); logTxn(num, "OPEN", opening, opening);
            cout << "\nAccount created! Number: " << num << " (keep it safe)\n";
        } else if (c == "2") {
            string ns = prompt("Account number: ");
            int num; try { num = stoi(ns); } catch (...) { cout << "Invalid number.\n"; continue; }
            Account* a = findAcc(accs, num);
            if (!a) { cout << "No such account.\n"; continue; }
            string pin = prompt("PIN: ");
            if (a->pin != pin) { cout << "Incorrect PIN.\n"; continue; }
            session(a, accs);
        } else if (c == "3") {
            if (accs.empty()) { cout << "No accounts.\n"; continue; }
            cout << fixed << setprecision(2);
            cout << "\n" << left << setw(10) << "NUMBER" << setw(20) << "NAME"
                 << setw(10) << "TYPE" << right << setw(12) << "BALANCE" << "\n";
            cout << string(52, '-') << "\n";
            for (auto& a : accs)
                cout << left << setw(10) << a.number << setw(20) << a.name
                     << setw(10) << a.type << right << setw(12) << a.balance << "\n";
        } else if (c == "0") {
            break;
        } else cout << "Invalid option.\n";
    }
    cout << "Thank you for banking with Sqrock!\n";
    return 0;
}
