#include <gtest/gtest.h>
#include <stdlib.h>
#include <string.h>
#include "hashtable.h"

bool string_equals(void *a, void *b) {
    return strcmp((char*)a, (char*)b) == 0;
}

void free_key(void *key) {
    free(key);
}

void free_val(void *val) {
    free(val);
}

// Test creation of hashtable
TEST(HashTableTest, Create) {
    hashtable *ht = ht_create(16, NULL, string_equals, free_key, free_val);
    ASSERT_NE(ht, nullptr);
    ASSERT_EQ(ht->size, 0);
    ASSERT_EQ(ht->capacity, 16);
    ht_destroy(ht);
}

// Test insertion into hashtable
TEST(HashTableTest, Insert) {
    hashtable* ht = ht_create(16, NULL, string_equals, free_key, free_val);

    char *key = strdup("key");
    char *value = strdup("value");
    int result = ht_insert(ht, key, value);
    ASSERT_EQ(result, 0);
    ASSERT_EQ(ht->size, 1);

    ht_destroy(ht);
}

// Test insertion and retrieval from hashtable
TEST(HashTableTest, InsertAndGet) {
    hashtable* ht = ht_create(16, NULL, string_equals, free_key, free_val);

    char *key = strdup("key");
    char *value = strdup("value");
    ht_insert(ht, key, value);

    char *retrieved_value = (char*)ht_get(ht, key);
    ASSERT_STREQ(retrieved_value, value);

    ht_destroy(ht);
}

// Test insertion and updating value
TEST(HashTableTest, InsertAndUpdate) {
    hashtable* ht = ht_create(16, NULL, string_equals, free_key, free_val);

    char *key = strdup("key");
    char *value1 = strdup("value1");
    char *value2 = strdup("value2");

    ht_insert(ht, key, value1);
    ht_insert(ht, key, value2);

    char *retrieved_value = (char*)ht_get(ht, key);
    ASSERT_STREQ(retrieved_value, value2);

    ht_destroy(ht);
}

// Test removal from hashtable
TEST(HashTableTest, Remove) {
    hashtable* ht = ht_create(16, NULL, string_equals, free_key, free_val);

    char *key = strdup("key");
    char *value = strdup("value");
    ht_insert(ht, key, value);

    int result = ht_remove(ht, key);
    ASSERT_EQ(result, 0);
    ASSERT_EQ(ht->size, 0);

    key = strdup("key");    // key is freed in ht_remove, so we need to allocate a new one
    char *retrieved_value = (char*)ht_get(ht, key);
    ASSERT_EQ(retrieved_value, nullptr);

    ht_destroy(ht);
}

// Test resizing of hashtable
TEST(HashTableTest, Resize) {
    hashtable* ht = ht_create(2, NULL, string_equals, free_key, free_val);

    for (int i = 0; i < 10; i++) {
        char *key = (char*)malloc(20);
        char *value = (char*)malloc(20);
        snprintf(key, 20, "key%d", i);
        snprintf(value, 20, "value%d", i);
        ht_insert(ht, key, value);
    }

    ASSERT_GT(ht->capacity, 2);

    for (int i = 0; i < 10; i++) {
        char key[20];
        snprintf(key, 20, "key%d", i);
        char *retrieved_value = (char*)ht_get(ht, key);
        ASSERT_NE(retrieved_value, nullptr);
    }

    ht_destroy(ht);
}

// Test removal and automatic shrinking of hashtable
TEST(HashTableTest, RemoveAndShrink) {
    hashtable* ht = ht_create(4, NULL, string_equals, free_key, free_val);

    for (int i = 0; i < 5; i++) {
        char *key = (char*)malloc(20);
        char *value = (char*)malloc(20);
        snprintf(key, 20, "key%d", i);
        snprintf(value, 20, "value%d", i);
        ht_insert(ht, key, value);
    }

    for (int i = 0; i < 5; i++) {
        char key[20];
        snprintf(key, 20, "key%d", i);
        ht_remove(ht, key);
    }

    ASSERT_LT(ht->capacity, 8); // Assuming the table shrinks when below LOAD_FACTOR

    ht_destroy(ht);
}

TEST(HashTableTest, StressTestInsert) {
    const size_t num_entries = 100000;
    hashtable* ht = ht_create(16, NULL, string_equals, free_key, free_val);

    for (size_t i = 0; i < num_entries; ++i) {
        char* key = (char*)malloc(20);
        char* value = (char*)malloc(20);
        snprintf(key, 20, "key%zu", i);
        snprintf(value, 20, "value%zu", i);
        ht_insert(ht, key, value);
    }

    ASSERT_EQ(ht->size, num_entries);

    for (size_t i = 0; i < num_entries; ++i) {
        char key[20];
        snprintf(key, 20, "key%zu", i);
        char* retrieved_value = (char*)ht_get(ht, key);
        ASSERT_NE(retrieved_value, nullptr);
    }

    ht_destroy(ht);
}

TEST(HashTableTest, InsertNullKey) {
    hashtable* ht = ht_create(16, NULL, string_equals, free_key, free_val);
    int result = ht_insert(ht, NULL, strdup("value"));
    ASSERT_EQ(result, -1);
    ht_destroy(ht);
}

TEST(HashTableTest, InsertNullValue) {
    hashtable* ht = ht_create(16, NULL, string_equals, free_key, free_val);
    char* key = strdup("key");
    int result = ht_insert(ht, key, NULL);
    ASSERT_EQ(result, 0);
    ASSERT_EQ(ht->size, 1);
    ht_destroy(ht);
}

TEST(HashTableTest, MemoryManagement) {
    hashtable* ht = ht_create(16, NULL, string_equals, free_key, free_val);

    char* key = strdup("key");
    char* value = strdup("value");
    ht_insert(ht, key, value);

    ht_remove(ht, key);

    // Ensure the key and value are freed properly
    ASSERT_EQ(ht->size, 0);

    ht_destroy(ht);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
