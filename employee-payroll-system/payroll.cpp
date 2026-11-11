// Employee Payroll System — Sqrock IT Solutions C++ Internship (Phase 1, Task 3)
//
// A console payroll application demonstrating Core C++, OOP, the STL and file
// handling. Manage employees, compute salary (basic + allowances - deductions),
// record attendance-based pay, and generate a payroll report. Data persists to
// a text file.
//
// Build:  g++ -std=c++17 -o payroll payroll.cpp
// Run:    ./payroll
//
// Author: Ajvi Dylgjeri

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>

using namespace std;

class Employee {
public:
    int id;
    string name, department, designation;
    double basic;          // monthly basic salary
    int presentDays;       // out of workingDays
    static const int WORKING_DAYS = 22;

    Employee() : id(0), basic(0), presentDays(0) {}
    Employee(int i, string n, string d, string des, double b, int p)
        : id(i), name(n), department(d), designation(des), basic(b), presentDays(p) {}

    double hra()        const { return basic * 0.20; }           // house rent allowance
    double da()         const { return basic * 0.10; }           // dearness allowance
    double grossFull()  const { return basic + hra() + da(); }
    double tax()        const { return grossFull() * 0.05; }     // 5% tax
    double pf()         const { return basic * 0.08; }           // provident fund
    double attendanceFactor() const {
        return WORKING_DAYS == 0 ? 1.0 : (double)presentDays / WORKING_DAYS;
    }
    double netPay() const {
        double gross = grossFull() * attendanceFactor();
        return gross - tax() - pf();
    }
    string serialize() const {
        stringstream ss;
        ss << id << '|' << name << '|' << department << '|' << designation
           << '|' << basic << '|' << presentDays;
        return ss.str();
    }
};

const string EMP_FILE = "employees.txt";

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
int promptInt(const string& m) { return (int)promptNum(m); }

vector<Employee> load() {
    vector<Employee> v; ifstream f(EMP_FILE); string line;
    while (getline(f, line)) { if (line.empty()) continue;
        auto p = split(line, '|');
        if (p.size() >= 6)
            v.push_back(Employee(stoi(p[0]), p[1], p[2], p[3], stod(p[4]), stoi(p[5])));
    }
    return v;
}
void save(const vector<Employee>& v) {
    ofstream f(EMP_FILE); for (auto& e : v) f << e.serialize() << '\n';
}
int nextId(const vector<Employee>& v) { int m = 100; for (auto& e : v) m = max(m, e.id); return m + 1; }

void addEmployee(vector<Employee>& v) {
    string name = prompt("Name: ");
    if (name.empty()) { cout << "Name required.\n"; return; }
    string dept = prompt("Department: ");
    string desig = prompt("Designation: ");
    double basic = promptNum("Basic monthly salary: ");
    int present = promptInt("Days present (out of 22): ");
    if (present < 0) present = 0;
    if (present > Employee::WORKING_DAYS) present = Employee::WORKING_DAYS;
    int id = nextId(v);
    v.push_back(Employee(id, name, dept, desig, basic, present));
    save(v);
    cout << "Employee added with ID " << id << ".\n";
}

void payslip(const Employee& e) {
    cout << fixed << setprecision(2);
    cout << "\n------- PAYSLIP -------\n";
    cout << "ID          : " << e.id << "\n";
    cout << "Name        : " << e.name << "\n";
    cout << "Department  : " << e.department << " (" << e.designation << ")\n";
    cout << "Attendance  : " << e.presentDays << "/" << Employee::WORKING_DAYS << "\n";
    cout << "Basic       : " << e.basic << "\n";
    cout << "HRA (20%)   : " << e.hra() << "\n";
    cout << "DA  (10%)   : " << e.da() << "\n";
    cout << "Gross(full) : " << e.grossFull() << "\n";
    cout << "Tax (5%)    : -" << e.tax() << "\n";
    cout << "PF  (8%)    : -" << e.pf() << "\n";
    cout << "Net pay     : " << e.netPay() << "\n";
    cout << "----------------------\n";
}

void report(const vector<Employee>& v) {
    if (v.empty()) { cout << "No employees.\n"; return; }
    cout << fixed << setprecision(2);
    cout << "\n" << left << setw(5) << "ID" << setw(18) << "NAME" << setw(14) << "DEPT"
         << right << setw(12) << "BASIC" << setw(12) << "NET PAY" << "\n";
    cout << string(61, '-') << "\n";
    double total = 0;
    for (auto& e : v) {
        cout << left << setw(5) << e.id << setw(18) << e.name << setw(14) << e.department
             << right << setw(12) << e.basic << setw(12) << e.netPay() << "\n";
        total += e.netPay();
    }
    cout << string(61, '-') << "\n";
    cout << left << setw(37) << "TOTAL PAYROLL" << right << setw(24) << total << "\n";
}

Employee* find(vector<Employee>& v, int id) {
    for (auto& e : v) if (e.id == id) return &e;
    return nullptr;
}

int main() {
    vector<Employee> emps = load();
    cout << "=====================================\n";
    cout << "    SQROCK EMPLOYEE PAYROLL SYSTEM\n";
    cout << "=====================================\n";
    while (true) {
        cout << "\n1. Add employee\n2. List employees\n3. Generate payslip\n"
             << "4. Payroll report\n5. Update attendance\n0. Exit\n";
        string c = prompt("Choose: ");
        if (c == "1") addEmployee(emps);
        else if (c == "2") {
            if (emps.empty()) { cout << "No employees.\n"; continue; }
            for (auto& e : emps)
                cout << "  #" << e.id << "  " << e.name << " — " << e.designation
                     << " (" << e.department << ")\n";
        } else if (c == "3") {
            int id = promptInt("Employee ID: ");
            Employee* e = find(emps, id);
            if (e) payslip(*e); else cout << "Not found.\n";
        } else if (c == "4") {
            report(emps);
        } else if (c == "5") {
            int id = promptInt("Employee ID: ");
            Employee* e = find(emps, id);
            if (!e) { cout << "Not found.\n"; continue; }
            int d = promptInt("Days present (0-22): ");
            if (d < 0) d = 0;
            if (d > Employee::WORKING_DAYS) d = Employee::WORKING_DAYS;
            e->presentDays = d; save(emps);
            cout << "Attendance updated.\n";
        } else if (c == "0") {
            break;
        } else cout << "Invalid option.\n";
    }
    cout << "Payroll session ended.\n";
    return 0;
}
