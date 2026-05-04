# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

Each lab builds independently from its own directory:

```bash
# lab_1
cd lab_1 && make all    # produces datetime_app.o
cd lab_1 && make run    # runs ./datetime_app.o

# lab_2
cd lab_2 && make all    # produces datatime_app.o
cd lab_2 && make run    # runs ./datatime_app.o

# lab_3
cd lab_3 && make gcc    # produces 1.o
cd lab_3 && ./1.o       # make run calls 'leaks' (macOS-only), run directly on Linux

# lab_4 (CLI tool)
cd lab_4 && make gcc    # produces 1.o
# Note: 'make run' has a typo (/1.o), use directly:
./1.o save <txt|bin> <file> [count]   # generate and save N records (default 10000)
./1.o load <txt|bin> <file>           # load and print all records
./1.o list <txt|bin> <file>           # count records in file
./1.o get  <txt|bin> <index> <file>  # fetch one record by index
```

## Architecture

Labs build on each other. Source files from earlier labs are included via relative paths (`../lab_2/`, `../lab_3/`), so changes to shared files affect all later labs.

**Dependency chain:** lab_4 → lab_3 → lab_2

### `datatime` struct (`lab_2/datatime.h`)
Central data type used throughout all labs. Fields are plain `int` (not pointers). Contains an embedded `device *dev` pointer (heap-allocated bitstruct). The `lab_1` version predates the refactor — its `datatime.h` uses `int*` pointer fields.

### `device` bitstruct (`lab_2/bitstruct.h`)
Packs 7 hardware device attributes into a single `uint16_t` using bitmask macros (`DIS_MASK`, `BRI_MASK`, etc.). Access only via `dev_get_*` / `dev_set_*` functions. Embedded in every `datatime` object starting from lab_2.

### `vector_t` dynamic array (`lab_3/contvector.h`)
Holds `datatime **data` with automatic capacity doubling. Has a secondary `datatime **res` field used as a scratch/swap buffer. Comes with an iterator (`vec_iter_t`) and the full CRUD API: push, pop, insert, remove, change, merge, copy.

### File I/O (`lab_4/fiovector.h`)
Extends the vector with persistence. Uses a flat `data_ft` struct (plain ints + `uint16_t dev_data`) for serialization — distinct from the heap-based `datatime`. Supports two formats:
- **Text**: fixed-width lines (`TXT_LL = 23` bytes each), enables O(1) indexed access via `get_elm_txt_fast`.
- **Binary**: raw `data_ft` structs, also O(1) indexed access via `get_elm_bin`.
