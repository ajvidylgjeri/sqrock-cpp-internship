# Cab Booking App (C++)

Console cab-booking application — Sqrock IT Solutions C++ Internship, Phase 1 Task 1.

## Features
- User registration & login (phone as ID)
- Book a cab: choose type (Mini / Sedan / SUV), pickup & drop, distance
- Fare calculation: flag-down + per-km rate by cab type, optional 25% night surcharge
- View trip history and cancel booked trips
- Data persisted to `users.txt` and `trips.txt`

## Build & run
```bash
g++ -std=c++17 -o cab cab_booking.cpp
./cab
```

## Concepts
Core C++ · OOP · STL (`vector`, `string`, `stringstream`) · file handling · fare logic.

**Author:** Ajvi Dylgjeri
