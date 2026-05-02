# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

Each lab has its own `Makefile`. From inside a lab directory:

```sh
make        # compile
make run    # compile and run
make clean  # remove build artifacts
```

Lab 4 accepts command-line arguments:
```sh
./1.o save txt output.txt 5   # save 5 records to text file
./1.o load bin input.bin      # load from binary file
./1.o list txt output.txt     # list all records
./1.o get txt output.txt 2    # get record at index 2
```

Memory leak check (macOS):
```sh
leaks --atExit -- ./1.o
```

## Architecture

Labs build on each other in a strict dependency chain:

```
lab_1  →  lab_2  →  lab_3  →  lab_4
```

- **lab_1**: Baseline `datatime` struct — all fields (`day`, `month`, `year`, `hour`, `minute`) are `int*` (dynamically allocated). Every operation goes through accessor functions.
- **lab_2**: Extends lab_1's `datatime.c` in-place. Adds `bitstruct` — a 16-bit packed config for a simulated CASIO device (display, brightness, time format, alarm, memory, CPU, water-resistance flags packed via bitfields).
- **lab_3**: `contvector` — a dynamic array (`void**` elements + `size_t count/capacity`) that stores `datatime*` pointers. Supports iterator pattern, copy, and merge. Compiles with `../lab_2/datatime.c` and `../lab_2/bitstruct.c`.
- **lab_4**: `fiovector` — file I/O layer on top of lab_3's vector. Text format uses `datatime_to_string`/`datatime_from_string`; binary format uses raw `fwrite`/`fread` of struct data. Compiles with sources from both lab_2 and lab_3.

## Key Conventions

- `datatime` is always heap-allocated; callers must call `datatime_destroy()` to free.
- `datatime_to_string()` returns a heap-allocated `char*` — caller must `free()` it.
- `contvector` owns its elements: `contvector_destroy()` frees both the array and each `datatime*` inside it.
- Simplified calendar model used throughout: 1 year = 365 days, 1 month = 30 days (not real calendar arithmetic).
- Binary file format: first `int` = element count, then sequential raw structs. `load_vec_bin` must allocate the vector with the count read from the file.
