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

hashtable *ht_create(size_t capacity, size_t (*hash)(void *), bool (*equals)(void *, void *), void (*key_free)(void *), void (*val_free)(void *));
void ht_destroy(hashtable *ht);
int ht_insert(hashtable *ht, void *key, void *value);
int ht_remove(hashtable *ht, void *key);
void *ht_get(hashtable *ht, void *key);

#ifdef __cplusplus
}
#endif

#endif
