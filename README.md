# miniSQL – A Lightweight SQL-like Database in C

## 📌 Overview
**miniSQL** is a minimal SQL-like database engine written entirely in C.  
It implements the fundamental components of a database system — **REPL**, **Pager**, **B-Tree storage**, and a **Virtual Machine (VM)** — to execute a subset of SQL commands.

The project is designed for learning **database internals** and covers:
- **Storage Engines** – how data is stored and retrieved
- **B-Tree Indexing** – how databases organize and search data efficiently
- **Paging Mechanisms** – how data is split into fixed-size pages for disk I/O
- **Query Execution Pipelines** – how SQL statements are processed internally

---

## 🚀 Features
- Interactive **REPL** for SQL command execution
- **File-backed storage** (`test.db`) to persist data
- **B-Tree implementation** for table storage and indexing
- **Paging system** to manage on-disk pages efficiently
- **Virtual Machine** to execute parsed SQL commands
- **Unit tests** for B-Tree, Pager, and VM components

---

## 📂 Project Structure
```

├── bin/                 # Compiled executables
│   ├── miniSQL           # Main database binary
│   ├── test\_btree        # B-tree unit tests
│   └── test\_vm           # VM unit tests
├── build/                # Object files
│   ├── btree.o
│   ├── main.o
│   ├── pager.o
│   ├── repl.o
│   ├── test\_btree.o
│   ├── test\_vm.o
│   └── vm.o
├── docs/                 # Documentation
│   ├── Btree.md
│   └── Makefile
├── include/              # Header files
│   ├── btree.h
│   ├── common.h
│   ├── Makefile
│   ├── pager.h
│   ├── repl.h
│   └── vm.h
├── src/                  # Source code
│   ├── btree/            # B-tree implementation
│   │   ├── btree.c
│   │   └── Makefile
│   ├── pager/            # Pager implementation
│   │   ├── pager.c
│   │   └── Makefile
│   ├── repl/             # Read–Eval–Print Loop
│   │   ├── repl.c
│   │   └── Makefile
│   ├── vm/               # Virtual Machine
│   │   ├── vm.c
│   │   └── Makefile
│   └── main.c            # Entry point
├── tests/                # Unit tests
│   ├── Makefile
│   ├── test\_btree.c
│   ├── test\_pager.c
│   └── test\_vm.c
├── Makefile              # Root build file
├── run\_vm\_test.sh        # Helper script to run VM tests
└── test.db               # Sample database file

````

---

## ⚙️ Build Instructions
### Build the main executable
```bash
make
````

### Run the database

```bash
./bin/miniSQL test.db
```

---

## 💻 Usage Example

```
miniSQL> CREATE TABLE users (id INT, name TEXT);
Table created.

miniSQL> INSERT INTO users VALUES (1, "Alice");
Row inserted.

miniSQL> SELECT * FROM users;
id   name
1    Alice
```

---

## 🧪 Running Tests

Run all unit tests:

```bash
cd tests
make
./../bin/test_btree
./../bin/test_vm
```

Or use the provided script:

```bash
./run_vm_test.sh
```

---

## 🏗 Internal Architecture

```plaintext
           ┌─────────────┐
           │    REPL     │
           │(Read-Eval-  │
           │ Print Loop) │
           └──────┬──────┘
                  │
                  ▼
           ┌─────────────┐
           │    Parser   │
           │ (SQL to VM  │
           │  Instructions)
           └──────┬──────┘
                  │
                  ▼
           ┌─────────────┐
           │ Virtual     │
           │ Machine (VM)│
           └──────┬──────┘
                  │
                  ▼
           ┌─────────────┐
           │   Pager     │
           │ (Page Cache)│
           └──────┬──────┘
                  │
                  ▼
           ┌─────────────┐
           │   B-Tree    │
           │ (Data Store)│
           └─────────────┘
```

**Flow Explanation**:

1. **REPL** takes SQL input from the user.
2. **Parser** converts SQL into a series of bytecode-like instructions.
3. **VM** executes these instructions.
4. **Pager** manages reading/writing pages between memory and disk.
5. **B-Tree** stores and retrieves records efficiently.

---

## 📚 Learning Goals

By working with miniSQL, you will:

* Understand **how a database stores and retrieves data**
* Implement **B-Tree indexing** from scratch
* Explore **page-based disk I/O**
* Build a **minimal SQL virtual machine**
* Write **testable, modular C code**

---

## 📝 Future Improvements

* Add `UPDATE` and `DELETE` support
* Implement **query optimizer**
* Add **transaction support**
* Support **multi-table joins**
* Improve **error handling**

---

## 📄 License

This project is for educational purposes.
You may modify and distribute it freely with attribution.
