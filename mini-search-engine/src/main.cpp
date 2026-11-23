// Mini Search Engine - Sqrock IT Solutions, C++ Internship Phase 2 (Task 2)
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

// ---------- Text processing ----------
class TextProcessor {
public:
    static std::string lower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
        return s;
    }
    // Words may contain letters, digits, '+' and '#' so that "C++" and "C#" are searchable.
    static std::vector<std::string> tokenize(const std::string& text) {
        std::vector<std::string> out; std::string cur;
        for (unsigned char c : text) {
            if (std::isalnum(c) || c == '+' || c == '#') cur += (char)std::tolower(c);
            else if (!cur.empty()) { out.push_back(cur); cur.clear(); }
        }
        if (!cur.empty()) out.push_back(cur);
        return out;
    }
};

// ---------- Documents ----------
struct Document {
    int id;
    std::string name, path, ext;
    std::vector<std::string> words;      // tokens in order (for phrase search)
    std::unordered_map<std::string, int> freq;
    size_t bytes = 0;
};

class DocumentManager {
    std::vector<Document> docs_;
    std::set<std::string> paths_;
    std::set<std::string> allowed_{".txt", ".csv", ".cpp", ".h", ".md"};

public:
    // returns empty string on success, otherwise an error message
    std::string add(const std::string& p) {
        if (!fs::exists(p)) return "File not found: " + p;
        if (!fs::is_regular_file(p)) return "Not a regular file: " + p;
        std::string ext = fs::path(p).extension().string();
        if (!allowed_.count(ext)) return "Unsupported file type (" + ext + "): " + p;
        std::string canon = fs::weakly_canonical(p).string();
        if (paths_.count(canon)) return "Already loaded: " + p;
        std::ifstream in(p);
        if (!in) return "Cannot open file: " + p;
        std::stringstream ss; ss << in.rdbuf();
        std::string text = ss.str();
        if (text.find_first_not_of(" \t\r\n") == std::string::npos) return "Document is empty: " + p;
        Document d; d.id = (int)docs_.size() + 1; d.name = fs::path(p).filename().string(); d.path = canon;
        d.ext = ext; d.bytes = text.size(); d.words = TextProcessor::tokenize(text);
        for (auto& w : d.words) ++d.freq[w];
        paths_.insert(canon);
        docs_.push_back(std::move(d));
        return "";
    }
    int addDirectory(const std::string& dir, std::vector<std::string>& errors) {
        int n = 0;
        if (!fs::is_directory(dir)) { errors.push_back("Not a directory: " + dir); return 0; }
        std::vector<fs::path> files;
        for (auto& e : fs::directory_iterator(dir)) if (e.is_regular_file()) files.push_back(e.path());
        std::sort(files.begin(), files.end());
        for (auto& f : files) { auto e = add(f.string()); if (e.empty()) ++n; else errors.push_back(e); }
        return n;
    }
    const std::vector<Document>& docs() const { return docs_; }
};

// ---------- Inverted index ----------
class InvertedIndex {
    std::unordered_map<std::string, std::set<int>> idx_;   // word -> doc ids
public:
    void build(const std::vector<Document>& docs) {
        idx_.clear();
        for (auto& d : docs) for (auto& [w, c] : d.freq) idx_[w].insert(d.id);
    }
    size_t docFreq(const std::string& w) const { auto it = idx_.find(w); return it == idx_.end() ? 0 : it->second.size(); }
    // simple "did you mean" suggestions: indexed words sharing the same prefix
    std::vector<std::string> suggest(const std::string& prefix, size_t limit = 5) const {
        std::vector<std::string> r;
        for (auto& [w, s] : idx_) if (w.size() > prefix.size() && w.compare(0, prefix.size(), prefix) == 0) r.push_back(w);
        std::sort(r.begin(), r.end()); if (r.size() > limit) r.resize(limit);
        return r;
    }
    size_t vocabulary() const { return idx_.size(); }
};

// ---------- Searching & ranking ----------
struct Result { const Document* doc; int matches; double score; double relevance; };

class SearchEngine {
    const DocumentManager& dm_;
    InvertedIndex index_;

