// Cab Booking App — Sqrock IT Solutions C++ Internship (Phase 1, Task 1)
//
// A console cab-booking application demonstrating Core C++, OOP, the STL and
// file handling. Users register/login, book cabs with fare calculation by
// distance and cab type, and view/cancel trips. Data persists to text files.
//
// Build:  g++ -std=c++17 -o cab cab_booking.cpp
// Run:    ./cab
//
// Author: Ajvi Dylgjeri

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <limits>

using namespace std;

struct User {
    string name, phone, password;
};

struct Trip {
    int id;
    string user;      // phone (unique key)
    string cabType;
    string pickup, drop;
    double distanceKm;
    double fare;
    string status;    // Booked / Completed / Cancelled
};

const string USER_FILE = "users.txt";
const string TRIP_FILE = "trips.txt";

// ---------- helpers ----------
vector<string> split(const string& s, char d) {
    vector<string> out; string cur; stringstream ss(s);
    while (getline(ss, cur, d)) out.push_back(cur);
    return out;
}

string prompt(const string& msg) {
    cout << msg; string s; getline(cin, s); return s;
}

double promptNum(const string& msg) {
    while (true) {
        cout << msg; string s; getline(cin, s);
        try { return stod(s); } catch (...) { cout << "Enter a valid number.\n"; }
    }
}

// ---------- persistence ----------
vector<User> loadUsers() {
    vector<User> v; ifstream f(USER_FILE); string line;
    while (getline(f, line)) {
        if (line.empty()) continue;
        auto p = split(line, '|');
        if (p.size() >= 3) v.push_back({p[0], p[1], p[2]});
    }
    return v;
}
void saveUsers(const vector<User>& v) {
    ofstream f(USER_FILE);
    for (auto& u : v) f << u.name << '|' << u.phone << '|' << u.password << '\n';
}

vector<Trip> loadTrips() {
    vector<Trip> v; ifstream f(TRIP_FILE); string line;
    while (getline(f, line)) {
        if (line.empty()) continue;
        auto p = split(line, '|');
        if (p.size() >= 8)
            v.push_back({stoi(p[0]), p[1], p[2], p[3], p[4], stod(p[5]), stod(p[6]), p[7]});
    }
    return v;
}
void saveTrips(const vector<Trip>& v) {
    ofstream f(TRIP_FILE);
    for (auto& t : v)
        f << t.id << '|' << t.user << '|' << t.cabType << '|' << t.pickup << '|'
          << t.drop << '|' << t.distanceKm << '|' << t.fare << '|' << t.status << '\n';
}

// ---------- fare ----------
double baseRate(const string& type) {
    if (type == "Mini")   return 0.8;   // per km
    if (type == "Sedan")  return 1.2;
    if (type == "SUV")    return 1.8;
    return 1.0;
}
double calcFare(const string& type, double km, bool night) {
    double base = 2.5;                         // flag-down
    double fare = base + baseRate(type) * km;
    if (night) fare *= 1.25;                    // 25% night surcharge
    return fare;
}

// ---------- app ----------
int nextTripId(const vector<Trip>& trips) {
    int m = 5000; for (auto& t : trips) m = max(m, t.id); return m + 1;
}

void book(const User& u, vector<Trip>& trips) {
    cout << "\nCab types: Mini (0.8/km), Sedan (1.2/km), SUV (1.8/km)\n";
    string type = prompt("Choose cab type: ");
    if (type != "Mini" && type != "Sedan" && type != "SUV") { cout << "Invalid type.\n"; return; }
    string pickup = prompt("Pickup location: ");
    string drop   = prompt("Drop location: ");
    double km     = promptNum("Distance (km): ");
    if (km <= 0) { cout << "Distance must be positive.\n"; return; }
    string n = prompt("Night trip? (y/n): ");
    bool night = (n == "y" || n == "Y");
    double fare = calcFare(type, km, night);

    Trip t{nextTripId(trips), u.phone, type, pickup, drop, km, fare, "Booked"};
    trips.push_back(t); saveTrips(trips);
    cout << fixed << setprecision(2);
    cout << "\n--- BOOKING CONFIRMED ---\n";
    cout << "Trip ID   : " << t.id << "\n";
    cout << "Route     : " << pickup << " -> " << drop << " (" << km << " km)\n";
    cout << "Cab       : " << type << (night ? "  [night +25%]" : "") << "\n";
    cout << "Fare      : " << fare << "\n";
}

void myTrips(const User& u, const vector<Trip>& trips) {
    cout << "\nYour trips:\n";
    bool any = false;
    cout << fixed << setprecision(2);
    for (auto& t : trips) if (t.user == u.phone) {
        cout << "  #" << t.id << "  " << t.cabType << "  " << t.pickup << " -> " << t.drop
             << "  " << t.distanceKm << "km  fare " << t.fare << "  [" << t.status << "]\n";
        any = true;
    }
    if (!any) cout << "  (no trips yet)\n";
}

void cancelTrip(const User& u, vector<Trip>& trips) {
    string ids = prompt("Trip ID to cancel: ");
    int id; try { id = stoi(ids); } catch (...) { cout << "Invalid ID.\n"; return; }
    for (auto& t : trips) if (t.id == id && t.user == u.phone) {
        if (t.status != "Booked") { cout << "Only booked trips can be cancelled.\n"; return; }
        t.status = "Cancelled"; saveTrips(trips);
        cout << "Trip #" << id << " cancelled.\n"; return;
    }
    cout << "Trip not found.\n";
}

void session(const User& u, vector<Trip>& trips) {
    cout << "\nWelcome, " << u.name << "!\n";
    while (true) {
        cout << "\n--- MENU (" << u.name << ") ---\n"
             << "1. Book a cab\n2. My trips\n3. Cancel a trip\n0. Logout\n";
        string c = prompt("Choose: ");
        if (c == "1") book(u, trips);
        else if (c == "2") myTrips(u, trips);
        else if (c == "3") cancelTrip(u, trips);
        else if (c == "0") break;
        else cout << "Invalid option.\n";
    }
}

int main() {
    vector<User> users = loadUsers();
    vector<Trip> trips = loadTrips();
    cout << "=====================================\n";
    cout << "        SQROCK CAB BOOKING APP\n";
    cout << "=====================================\n";

    while (true) {
        cout << "\n1. Register\n2. Login\n0. Exit\n";
        string c = prompt("Choose: ");
        if (c == "1") {
            string name = prompt("Name: ");
            string phone = prompt("Phone (used as login ID): ");
            bool exists = false;
            for (auto& u : users) if (u.phone == phone) exists = true;
            if (exists) { cout << "Phone already registered.\n"; continue; }
            string pw = prompt("Password: ");
            users.push_back({name, phone, pw}); saveUsers(users);
            cout << "Registered! You can now log in.\n";
        } else if (c == "2") {
            string phone = prompt("Phone: ");
            string pw = prompt("Password: ");
            User* found = nullptr;
            for (auto& u : users) if (u.phone == phone && u.password == pw) found = &u;
            if (found) session(*found, trips);
            else cout << "Invalid credentials.\n";
        } else if (c == "0") {
            break;
        } else cout << "Invalid option.\n";
    }
    cout << "Thanks for riding with Sqrock Cabs!\n";
    return 0;
}
