# C Hashtable

A simple hashtable implementation in C. This hashtable handles collisions using linear probing and automatically resizes when needed.

## Building

```bash
mkdir build
cd build
cmake ..
make
```

## Running Tests

```bash
./build/hashtable_test
```

## Basic Usage

```c
// Create a hashtable
hashtable *ht = ht_create(16, NULL, equals_function, free_key, free_value);

// Insert
ht_insert(ht, key, value);

// Get
value = ht_get(ht, key);

// Remove
ht_remove(ht, key);

// Clean up
ht_destroy(ht);
```

## Features

- Key-value storage with O(1) average lookup time
- Automatic resizing (grows and shrinks)
- Handles deleted entries correctly
- Iterator for traversing all entries
- Customizable hash and equality functions

## Notes

- Minimum capacity is enforced (won't shrink below 8 entries)
- You must provide an equals function for key comparison
- Free functions are optional but recommended for dynamic memory
