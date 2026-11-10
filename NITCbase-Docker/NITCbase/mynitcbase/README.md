# NITCBase – Relational Database Management System

NITCBase is a simple Relational Database Management System (RDBMS) developed in **C++** as part of the DBMS Lab project. It demonstrates the basic working and architecture of a database system, from disk storage up to the user-facing command interface.

---

## Features

- **SQL Query Processing** – Supports database operations through user commands.
- **Relational Algebra** – Implements Select, Project, and Join operations.
- **B+ Tree Indexing** – Uses B+ Trees for efficient data searching.
- **Buffer Management** – Manages data pages between memory and disk.
- **Disk Storage** – Handles database storage and block-level access.
- **Schema Management** – Maintains table structures and metadata.

---

## Project Structure

```text
NITCbase/
├── Disk/
├── Files/
├── mynitcbase/
│   ├── Algebra/
│   ├── BPlusTree/
│   ├── BlockAccess/
│   ├── Buffer/
│   ├── Cache/
│   ├── Disk_Class/
│   ├── Frontend/
│   ├── FrontendInterface/
│   ├── Schema/
│   └── main.cpp
├── XFS_Interface/
└── Dockerfile
```

| Directory | Purpose |
|-----------|---------|
| `Disk/` | Simulated disk used for block-level storage |
| `Files/` | Supporting files for the project |
| `mynitcbase/` | Main source code of the RDBMS |
| `XFS_Interface/` | Interface for managing the disk and files |
| `Dockerfile` | Container setup for a consistent build environment |

### Source Modules (`mynitcbase/`)

| Module | Description |
|--------|-------------|
| `Algebra/` | Select, Project, and Join operations |
| `BPlusTree/` | B+ Tree indexing implementation |
| `BlockAccess/` | Block-level record access and search |
| `Buffer/` | Buffer management between memory and disk |
| `Cache/` | Caching of schema and metadata |
| `Disk_Class/` | Disk read/write abstraction |
| `Frontend/` | Command parsing and handling |
| `FrontendInterface/` | Interface between the frontend and lower layers |
| `Schema/` | Table creation, deletion, and metadata management |
| `main.cpp` | Program entry point |

---

## Prerequisites

- GCC / G++ compiler
- Make
- Linux-based environment

---

## Build Instructions

1. Navigate to the project directory:

   ```bash
   cd mynitcbase
   ```

2. Compile the project:

   ```bash
   make
   ```

---

## Running the Project

Run the database executable:

```bash
./nitcbase
```

---

## Clean Up

Remove compiled files:

```bash
make clean
```

---

## Technologies Used

- C++
- Data Structures and Algorithms
- Database Management Systems
- B+ Trees
- Linux and Make

---

## Contributing

Contributions, issues, and suggestions are welcome. Feel free to fork the repository and open a pull request.

## License

This project was developed for academic purposes as part of the DBMS Lab course.
