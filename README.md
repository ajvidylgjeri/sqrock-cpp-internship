# Sqrock IT Solutions — C++ Internship (Phase 1)

Console applications built with **Core C++**, demonstrating OOP, the STL and file handling. Three projects completed (the phase requires any 3).

| # | Project | Folder | Highlights |
|---|---------|--------|-----------|
| 1 | Cab Booking App | [`cab-booking-app`](./cab-booking-app) | Register/login, book cabs, fare calc by distance & type, night surcharge, trip history & cancel |
| 2 | Banking Management System | [`banking-management-system`](./banking-management-system) | Accounts, PIN login, deposit/withdraw/transfer, transaction history, file storage |
| 3 | Employee Payroll System | [`employee-payroll-system`](./employee-payroll-system) | Employees, salary (HRA/DA, tax, PF), attendance-based net pay, payroll report |

## 🛠️ Requirements
- A C++17 compiler (developed on g++ 13)

## 🚀 Build & run any project
```bash
cd cab-booking-app                       # or another project folder
g++ -std=c++17 -o app cab_booking.cpp    # banking.cpp / payroll.cpp for the others
./app
```
Each program is menu-driven and stores its data in local text files, so state persists between runs.

## 📚 Concepts demonstrated
Core C++ · OOP (classes, members, methods) · STL (`vector`, `string`, `stringstream`) · File handling (`ifstream`/`ofstream`) · Fare & payroll calculations · Input validation

## 👤 Author
**Ajvi Dylgjeri** — Software Engineering student, University of New York Tirana.

---
*Built for the Sqrock IT Solutions internship program.*
