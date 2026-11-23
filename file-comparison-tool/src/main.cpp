// File Comparison Tool - Sqrock IT Solutions, C++ Internship Phase 2 (Task 1)
#include <algorithm>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ---------- Options ----------
struct Options {
    bool ignoreWhitespace = false;
    bool ignoreBlank = false;
    bool ignoreCase = false;
};

// ---------- File handling ----------
class FileManager {
public:
    static bool exists(const std::string& p) { return fs::exists(p) && fs::is_regular_file(p); }

    static bool readLines(const std::string& path, std::vector<std::string>& out, std::string& err) {
        std::ifstream in(path);
        if (!in) { err = "Cannot open file: " + path; return false; }
        std::string line;
        while (std::getline(in, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            out.push_back(line);
        }
        return true;
    }

    static void printInfo(const std::string& path) {
        if (!exists(path)) { std::cout << "  [!] File not found: " << path << "\n"; return; }
        std::vector<std::string> lines; std::string err;
        readLines(path, lines, err);
        auto ftime = fs::last_write_time(path);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
        std::time_t t = std::chrono::system_clock::to_time_t(sctp);
        char buf[32]; std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", std::localtime(&t));
        std::cout << "  Path     : " << path << "\n"
                  << "  Size     : " << fs::file_size(path) << " bytes\n"
                  << "  Lines    : " << lines.size() << "\n"
                  << "  Modified : " << buf << "\n"
                  << "  Extension: " << fs::path(path).extension().string() << "\n";
    }
};

// ---------- Diff engine ----------
enum class Kind { Match, Modified, Added, Removed };

struct DiffEntry {
    Kind kind;
    int line1;  // 1-based, 0 if n/a
    int line2;
    std::string text1, text2;
};

struct Summary {
    size_t total1 = 0, total2 = 0, matching = 0, modified = 0, added = 0, removed = 0;
    bool identical() const { return modified == 0 && added == 0 && removed == 0; }
};

class Differ {
    Options opt_;

    std::string normalize(std::string s) const {
        if (opt_.ignoreCase) std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
        if (opt_.ignoreWhitespace) s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c); }), s.end());
        return s;
    }

public:
    explicit Differ(Options o) : opt_(o) {}

    // Line-by-line comparison using Longest Common Subsequence.
    std::vector<DiffEntry> compare(const std::vector<std::string>& a, const std::vector<std::string>& b) const {
        // keep original line numbers when blank lines are ignored
        struct L { std::string raw, norm; int no; };
        std::vector<L> x, y;
        for (size_t i = 0; i < a.size(); ++i)
            if (!(opt_.ignoreBlank && normalize(a[i]).empty() && a[i].find_first_not_of(" \t") == std::string::npos))
                x.push_back({a[i], normalize(a[i]), (int)i + 1});
        for (size_t i = 0; i < b.size(); ++i)
            if (!(opt_.ignoreBlank && b[i].find_first_not_of(" \t") == std::string::npos))
                y.push_back({b[i], normalize(b[i]), (int)i + 1});

        size_t n = x.size(), m = y.size();
        std::vector<std::vector<int>> dp(n + 1, std::vector<int>(m + 1, 0));
        for (size_t i = n; i-- > 0;)
            for (size_t j = m; j-- > 0;)
                dp[i][j] = (x[i].norm == y[j].norm) ? dp[i + 1][j + 1] + 1 : std::max(dp[i + 1][j], dp[i][j + 1]);

        std::vector<DiffEntry> out;
        std::vector<L> pendRem, pendAdd;
        auto flush = [&]() {
            size_t k = 0;
            for (; k < pendRem.size() && k < pendAdd.size(); ++k)
                out.push_back({Kind::Modified, pendRem[k].no, pendAdd[k].no, pendRem[k].raw, pendAdd[k].raw});
            for (size_t r = k; r < pendRem.size(); ++r) out.push_back({Kind::Removed, pendRem[r].no, 0, pendRem[r].raw, ""});
            for (size_t r = k; r < pendAdd.size(); ++r) out.push_back({Kind::Added, 0, pendAdd[r].no, "", pendAdd[r].raw});
            pendRem.clear(); pendAdd.clear();
        };
        size_t i = 0, j = 0;
        while (i < n || j < m) {
            if (i < n && j < m && x[i].norm == y[j].norm) {
                flush();
                out.push_back({Kind::Match, x[i].no, y[j].no, x[i].raw, y[j].raw});
                ++i; ++j;
            } else if (j < m && (i == n || dp[i][j + 1] >= dp[i + 1][j])) {
                pendAdd.push_back(y[j++]);
            } else {
                pendRem.push_back(x[i++]);
            }
        }
        flush();
        return out;
    }

    static Summary summarize(const std::vector<DiffEntry>& d, size_t t1, size_t t2) {
        Summary s; s.total1 = t1; s.total2 = t2;
        for (auto& e : d) {
            switch (e.kind) {
                case Kind::Match: ++s.matching; break;
                case Kind::Modified: ++s.modified; break;
                case Kind::Added: ++s.added; break;
                case Kind::Removed: ++s.removed; break;
            }
        }
        return s;
    }

    // Character-level comparison inside a modified line (common prefix / suffix trimming).
    static std::string charDiff(const std::string& a, const std::string& b) {
        size_t p = 0;
        while (p < a.size() && p < b.size() && a[p] == b[p]) ++p;
        size_t sa = a.size(), sb = b.size();
        while (sa > p && sb > p && a[sa - 1] == b[sb - 1]) { --sa; --sb; }
        std::string rem = a.substr(p, sa - p), add = b.substr(p, sb - p);
        std::ostringstream o;
        if (rem.empty() && !add.empty()) o << "Additional text -> \"" << add << "\" (at column " << p + 1 << ")";
        else if (!rem.empty() && add.empty()) o << "Removed text -> \"" << rem << "\" (at column " << p + 1 << ")";
        else o << "Changed \"" << rem << "\" -> \"" << add << "\" (at column " << p + 1 << ")";
        return o.str();
    }
};

