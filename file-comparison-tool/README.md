# File Comparison Tool (C++)

Sqrock IT Solutions - C++ Programming Internship, Project Phase 2, Task 1.

## Project overview
A console application that compares two (or more) text-based files line by line,
detects matching, modified, added and removed lines, shows character-level
differences inside modified lines, and prints a summary that says whether the
files are identical.

## Features
- File path input with existence validation and file information (size, lines, modified date, extension)
- Line-by-line comparison with line numbers (matching / modified / added / removed)
- Character-level difference inside modified lines (e.g. `Additional text -> " IT Solutions"`)
- Summary: totals, matching, modified, added, removed, identical or different
- Bonus: compare more than two files, ignore whitespace / blank lines / case,
  save report to `.txt`, colored output, duplicate file detection, file statistics

## Technologies used
C++17, STL (`vector`, `map`, `string`), `<fstream>`, `<filesystem>`, OOP
(`FileManager`, `Differ`, `ReportWriter`, `App`).

## How the comparison works
1. Both files are read into `vector<string>` (normalised if ignore-options are on).
2. A Longest Common Subsequence table (dynamic programming) finds the lines that match.
3. Lines outside the LCS are collected as removed (file 1) / added (file 2); a removed
   and an added block at the same position are paired up as *modified* lines.
4. For modified lines the common prefix and suffix are trimmed to show exactly which
   characters were added, removed or changed.

## How to run
```bash
make                      # or: g++ -std=c++17 -O2 -o filecompare src/main.cpp
./filecompare             # interactive menu
./filecompare a.txt b.txt # direct comparison (-w ignore whitespace, -b ignore blank lines, -i ignore case, -o report.txt)
```
Exit code: 0 identical, 1 different, 2 error.

## Sample input / output
See `samples/` and `output/sample_output.txt`.

```
  ~ Line 2 (File 1) vs Line 2 (File 2)
      File 1 -> Welcome to Sqrock
      File 2 -> Welcome to Sqrock IT Solutions
      Diff   : Additional text -> " IT Solutions" (at column 18)
...
Matching Lines: 2   Modified Lines: 2   Added Lines: 1   Removed Lines: 0
Result: Files are different.
```
