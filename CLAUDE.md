# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## About

Учебный проект по программированию на C (2-й семестр). Четыре лабораторные работы, каждая из которых строится на предыдущей.

## Build & Run

Each lab has its own Makefile. Run from inside the lab directory:

```bash
cd lab_N
make gcc    # compile (lab_3, lab_4)
make all    # compile (lab_1, lab_2)
make run    # build + run
make clean  # remove build artifacts
```

lab_3 runs with `leaks --atExit -- ./1.o` (macOS memory leak check).  
lab_4 Makefile has a bug: `run` target calls `/1.o` instead of `./1.o` — run manually with `./1.o`.

## Architecture

Labs build on each other cumulatively — later labs compile earlier labs' `.c` files directly (not as libraries):

```
lab_4/fiovector.c
  └── lab_3/contvector.c   (vector_t — dynamic array of datatime*)
        └── lab_2/datatime.c + bitstruct.c
```

### lab_1 & lab_2 — `datatime`
Struct representing date+time. **Important difference between labs**: in lab_1, fields are plain `int`; in lab_2, fields are `int*` (individually heap-allocated). lab_3 and lab_4 use lab_2's version.

### lab_2 — `device` / `bitstruct`
A `uint16_t` packed with 7 device config fields (display, brightness, time format, alarm, memory, cpu, water resistance) accessed via bitmask macros defined in `bitstruct.h`.

### lab_3 — `vector_t` (contvector)
Dynamic array of `datatime*` with capacity doubling. Has iterator API (`vec_begin`, `vec_end`, `vec_iter_next`, `vec_isequal`). The `res` field in the struct is unused/reserved.

### lab_4 — `fiovector` / `data_ft`
File I/O layer over `vector_t`. Introduces `data_ft` struct that combines `datatime` fields + `uint16_t dev_data` (device config) into a flat struct for binary serialization. Provides both text and binary save/load, with fast (fixed-width) and slow (line-scan) variants for text access.

## Key conventions

- All heap-allocated structs are created via `*_create()` and freed via `*_destroy()`, which takes a double pointer and NULLs it.
- `datatime_to_string()` returns a heap-allocated string — caller must `free()` it.
- Time arithmetic uses a simplified calendar: 1 year = 365 days, 1 month = 30 days.
- `TXT_LL` in fiovector.h is the fixed text line length (23 bytes on Unix, 24 on Windows) used for fast random-access reads.