// ---------- Report generation ----------
class ReportWriter {
public:
    static void write(std::ostream& os, const std::string& f1, const std::string& f2,
                      const std::vector<DiffEntry>& d, const Summary& s, bool showMatches, bool color) {
        const char* R = color ? "\033[31m" : ""; const char* G = color ? "\033[32m" : "";
        const char* Y = color ? "\033[33m" : ""; const char* Z = color ? "\033[0m" : "";
        os << "===== COMPARISON REPORT =====\n"
           << "File 1: " << f1 << "\nFile 2: " << f2 << "\n\n";
        for (auto& e : d) {
            switch (e.kind) {
                case Kind::Match:
                    if (showMatches) os << "  = Line " << e.line1 << " / " << e.line2 << ": " << e.text1 << "\n";
                    break;
                case Kind::Modified:
                    os << Y << "  ~ Line " << e.line1 << " (File 1) vs Line " << e.line2 << " (File 2)" << Z << "\n"
                       << "      File 1 -> " << e.text1 << "\n      File 2 -> " << e.text2 << "\n"
                       << "      Diff   : " << Differ::charDiff(e.text1, e.text2) << "\n";
                    break;
                case Kind::Added:
                    os << G << "  + Added   (File 2, line " << e.line2 << "): " << e.text2 << Z << "\n"; break;
                case Kind::Removed:
                    os << R << "  - Removed (File 1, line " << e.line1 << "): " << e.text1 << Z << "\n"; break;
            }
        }
        os << "\n===== SUMMARY =====\n"
           << "Total Lines in File 1: " << s.total1 << "\n"
           << "Total Lines in File 2: " << s.total2 << "\n"
           << "Matching Lines: " << s.matching << "\n"
           << "Modified Lines: " << s.modified << "\n"
           << "Added Lines: " << s.added << "\n"
           << "Removed Lines: " << s.removed << "\n"
           << (s.identical() ? "Result: Files are identical.\n" : "Result: Files are different.\n");
    }
};

// ---------- Application ----------
class App {
    Options opt_;

    bool load(const std::string& p, std::vector<std::string>& v) {
        std::string err;
        if (!FileManager::exists(p)) { std::cout << "  [!] File does not exist: " << p << "\n"; return false; }
        if (!FileManager::readLines(p, v, err)) { std::cout << "  [!] " << err << "\n"; return false; }
        return true;
    }

public:
    bool compareTwo(const std::string& p1, const std::string& p2, std::ostream& os, bool showMatches, bool color,
                    Summary* outSum = nullptr) {
        std::vector<std::string> a, b;
        if (!load(p1, a) || !load(p2, b)) return false;
        Differ d(opt_);
        auto diff = d.compare(a, b);
        auto sum = Differ::summarize(diff, a.size(), b.size());
        ReportWriter::write(os, p1, p2, diff, sum, showMatches, color);
        if (outSum) *outSum = sum;
        return true;
    }