    static bool phraseIn(const Document& d, const std::vector<std::string>& ph, int& count) {
        count = 0;
        if (ph.empty() || d.words.size() < ph.size()) return false;
        for (size_t i = 0; i + ph.size() <= d.words.size(); ++i)
            if (std::equal(ph.begin(), ph.end(), d.words.begin() + i)) ++count;
        return count > 0;
    }

public:
    explicit SearchEngine(const DocumentManager& dm) : dm_(dm) {}
    void reindex() { index_.build(dm_.docs()); }
    const InvertedIndex& index() const { return index_; }

    // Query syntax: words (OR ranking by default), AND between words with "AND", "quoted phrase".
    std::vector<Result> search(const std::string& query) const {
        bool requireAll = false;
        std::vector<std::vector<std::string>> units;   // each unit = one word or one phrase
        std::string q = query;
        size_t pos = 0;
        while (pos < q.size()) {
            if (q[pos] == '"') {
                size_t end = q.find('"', pos + 1);
                if (end == std::string::npos) end = q.size();
                auto t = TextProcessor::tokenize(q.substr(pos + 1, end - pos - 1));
                if (!t.empty()) units.push_back(t);
                pos = end + 1;
            } else if (std::isspace((unsigned char)q[pos])) ++pos;
            else {
                size_t end = pos; while (end < q.size() && !std::isspace((unsigned char)q[end])) ++end;
                std::string w = q.substr(pos, end - pos);
                if (w == "AND") requireAll = true;
                else if (w != "OR") { auto t = TextProcessor::tokenize(w); for (auto& x : t) units.push_back({x}); }
                pos = end;
            }
        }
        std::vector<Result> res;
        const auto& docs = dm_.docs();
        double N = (double)docs.size();
        for (auto& d : docs) {
            int totalMatches = 0, unitsHit = 0; double score = 0;
            for (auto& u : units) {
                int c = 0;
                if (u.size() == 1) { auto it = d.freq.find(u[0]); c = it == d.freq.end() ? 0 : it->second; }
                else phraseIn(d, u, c);
                if (c > 0) {
                    ++unitsHit; totalMatches += c;
                    double df = std::max<size_t>(1, index_.docFreq(u[0]));
                    double tf = (double)c / (double)std::max<size_t>(1, d.words.size());
                    score += tf * (1.0 + std::log((N + 1.0) / df)) * 1000.0;   // tf-idf style
                }
            }
            if (unitsHit == 0) continue;
            if (requireAll && unitsHit != (int)units.size()) continue;
            score *= 1.0 + 0.5 * (unitsHit - 1);     // reward documents containing more distinct terms
            res.push_back({&d, totalMatches, score, 0});
        }
        std::sort(res.begin(), res.end(), [](const Result& a, const Result& b) {
            return a.score != b.score ? a.score > b.score : a.doc->name < b.doc->name; });
        if (!res.empty()) for (auto& r : res) r.relevance = 100.0 * r.score / res.front().score;
        return res;
    }
};

// ---------- History ----------
class SearchHistory {
    std::vector<std::string> h_;
public:
    void add(const std::string& q) { h_.push_back(q); }
    void print() const {
        std::cout << "\n===== SEARCH HISTORY =====\n";
        if (h_.empty()) { std::cout << "(no searches yet)\n"; return; }
        for (size_t i = 0; i < h_.size(); ++i) std::cout << i + 1 << ". " << h_[i] << "\n";
    }
    bool save(const std::string& f) const { std::ofstream o(f); if (!o) return false; for (auto& s : h_) o << s << "\n"; return true; }
};

// ---------- Application ----------
class App {
    DocumentManager dm_;
    SearchEngine eng_{dm_};
    SearchHistory hist_;

    static std::string trim(std::string s) {
        size_t a = s.find_first_not_of(" \t\r\n"); if (a == std::string::npos) return "";
        return s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
    }

public:
    void load(const std::string& path) {
        std::vector<std::string> errs; int n = 0;
        if (fs::is_directory(path)) n = dm_.addDirectory(path, errs);
        else { auto e = dm_.add(path); if (e.empty()) n = 1; else errs.push_back(e); }
        eng_.reindex();
        std::cout << "Loaded " << n << " document(s).\n";
        for (auto& e : errs) std::cout << "  [!] " << e << "\n";
    }

