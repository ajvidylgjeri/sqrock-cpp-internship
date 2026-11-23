# Mini Search Engine (C++)

Sqrock IT Solutions - C++ Programming Internship, Project Phase 2, Task 2.

## Project overview
A console search engine that loads local text documents (.txt, .csv, .cpp, .h, .md),
indexes them and returns ranked results for keyword queries.

## Features
- Document management: add a file or a whole folder, validation, duplicate / empty / unsupported file handling
- Case-insensitive search (`C++`, `c++`), multi-keyword queries, `C++` and `C#` aware tokenizer
- Relevance ranking with match counts and a relevance percentage
- Search history (view + save to `search_history.txt`), document statistics (top words, vocabulary)
- Bonus: inverted index, boolean `AND` / `OR`, phrase search (`"data structures"`), word suggestions ("Did you mean"), word frequency analysis
- Error handling: invalid path, empty documents, empty query, no documents, no results

## Technologies used
C++17, STL (`vector`, `unordered_map`, `map`, `set`), `<fstream>`, `<filesystem>`, OOP
(`DocumentManager`, `InvertedIndex`, `SearchEngine`, `SearchHistory`, `App`).

## Search methodology
Every document is tokenized (lower-cased words made of letters, digits, `+`, `#`) and a word-frequency map
plus an inverted index (`word -> document ids`) is built. A query is split into words and quoted phrases.
Default is OR (any term); `AND` requires every term. Phrases are matched on the token sequence.

## Ranking methodology
For each term: `score += (count / words_in_doc) * (1 + ln((N+1)/df)) * 1000` (TF-IDF style),
then multiplied by `1 + 0.5 * (distinct_terms_found - 1)`. Results are sorted by score; the best
document is 100% and the others are shown relative to it.

## How to run
```bash
make
./search_engine                              # interactive menu
./search_engine docs -q "C++ programming"    # load folder, run query (repeat -q for more)
```

## Sample input / output
See `docs/` and `output/sample_output.txt`.
```
Query: C++ programming
Rank 1 -> cpp_basics.txt   Keyword Matches: 6   Relevance Score: 100%
Rank 2 -> programming.txt  Keyword Matches: 3   Relevance Score: 45%
Rank 3 -> internship.txt   Keyword Matches: 1   Relevance Score: 19%
```
