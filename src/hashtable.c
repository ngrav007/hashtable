#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "hashtable.h"

// Forward declarations
static size_t _default_hash(void *key);
static int _ht_resize(hashtable *ht, size_t new_capacity);

#define LOAD_FACTOR 0.75
#define RESIZE_FACTOR 2
#define DEFAULT_CAPACITY 16
#define HT_MIN_CAPACITY 8

// Global debug flag
bool ht_debug_enabled = false;

#define DEBUG_PRINT(fmt, ...) \
    do { if (ht_debug_enabled) fprintf(stderr, fmt, ##__VA_ARGS__); } while (0)

/**
 * @brief Create a new hashtable. If no hash function is provided, a default hash function for strings is used (djb2).
 *  If passed a capacity of 0, the default capacity is used. An equals function must be provided to compare keys, a
 *  hash table will not be created without one. Functions to free keys and free values should be provided if the keys
 *  and values are dynamically allocated, otherwise they can be NULL.
 * 
 * @param[in] capacity The initial capacity of the hashtable (0 for default)
 * @param[in] hash The hash function to use for keys (NULL for default)
 * @param[in] equals The function to use to compare keys (required)
 * @param[in] key_free The function to use to free keys (NULL if not needed)
 * @param[in] val_free The function to use to free values (NULL if not needed)
 * @return hashtable* The newly created hashtable
 */
hashtable *ht_create(size_t capacity, size_t (*hash)(void *), bool (*equals)(void *, void *), void (*key_free)(void *), void (*val_free)(void *)) {
    if (equals == NULL) {
        fprintf(stderr, "[-] Equals function is required\n");
        return NULL;
    }

    if (capacity == 0) {
        capacity = DEFAULT_CAPACITY;
    }

    if (capacity < HT_MIN_CAPACITY) {
        capacity = HT_MIN_CAPACITY;
    }

    if (hash == NULL) {
        hash = _default_hash;
    }

    hashtable *ht = calloc(1, sizeof(hashtable));
    if (ht == NULL) {
        fprintf(stderr, "[-] Failed to allocate hashtable\n");
        return NULL;
    }

    ht->entries = calloc(capacity, sizeof(ht_entry));
    if (ht->entries == NULL) {
        free(ht);
        fprintf(stderr, "[-] Failed to allocate entries array\n");
        return NULL;
    }

    ht->size = 0;
    ht->capacity = capacity;
    ht->hash = hash;
    ht->equals = equals;
    ht->key_free = key_free;
    ht->val_free = val_free;

    return ht;
}

/**
 * @brief Destroy a hashtable and free all memory associated with it
 * 
 * @param[in] ht The hashtable to destroy
 * @return void
 */
void ht_destroy(hashtable *ht) {
    if (ht != NULL) {
        for (size_t i = 0; i < ht->capacity; i++) {
            if (ht->entries[i].key != NULL) {
                if (ht->key_free != NULL) {
                    ht->key_free(ht->entries[i].key);
                }
                if (ht->val_free != NULL) {
                    ht->val_free(ht->entries[i].value);
                }
            }
        }

        free(ht->entries);
        free(ht);
    }
}

/**
 * @brief Insert a key-value pair into the hashtable. If the key already exists, the value is updated and the old value
 *  is freed if val_free is provided, but the key is not freed (the same key object is reused). If the load
 *  factor exceeds LOAD_FACTOR, the hashtable is resized to RESIZE_FACTOR * capacity.
 * 
 * @param[in] ht The hashtable to insert into
 * @param[in] key The key to insert
 * @param[in] value The value to insert
 * @return int 0 on success, -1 on failure
 */
int ht_insert(hashtable *ht, void *key, void *value) {
    if (ht == NULL || key == NULL) {
        DEBUG_PRINT("[-] Hashtable or key is NULL\n");
        return -1;
    }

    if (ht->size >= ht->capacity * LOAD_FACTOR) {
        if (_ht_resize(ht, ht->capacity * RESIZE_FACTOR) != 0) {
            DEBUG_PRINT("[-] Failed to resize hashtable\n");
            return -1;
        }
    }

    size_t index = ht->hash(key) % ht->capacity;
    size_t first_deleted = -1; // Track first deleted slot
    
    while (ht->entries[index].key != NULL || ht->entries[index].is_deleted) {
        // If we found a deleted slot and haven't saved one yet, remember it
        if (ht->entries[index].is_deleted && first_deleted == (size_t)-1) {
            first_deleted = index;
        }
        
        // If we found the key, update its value
        if (ht->entries[index].key != NULL && ht->equals(ht->entries[index].key, key)) {
            if (ht->val_free != NULL) {
                ht->val_free(ht->entries[index].value);
            }
            ht->entries[index].value = value;
            ht->entries[index].is_deleted = false; // Ensure it's not marked as deleted
            return 0;
        }
        
        index = (index + 1) % ht->capacity;
    }
    
    // If we found a deleted slot earlier, use that instead of an empty slot
    if (first_deleted != (size_t)-1) {
        index = first_deleted;
    }

    ht->entries[index].key = key;
    ht->entries[index].value = value;
    ht->entries[index].is_deleted = false;
    
    // Only increment size if we're not reusing a deleted slot
    if (first_deleted == (size_t)-1) {
        ht->size++;
    }
    return 0;
}

/**
 * @brief Remove a key-value pair from the hashtable, if it exists. If the key is not found, nothing is done. If the
 *  load factor falls below 1 - LOAD_FACTOR, the hashtable is resized to RESIZE_FACTOR * capacity. If the key is found,
 *  the key and value are freed if the key_free and val_free functions are provided.
 * 
 * @param[in] ht The hashtable to remove from
 * @param[in] key The key to remove
 * @return int 0 on success, -1 on failure
 */
int ht_remove(hashtable *ht, void *key) {
    if (ht == NULL || key == NULL) {
        DEBUG_PRINT("[-] Hashtable or key is NULL\n");
        return -1;
    }

    size_t index = ht->hash(key) % ht->capacity;
    while (ht->entries[index].key != NULL) {
        if (ht->equals(ht->entries[index].key, key)) {
            if (ht->key_free != NULL) {
                ht->key_free(ht->entries[index].key);
            }
            if (ht->val_free != NULL) {
                ht->val_free(ht->entries[index].value);
            }
            ht->entries[index].key = NULL;
            ht->entries[index].value = NULL;
            ht->entries[index].is_deleted = true;
            ht->size--;

            if (ht->size < ht->capacity * (1 - LOAD_FACTOR)) {
                if (_ht_resize(ht, ht->capacity / RESIZE_FACTOR) != 0) {
                    DEBUG_PRINT("[-] Failed to resize hashtable\n");
                    return -1;
                }
            }

            return 0;
        }
        index = (index + 1) % ht->capacity;
    }

    DEBUG_PRINT("[!] Key not found\n");
    return -1;  // Key not found
}

/**
 * @brief Get the value associated with a key in the hashtable. If the key is not found, NULL is returned.
 * 
 * @param[in] ht The hashtable to search
 * @param[in] key The key to search for
 * @return void* The value associated with the key, or NULL if the key is not found
 */
void *ht_get(hashtable *ht, void *key) {
    if (ht == NULL || key == NULL) {
        DEBUG_PRINT("[-] Hashtable or key is NULL\n");
        return NULL;
    }

    size_t index = ht->hash(key) % ht->capacity;
    size_t start_index = index;
    
    do {
        if (ht->entries[index].key == NULL && !ht->entries[index].is_deleted) {
            DEBUG_PRINT("[!] Key not found\n");
            return NULL;
        }
        if (ht->entries[index].key != NULL && ht->equals(ht->entries[index].key, key)) {
            DEBUG_PRINT("[+] Found key at index %zu\n", index);
            return ht->entries[index].value;
        }
        index = (index + 1) % ht->capacity;
    } while (index != start_index);

    DEBUG_PRINT("[!] Key not found (table full)\n");
    return NULL;
}

/**
 * @brief Resize the hashtable to a new capacity. All entries are rehashed and placed in the new hashtable.
 * 
 * @param[in] ht The hashtable to resize
 * @param[in] new_capacity The new capacity of the hashtable
 * @return int 0 on success, -1 on failure
 */
static int _ht_resize(hashtable *ht, size_t new_capacity) {
    if (ht == NULL) {
        DEBUG_PRINT("[-] Hashtable is NULL\n");
        return -1;
    }

    // Enforce minimum capacity
    if (new_capacity < HT_MIN_CAPACITY) {
        new_capacity = HT_MIN_CAPACITY;
    }

    DEBUG_PRINT("[+] Resizing from %zu to %zu\n", ht->capacity, new_capacity);

    // Allocate new entries array
    ht_entry *new_entries = calloc(new_capacity, sizeof(ht_entry));
    if (new_entries == NULL) {
        DEBUG_PRINT("[-] Failed to allocate new entries array\n");
        return -1;
    }

    // Rehash all entries
    size_t actual_size = 0;
    for (size_t i = 0; i < ht->capacity; i++) {
        if (ht->entries[i].key != NULL && !ht->entries[i].is_deleted) {
            size_t index = ht->hash(ht->entries[i].key) % new_capacity;
            while (new_entries[index].key != NULL) {
                index = (index + 1) % new_capacity;
            }
            new_entries[index] = ht->entries[i];
            new_entries[index].is_deleted = false;  // Reset deleted flag
            actual_size++;
        }
    }
    
    // Update size to reflect actual number of entries after rehashing
    ht->size = actual_size;

    // Free old entries and update hashtable
    free(ht->entries);
    ht->entries = new_entries;
    ht->capacity = new_capacity;

    return 0;
}

/**
 * @brief Default hash function for strings. Uses the djb2 algorithm (http://www.cse.yorku.ca/~oz/hash.html)
 * 
 * @param[in] key The key to hash
 * @return size_t The hash value
 */
static size_t _default_hash(void *key) {
    if (key == NULL) {
        return 0;
    }
    char *str = (char *)key;
    size_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

// New utility functions
float ht_load_factor(hashtable *ht) {
    if (ht == NULL) return 0.0f;
    return (float)ht->size / (float)ht->capacity;
}

size_t ht_size(hashtable *ht) {
    return ht ? ht->size : 0;
}

size_t ht_capacity(hashtable *ht) {
    return ht ? ht->capacity : 0;
}

void ht_clear(hashtable *ht) {
    if (ht == NULL) return;
    
    for (size_t i = 0; i < ht->capacity; i++) {
        if (ht->entries[i].key != NULL) {
            if (ht->key_free != NULL) {
                ht->key_free(ht->entries[i].key);
            }
            if (ht->val_free != NULL) {
                ht->val_free(ht->entries[i].value);
            }
            ht->entries[i].key = NULL;
            ht->entries[i].value = NULL;
            ht->entries[i].is_deleted = false;
        }
    }
    ht->size = 0;
}

ht_iterator ht_iterator_create(hashtable *ht) {
    ht_iterator it = {ht, 0};
    return it;
}

bool ht_iterator_next(ht_iterator *it, void **key, void **value) {
    if (it == NULL || it->ht == NULL) return false;
    
    while (it->index < it->ht->capacity) {
        if (it->ht->entries[it->index].key != NULL && !it->ht->entries[it->index].is_deleted) {
            *key = it->ht->entries[it->index].key;
            *value = it->ht->entries[it->index].value;
            it->index++;
            return true;
        }
        it->index++;
    }
    return false;
}
