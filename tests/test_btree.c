#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "btree.h"
#include "pager.h"

void print_test_result(const char* test_name, int success) {
    printf("%s: %s\n", test_name, success ? "PASSED" : "FAILED");
}

int main() {
    srand(time(NULL));
    int overall_success = 1;

    // Test 1: Sequential Insertion
    int test1_passed = 1;
    Pager* pager_seq = pager_open("test_seq.db");
    BTree* btree_seq = btree_open(pager_seq);
    
    int num_inserts = 100;
    char** inserted_values = malloc(sizeof(char*) * num_inserts);

    for (int i = 0; i < num_inserts; ++i) {
        inserted_values[i] = malloc(16);
        sprintf(inserted_values[i], "val%d", i);
        btree_insert(btree_seq, i, inserted_values[i], strlen(inserted_values[i]) + 1);
    }

    BTreeCursor* cursor_seq = btree_find(btree_seq, 0);
    for (int i = 0; i < num_inserts; ++i) {
        char* out_value;
        uint32_t out_size;
        btree_cursor_get_value(cursor_seq, (void**)&out_value, &out_size);
        
        if (strcmp(out_value, inserted_values[i]) != 0) {
            test1_passed = 0;
            printf("Sequential insert FAIL: key %d | expected '%s', got '%s'\n", i, inserted_values[i], out_value);
        }
        free(out_value); 
        btree_cursor_advance(cursor_seq);
    }

    for (int i = 0; i < num_inserts; ++i) {
        free(inserted_values[i]); 
    }
    free(inserted_values);

    print_test_result("Sequential Insertion", test1_passed);
    if (!test1_passed) overall_success = 0;
    
    free(cursor_seq);
    btree_close(btree_seq);
    pager_close(pager_seq);

    // Test 2: Random Insertion
    int test2_passed = 1;
    Pager* pager_rand = pager_open("test_rand.db");
    BTree* btree_rand = btree_open(pager_rand);
    
    int rand_inserts = 100;
    int* keys = malloc(sizeof(int) * rand_inserts);
    for(int i=0; i < rand_inserts; ++i) keys[i] = i;
    for(int i=0; i < rand_inserts; ++i) {
        int j = i + rand() / (RAND_MAX / (rand_inserts - i) + 1);
        int t = keys[j];
        keys[j] = keys[i];
        keys[i] = t;
    }

    char** rand_inserted_values = malloc(sizeof(char*) * rand_inserts);
    for (int i = 0; i < rand_inserts; ++i) {
        rand_inserted_values[keys[i]] = malloc(16);
        sprintf(rand_inserted_values[keys[i]], "val%d", keys[i]);
        btree_insert(btree_rand, keys[i], rand_inserted_values[keys[i]], strlen(rand_inserted_values[keys[i]]) + 1);
    }
    
    BTreeCursor* cursor_rand = btree_find(btree_rand, 0);
    for (int i = 0; i < rand_inserts; ++i) {
        char* out_value;
        uint32_t out_size;
        btree_cursor_get_value(cursor_rand, (void**)&out_value, &out_size);

        if (strcmp(out_value, rand_inserted_values[i]) != 0) {
            test2_passed = 0;
            printf("Random insert FAIL: key %d | expected '%s', got '%s'\n", i, rand_inserted_values[i], out_value);
        }
        free(out_value);
        btree_cursor_advance(cursor_rand);
    }
    
    for (int i = 0; i < rand_inserts; ++i) {
        free(rand_inserted_values[i]);
    }
    free(rand_inserted_values);

    print_test_result("Random Insertion", test2_passed);
    if (!test2_passed) overall_success = 0;

    free(keys);
    free(cursor_rand);
    btree_close(btree_rand);
    pager_close(pager_rand);

    return overall_success ? 0 : 1;
}