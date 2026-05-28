# SimpleDB

**AP Computer Science A Final Project**

A lightweight SQL database engine built from scratch in C++.

---

##  Features

| Category | Features |
|----------|----------|
| **Schema** | CREATE TABLE with INT, FLOAT, TEXT data types |
| **Constraints** | PRIMARY KEY, NOT NULL, UNIQUE |
| **CRUD** | INSERT, SELECT, UPDATE, DELETE |
| **Query** | WHERE clause with AND/OR logic |
| **Sorting** | ORDER BY (ASC/DESC) and LIMIT |
| **Aggregates** | COUNT, SUM, AVG, MIN, MAX |
| **Joins** | INNER JOIN and LEFT JOIN |
| **Persistence** | schema.txt + CSV files |
| **UX** | Color-coded output, multi-line queries |

---

##  Changelog

### Version 1.2 (May 25, 2026) ~7 hours
- Added color output
- Improved error messages  
- Multi-line query support

### Version 1.1 (May 23, 2026) ~19 hours
- Added JOIN (INNER and LEFT)
- Added aggregate functions
- Fixed CSV parsing for text with commas

### Version 1.0 (May 17, 2026) ~50 hours
- Initial release
- Core SQL operations implemented
- Basic WHERE clause support

**Total Development Time: ~44 hours**

---

##  Quick Start

```bash
g++ *.cpp -o simpledb
./simpledb
```

## Future Development

- B-Tree
- B+ Tree (potentially)
