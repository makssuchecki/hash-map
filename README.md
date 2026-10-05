# HashMap from scratch (C++17)

A header-only hash map (`string -> int`) built from scratch to learn how hash tables actually work: hashing, collision handling, load factor and rehashing. Collisions are resolved with **separate chaining**.

Not a replacement for `std::unordered_map`. It is a learning project with tests and a benchmark against the standard library.

## Design

- **Storage:** `std::vector<std::list<std::pair<std::string,int>>>`, one list per bucket.
- **Hashing:** `std::hash<std::string>`, bucket index is `hash % bucket_count`.
- **Load factor:** `size / bucket_count`. When it exceeds `max_load_factor` (default `0.75`), the table doubles and rehashes.
- **Rehash:** every key's index is recomputed for the new bucket count (the index depends on the table size). Nodes are moved between lists with `std::list::splice`, so rehashing does **no per-element allocation**.
- **Reference stability:** because nodes are never reallocated, references returned by `operator[]` stay valid across rehashes (same guarantee as `std::unordered_map`). Iterators are not provided.
- **Amortized O(1)** insert/find/erase: lookups scan a single bucket, and the O(n) rehash cost is spread over geometric growth.

## API

| Method | Description |
|---|---|
| `HashMap(size_t bucket_count = 16)` | Throws `std::invalid_argument` if `bucket_count == 0` |
| `void insert(key, value)` | Inserts, or overwrites the value if the key exists |
| `std::optional<int> find(key) const` | Value if present, `std::nullopt` otherwise |
| `bool erase(key)` | `true` if the key was removed |
| `int& operator[](key)` | Returns a reference, inserting `0` if the key is missing |
| `size()`, `bucket_count()`, `load_factor()` | Introspection |
| `void max_load_factor(double)` | Rehash threshold |

```cpp
#include "hash_map.h"

HashMap m;
m.insert("alice", 1);
m["bob"] += 5;

if (auto v = m.find("alice")) { /* *v == 1 */ }
m.erase("alice");
```

## Build and test

```bash
# tests, with sanitizers
g++ -std=c++17 -Wall -Wextra -Wpedantic -fsanitize=address,undefined -g test.cpp -o test && ./test

# benchmark
g++ -std=c++17 -O2 bench.cpp -o bench && ./bench
```

The tests cover insert/update/find/erase, forced collisions and rehashing (starting from 2 buckets and inserting 1000 keys), and `operator[]` reference stability across a rehash.

## Benchmark

1,000,000 string keys (`"key0"` ... `"key999999"`), `g++ -O2`, WSL, single run, successful lookups only.

| Container | insert (ms) | find (ms) | final buckets |
|---|---:|---:|---:|
| HashMap, max load 0.5 | 351 | 78 | 2,097,152 |
| HashMap, max load 0.75 | 505 | 77 | 2,097,152 |
| HashMap, max load 2.0 | 471 | 128 | 524,288 |
| `std::unordered_map` (max load 1.0) | 337 | 125 | 1,447,153 |

What the numbers show:

- Load factors 0.5 and 0.75 end with the same bucket count for N = 1M, so their `find` times are the same. The result depends on where N falls relative to the power-of-two growth steps.
- Max load 2.0 makes `find` about 65% slower than 0.5, because chains are longer and every list node is a separate cache miss.
- The comparison with `std::unordered_map` is not like-for-like: it ends at a different load factor and caches hashes in its nodes.

Not yet verified: the `insert` gap between 0.5 and 0.75 is probably extra rehash work (more elements moved before the final table size). Planned check: a pre-sized run (`HashMap m(1 << 21)`) and `reserve()` on the std map, plus multiple runs and lookups of missing keys.

## Limitations

- Fixed key/value types (`std::string`, `int`).
- No iterators, no shrinking, not thread-safe.
- Chaining with `std::list` has poor cache locality compared to open-addressing designs.
