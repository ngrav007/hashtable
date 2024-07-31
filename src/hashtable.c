#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "hashtable.h"

#define LOAD_FACTOR 0.75
#define RESIZE_FACTOR 2
#define DEFAULT_CAPACITY 16

static size_t _default_hash(void *ket);
static int _ht_resize(hashtable *ht, size_t new_capacity);

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
 * @brief Insert a key-value pair into the hashtable. If the key already exists, the value is updated. If the load
 *  factor exceeds LOAD_FACTOR, the hashtable is resized to RESIZE_FACTOR * capacity.
 * 
 * @param[in] ht The hashtable to insert into
 * @param[in] key The key to insert
 * @param[in] value The value to insert
 * @return int 0 on success, -1 on failure
 */
int ht_insert(hashtable *ht, void *key, void *value) {
    if (ht == NULL || key == NULL) {
        fprintf(stderr, "[-] Hashtable or key is NULL\n");
        return -1;
    }

    if (ht->size >= ht->capacity * LOAD_FACTOR) {
        if (_ht_resize(ht, ht->capacity * RESIZE_FACTOR) != 0) {
            fprintf(stderr, "[-] Failed to resize hashtable\n");
            return -1;
        }
    }

    size_t index = ht->hash(key) % ht->capacity;
    while (ht->entries[index].key != NULL) {
        if (ht->equals(ht->entries[index].key, key)) {
            if (ht->val_free != NULL) {
                ht->val_free(ht->entries[index].value);
            }
            ht->entries[index].value = value;
            return 0;
        }
        index = (index + 1) % ht->capacity;
    }

    ht->entries[index].key = key;
    ht->entries[index].value = value;
    ht->size++;
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
        fprintf(stderr, "[-] Hashtable or key is NULL\n");
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
                    fprintf(stderr, "[-] Failed to resize hashtable\n");
                    return -1;
                }
            }

            return 0;
        }
        index = (index + 1) % ht->capacity;
    }

    fprintf(stderr, "[!] Key not found\n");
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
        fprintf(stderr, "[-] Hashtable or key is NULL\n");
        return NULL;
    }

    size_t index = ht->hash(key) % ht->capacity;
    while (ht->entries[index].key != NULL) {
        if (ht->equals(ht->entries[index].key, key)) {
            fprintf(stderr, "[+] Found key\n");
            return ht->entries[index].value;
        }
        index = (index + 1) % ht->capacity;
    }

    fprintf(stderr, "[!] Key not found\n");
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
        fprintf(stderr, "[-] Hashtable is NULL\n");
        return -1;
    }

    // Allocate new entries array
    ht_entry *new_entries = calloc(new_capacity, sizeof(ht_entry));
    if (new_entries == NULL) {
        fprintf(stderr, "[-] Failed to allocate new entries array\n");
        return -1;
    }

    // Rehash all entries
    for (size_t i = 0; i < ht->capacity; i++) {
        if (ht->entries[i].key != NULL) {
            size_t index = ht->hash(ht->entries[i].key) % new_capacity;
            while (new_entries[index].key != NULL) {
                index = (index + 1) % new_capacity;
            }
            new_entries[index] = ht->entries[i];
        }
    }

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
