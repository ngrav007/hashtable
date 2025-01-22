#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef struct ht_entry {
    void *key;
    void *value;
    bool is_deleted;
} ht_entry;

typedef struct hashtable {
    ht_entry *entries;
    size_t size;
    size_t capacity;
    size_t (*hash)(void *);
    bool (*equals)(void *, void *);
    void (*key_free)(void *);
    void (*val_free)(void *);
} hashtable;

// Debug control
extern bool ht_debug_enabled;

// Additional utility functions
float ht_load_factor(hashtable *ht);
size_t ht_size(hashtable *ht);
size_t ht_capacity(hashtable *ht);
void ht_clear(hashtable *ht);

// Iterator interface
typedef struct ht_iterator {
    hashtable *ht;
    size_t index;
} ht_iterator;

ht_iterator ht_iterator_create(hashtable *ht);
bool ht_iterator_next(ht_iterator *it, void **key, void **value);

hashtable *ht_create(size_t capacity, size_t (*hash)(void *), bool (*equals)(void *, void *), void (*key_free)(void *), void (*val_free)(void *));
void ht_destroy(hashtable *ht);
int ht_insert(hashtable *ht, void *key, void *value);
int ht_remove(hashtable *ht, void *key);
void *ht_get(hashtable *ht, void *key);

#ifdef __cplusplus
}
#endif

#endif
