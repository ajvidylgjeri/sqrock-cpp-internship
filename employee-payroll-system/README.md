# Employee Payroll System (C++)

Console payroll application — Sqrock IT Solutions C++ Internship, Phase 1 Task 3.

## Features
- Add and list employees (department, designation, basic salary)
- Salary breakdown: HRA (20%), DA (10%), tax (5%), provident fund (8%)
- Attendance-based net pay (present days out of 22 working days)
- Generate individual payslips
- Payroll report with total payroll across all employees
- Update attendance; data persisted to `employees.txt`

## Build & run
```bash
g++ -std=c++17 -o payroll payroll.cpp
./payroll
```

## Concepts
Core C++ · OOP (class `Employee`, member calculations) · STL · file handling · formatted output.

**Author:** Ajvi Dylgjeri