    void compareMultiple() {
        std::cout << "How many files? ";
        int n; if (!(std::cin >> n) || n < 2) { std::cin.clear(); std::cin.ignore(1000, '\n'); std::cout << "  [!] Need at least 2 files.\n"; return; }
        std::cin.ignore(1000, '\n');
        std::vector<std::string> paths(n);
        for (int i = 0; i < n; ++i) { std::cout << "Path of file " << i + 1 << ": "; std::getline(std::cin, paths[i]); }
        std::cout << "\n===== MULTI-FILE COMPARISON (against file 1) =====\n";
        for (int i = 1; i < n; ++i) {
            std::cout << "\n--- " << paths[0] << "  vs  " << paths[i] << " ---\n";
            Summary s;
            std::ostringstream sink;
            if (compareTwo(paths[0], paths[i], sink, false, false, &s))
                std::cout << "Modified: " << s.modified << "  Added: " << s.added << "  Removed: " << s.removed
                          << "  ->  " << (s.identical() ? "identical" : "different") << "\n";
        }
    }

    void duplicates(const std::vector<std::string>& paths) {
        std::map<std::string, std::vector<std::string>> byContent;
        for (auto& p : paths) {
            std::vector<std::string> l; std::string e; if (!FileManager::readLines(p, l, e)) continue;
            std::string key; for (auto& s : l) key += s + "\n";
            byContent[key].push_back(p);
        }
        for (auto& [k, v] : byContent) if (v.size() > 1) { std::cout << "Duplicates:"; for (auto& p : v) std::cout << " " << p; std::cout << "\n"; }
    }

    void toggleOptions() {
        auto yn = [](const char* q) { std::cout << q << " (y/n): "; std::string s; std::getline(std::cin, s); return !s.empty() && (s[0] == 'y' || s[0] == 'Y'); };
        opt_.ignoreWhitespace = yn("Ignore whitespace differences?");
        opt_.ignoreBlank = yn("Ignore blank lines?");
        opt_.ignoreCase = yn("Ignore case?");
    }

    void menu() {
        std::string c;
        while (true) {
            std::cout << "\n===== FILE COMPARISON TOOL =====\n"
                         "1. Compare Two Files\n2. View File Information\n3. Generate Comparison Report\n"
                         "4. Compare Multiple Files\n5. Comparison Options (ignore whitespace/blank/case)\n"
                         "6. Duplicate File Detection\n7. Exit\nEnter your choice: ";
            if (!std::getline(std::cin, c)) return;
            std::string p1, p2;
            if (c == "1" || c == "3") {
                std::cout << "Path of first file : "; std::getline(std::cin, p1);
                std::cout << "Path of second file: "; std::getline(std::cin, p2);
                if (c == "1") compareTwo(p1, p2, std::cout, true, true);
                else {
                    std::cout << "Report file name (.txt or .csv-style text): "; std::string out; std::getline(std::cin, out);
                    std::ofstream f(out);
                    if (!f) { std::cout << "  [!] Cannot write " << out << "\n"; continue; }
                    if (compareTwo(p1, p2, f, true, false)) std::cout << "Report saved to " << out << "\n";
                }
            } else if (c == "2") {
                std::cout << "Path: "; std::getline(std::cin, p1); FileManager::printInfo(p1);
            } else if (c == "4") compareMultiple();
            else if (c == "5") toggleOptions();
            else if (c == "6") {
                std::cout << "Enter paths separated by spaces: "; std::string line; std::getline(std::cin, line);
                std::istringstream is(line); std::vector<std::string> ps; std::string p; while (is >> p) ps.push_back(p);
                duplicates(ps);
            } else if (c == "7") { std::cout << "Goodbye!\n"; return; }
            else std::cout << "  [!] Invalid choice.\n";
        }
    }

    Options& options() { return opt_; }
};

int main(int argc, char** argv) {
    App app;
    // Non-interactive mode: filecompare <file1> <file2> [-w] [-b] [-i] [-o report.txt]
    if (argc >= 3) {
        std::string out;
        for (int i = 3; i < argc; ++i) {
            std::string a = argv[i];
            if (a == "-w") app.options().ignoreWhitespace = true;
            else if (a == "-b") app.options().ignoreBlank = true;
            else if (a == "-i") app.options().ignoreCase = true;
            else if (a == "-o" && i + 1 < argc) out = argv[++i];
        }
        Summary s;
        if (!out.empty()) { std::ofstream f(out); if (!f) { std::cerr << "Cannot write " << out << "\n"; return 2; } if (!app.compareTwo(argv[1], argv[2], f, true, false, &s)) return 2; std::cout << "Report saved to " << out << "\n"; }
        else if (!app.compareTwo(argv[1], argv[2], std::cout, true, false, &s)) return 2;
        return s.identical() ? 0 : 1;
    }
    app.menu();
    return 0;
}