    void viewDocs() const {
        std::cout << "\n===== DOCUMENTS =====\n";
        if (dm_.docs().empty()) { std::cout << "(none loaded)\n"; return; }
        for (auto& d : dm_.docs()) std::cout << d.id << ". " << d.name << "  (" << d.bytes << " bytes, " << d.words.size() << " words)  " << d.path << "\n";
    }

    void stats() const {
        std::cout << "\n===== DOCUMENT STATISTICS =====\n";
        if (dm_.docs().empty()) { std::cout << "(none loaded)\n"; return; }
        size_t total = 0;
        for (auto& d : dm_.docs()) {
            total += d.words.size();
            std::vector<std::pair<std::string, int>> top(d.freq.begin(), d.freq.end());
            std::sort(top.begin(), top.end(), [](auto& a, auto& b) { return a.second != b.second ? a.second > b.second : a.first < b.first; });
            std::cout << d.name << ": " << d.words.size() << " words, " << d.freq.size() << " unique. Top words:";
            for (size_t i = 0; i < top.size() && i < 5; ++i) std::cout << " " << top[i].first << "(" << top[i].second << ")";
            std::cout << "\n";
        }
        std::cout << "Total documents: " << dm_.docs().size() << ", total words: " << total
                  << ", vocabulary: " << eng_.index().vocabulary() << "\n";
    }

    void search(const std::string& rawQuery) {
        std::string query = trim(rawQuery);
        if (query.empty()) { std::cout << "  [!] Search query cannot be empty.\n"; return; }
        if (dm_.docs().empty()) { std::cout << "  [!] No documents loaded. Add documents first.\n"; return; }
        hist_.add(query);
        auto res = eng_.search(query);
        std::cout << "\n===== SEARCH RESULTS =====\nQuery: " << query << "\n";
        if (res.empty()) {
            std::cout << "No results found for: " << query << "\n";
            auto toks = TextProcessor::tokenize(query);
            if (!toks.empty()) { auto s = eng_.index().suggest(toks[0]); if (!s.empty()) { std::cout << "Did you mean:"; for (auto& w : s) std::cout << " " << w; std::cout << "\n"; } }
            return;
        }
        int rank = 1;
        for (auto& r : res) {
            std::cout << "Rank " << rank++ << " -> " << r.doc->name << "\n"
                      << "   Keyword Matches: " << r.matches << "\n"
                      << "   Relevance Score: " << std::fixed << std::setprecision(0) << r.relevance << "%\n";
        }
    }

    void menu() {
        std::string c;
        while (true) {
            std::cout << "\n===== MINI SEARCH ENGINE =====\n1. Add Documents\n2. View Documents\n3. Search\n"
                         "4. View Search History\n5. View Document Statistics\n6. Save Search History\n7. Exit\nEnter your choice: ";
            if (!std::getline(std::cin, c)) return;
            if (c == "1") { std::cout << "File or folder path: "; std::string p; std::getline(std::cin, p); load(trim(p)); }
            else if (c == "2") viewDocs();
            else if (c == "3") { std::cout << "Enter search query: "; std::string q; std::getline(std::cin, q); search(q); }
            else if (c == "4") hist_.print();
            else if (c == "5") stats();
            else if (c == "6") std::cout << (hist_.save("search_history.txt") ? "Saved to search_history.txt\n" : "  [!] Could not write file.\n");
            else if (c == "7") { std::cout << "Goodbye!\n"; return; }
            else std::cout << "  [!] Invalid choice.\n";
        }
    }
};

int main(int argc, char** argv) {
    App app;
    // Non-interactive: search_engine <docs-folder-or-file> [more paths...] -q "query"
    std::vector<std::string> queries;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-q" && i + 1 < argc) queries.push_back(argv[++i]);
        else app.load(a);
    }
    if (!queries.empty()) { for (auto& q : queries) app.search(q); return 0; }
    app.menu();
    return 0;
}
