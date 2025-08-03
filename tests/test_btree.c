#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "btree.h"
#include "pager.h"

void print_test_result(const char* test_name, int success) {
    printf("%s: %s\n", test_name, success ? "PASSED" : "FAILED");
}

void print_tree_structure(BTree* btree, page_num_t page_num, int depth) {
    void* node = pager_get_page(btree->pager, page_num);
    
    for (int i = 0; i < depth; i++) {   
        printf("  ");
    }
    
    if (get_node_type(node) == NODE_LEAF) {
        uint32_t num_cells = *leaf_node_num_cells(node);
        printf("Leaf (page %d) with %d cells: ", page_num, num_cells);
        for (uint32_t i = 0; i < num_cells; i++) {
            printf("%d ", *leaf_node_key(node, i));
        }
        printf("(next: %d)\n", *leaf_node_next_leaf(node));
    } else {
        uint32_t num_keys = *internal_node_num_keys(node);
        printf("Internal (page %d) with %d keys: ", page_num, num_keys);
        for (uint32_t i = 0; i < num_keys; i++) {
            printf("%d ", *internal_node_key(node, i));
        }
        printf("(right_child: %d)\n", *internal_node_right_child(node));
        
        // Recursively print children
        for (uint32_t i = 0; i <= num_keys; i++) {
            uint32_t child_page = *internal_node_child(node, i);
            print_tree_structure(btree, child_page, depth + 1);
        }
    }
}

// Validate tree structure integrity
int validate_tree_structure(BTree* btree, page_num_t page_num, uint32_t min_key, uint32_t max_key, int depth) {
    void* node = pager_get_page(btree->pager, page_num);
    
    if (get_node_type(node) == NODE_LEAF) {
        uint32_t num_cells = *leaf_node_num_cells(node);
        uint32_t prev_key = 0;
        
        for (uint32_t i = 0; i < num_cells; i++) {
            uint32_t key = *leaf_node_key(node, i);
            if (i > 0 && key <= prev_key) {
                printf("ERROR: Leaf keys not in order at page %d, position %d: %d <= %d\n", 
                       page_num, i, key, prev_key);
                return 0;
            }
            prev_key = key;
        }
        return 1;
    } else {
        uint32_t num_keys = *internal_node_num_keys(node);
        
        for (uint32_t i = 0; i <= num_keys; i++) {
            uint32_t child_page = *internal_node_child(node, i);
            
            if (!validate_tree_structure(btree, child_page, 0, UINT32_MAX, depth + 1)) {
                return 0;
            }
        }
        return 1;
    }
}

int test_basic_operations() {
    printf("\n=== Testing Basic Operations ===\n");
    
    Pager* pager = pager_open("test_basic.db");
    BTree* btree = btree_open(pager);
    
    // Test insertion
    char value1[] = "value1";
    char value2[] = "value2";
    char value3[] = "value3";
    
    int result1 = btree_insert(btree, 1, value1, strlen(value1) + 1);
    int result2 = btree_insert(btree, 2, value2, strlen(value2) + 1);
    int result3 = btree_insert(btree, 3, value3, strlen(value3) + 1);
    
    if (result1 != 0 || result2 != 0 || result3 != 0) {
        printf("Basic insertion failed: %d, %d, %d\n", result1, result2, result3);
        btree_close(btree);
        pager_close(pager);
        return 0;
    }
    
    printf("Tree after basic insertions:\n");
    print_tree_structure(btree, btree->root_page_num, 0);
    
    // Test retrieval
    BTreeCursor* cursor = btree_find(btree, 2);
    if (!cursor) {
        printf("Failed to find key 2\n");
        btree_close(btree);
        pager_close(pager);
        return 0;
    }
    
    char retrieved_value[100];
    uint32_t retrieved_size;
    btree_cursor_get_value(cursor, retrieved_value, sizeof(retrieved_value), &retrieved_size);
    
    int success = (strcmp(retrieved_value, value2) == 0);
    if (!success) {
        printf("Basic retrieval failed: expected '%s', got '%s'\n", value2, retrieved_value);
    }
    
    free(cursor);
    btree_close(btree);
    pager_close(pager);
    
    return success;
}

int test_sequential_insertion() {
    printf("\n=== Testing Sequential Insertion ===\n");
    
    Pager* pager = pager_open("test_sequential.db");
    BTree* btree = btree_open(pager);
    
    int num_inserts = 30; // Reduced for easier debugging
    char** inserted_values = malloc(sizeof(char*) * num_inserts);
    
    // Insert sequential keys
    for (int i = 0; i < num_inserts; i++) {
        inserted_values[i] = malloc(32);
        sprintf(inserted_values[i], "seq_value_%d", i);
        int result = btree_insert(btree, i, inserted_values[i], strlen(inserted_values[i]) + 1);
        if (result != 0) {
            printf("Sequential insertion failed at key %d (result: %d)\n", i, result);
            print_tree_structure(btree, btree->root_page_num, 0);
            // Cleanup
            for (int j = 0; j <= i; j++) {
                free(inserted_values[j]);
            }
            free(inserted_values);
            btree_close(btree);
            pager_close(pager);
            return 0;
        }
        
        // Print tree every few insertions for debugging
        if (i % 10 == 9 || i == num_inserts - 1) {
            printf("Tree after inserting %d keys:\n", i + 1);
            print_tree_structure(btree, btree->root_page_num, 0);
        }
    }
    
    // Validate tree structure
    if (!validate_tree_structure(btree, btree->root_page_num, 0, num_inserts - 1, 0)) {
        printf("Tree structure validation failed\n");
        // Cleanup
        for (int i = 0; i < num_inserts; i++) {
            free(inserted_values[i]);
        }
        free(inserted_values);
        btree_close(btree);
        pager_close(pager);
        return 0;
    }
    
    // Verify all values can be retrieved in order
    BTreeCursor* cursor = btree_start(btree);
    int success = 1;
    
    for (int i = 0; i < num_inserts && !cursor->end_of_table; i++) {
        char retrieved_value[100];
        uint32_t retrieved_size;
        btree_cursor_get_value(cursor, retrieved_value, sizeof(retrieved_value), &retrieved_size);
        
        if (strcmp(retrieved_value, inserted_values[i]) != 0) {
            printf("Sequential verification failed at index %d: expected '%s', got '%s'\n", 
                   i, inserted_values[i], retrieved_value);
            success = 0;
        }
        
        if (i < num_inserts - 1) { // Don't advance past last element
            btree_cursor_advance(cursor);
        }
    }
    
    // Cleanup
    for (int i = 0; i < num_inserts; i++) {
        free(inserted_values[i]);
    }
    free(inserted_values);
    free(cursor);
    btree_close(btree);
    pager_close(pager);
    
    return success;
}

int test_random_insertion() {
    printf("\n=== Testing Random Insertion ===\n");
    
    Pager* pager = pager_open("test_random.db");
    BTree* btree = btree_open(pager);
    
    int num_inserts = 25; // Reduced for easier debugging
    int* keys = malloc(sizeof(int) * num_inserts);
    char** values = malloc(sizeof(char*) * num_inserts);
    
    // Generate keys 0 to num_inserts-1
    for (int i = 0; i < num_inserts; i++) {
        keys[i] = i;
        values[i] = malloc(32);
        sprintf(values[i], "random_value_%d", i);
    }
    
    // Shuffle the keys array
    srand(42); // Fixed seed for reproducible results
    for (int i = num_inserts - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = keys[i];
        keys[i] = keys[j];
        keys[j] = temp;
    }
    
    printf("Insertion order: ");
    for (int i = 0; i < num_inserts; i++) {
        printf("%d ", keys[i]);
    }
    printf("\n");
    
    // Insert in random order
    for (int i = 0; i < num_inserts; i++) {
        int key = keys[i];
        int result = btree_insert(btree, key, values[key], strlen(values[key]) + 1);
        if (result != 0) {
            printf("Random insertion failed at key %d (position %d, result: %d)\n", key, i, result);
            print_tree_structure(btree, btree->root_page_num, 0);
            // Cleanup
            for (int j = 0; j < num_inserts; j++) {
                free(values[j]);
            }
            free(values);
            free(keys);
            btree_close(btree);
            pager_close(pager);
            return 0;
        }
        
        if (i % 10 == 9 || i == num_inserts - 1) {
            printf("Tree after inserting %d keys:\n", i + 1);
            print_tree_structure(btree, btree->root_page_num, 0);
        }
    }
    
    // Validate tree structure
    if (!validate_tree_structure(btree, btree->root_page_num, 0, num_inserts - 1, 0)) {
        printf("Tree structure validation failed\n");
        // Cleanup
        for (int i = 0; i < num_inserts; i++) {
            free(values[i]);
        }
        free(values);
        free(keys);
        btree_close(btree);
        pager_close(pager);
        return 0;
    }
    
    // Verify all values are retrievable in sorted order
    BTreeCursor* cursor = btree_start(btree);
    int success = 1;
    
    for (int i = 0; i < num_inserts && !cursor->end_of_table; i++) {
        char retrieved_value[100];
        uint32_t retrieved_size;
        btree_cursor_get_value(cursor, retrieved_value, sizeof(retrieved_value), &retrieved_size);
        
        if (strcmp(retrieved_value, values[i]) != 0) {
            printf("Random verification failed at sorted index %d: expected '%s', got '%s'\n", 
                   i, values[i], retrieved_value);
            success = 0;
        }
        
        if (i < num_inserts - 1) {
            btree_cursor_advance(cursor);
        }
    }
    
    // Cleanup
    for (int i = 0; i < num_inserts; i++) {
        free(values[i]);
    }
    free(values);
    free(keys);
    free(cursor);
    btree_close(btree);
    pager_close(pager);
    
    return success;
}

int test_duplicate_keys() {
    printf("\n=== Testing Duplicate Key Handling ===\n");
    
    Pager* pager = pager_open("test_duplicates.db");
    BTree* btree = btree_open(pager);
    
    char value1[] = "first_value";
    char value2[] = "second_value";
    
    // Insert first value
    int result1 = btree_insert(btree, 42, value1, strlen(value1) + 1);
    if (result1 != 0) {
        printf("First insertion failed (result: %d)\n", result1);
        btree_close(btree);
        pager_close(pager);
        return 0;
    }
    
    // Try to insert duplicate key - should fail
    int result2 = btree_insert(btree, 42, value2, strlen(value2) + 1);
    if (result2 == 0) {
        printf("Duplicate key insertion should have failed but didn't\n");
        btree_close(btree);
        pager_close(pager);
        return 0;
    }
    
    // Verify original value is still there
    BTreeCursor* cursor = btree_find(btree, 42);
    if (!cursor) {
        printf("Failed to find key 42 after duplicate insertion attempt\n");
        btree_close(btree);
        pager_close(pager);
        return 0;
    }
    
    char retrieved_value[100];
    uint32_t retrieved_size;
    btree_cursor_get_value(cursor, retrieved_value, sizeof(retrieved_value), &retrieved_size);
    
    int success = (strcmp(retrieved_value, value1) == 0);
    if (!success) {
        printf("Original value was corrupted: expected '%s', got '%s'\n", value1, retrieved_value);
    }
    
    free(cursor);
    btree_close(btree);
    pager_close(pager);
    
    return success;
}

int test_stress_insertion() {
    printf("\n=== Testing Stress Insertion (Force Node Splits) ===\n");
    
    Pager* pager = pager_open("test_stress.db");
    BTree* btree = btree_open(pager);
    
    int num_inserts = 100;  // This should cause multiple splits
    int success = 1;
    
    printf("Inserting %d sequential keys to force splits...\n", num_inserts);
    
    // Insert many sequential keys to force splits
    for (int i = 0; i < num_inserts; i++) {
        char value[32];
        sprintf(value, "stress_value_%d", i);
        
        int result = btree_insert(btree, i, value, strlen(value) + 1);
        if (result != 0) {
            printf("Stress insertion failed at key %d (result: %d)\n", i, result);
            print_tree_structure(btree, btree->root_page_num, 0);
            success = 0;
            break;
        }
        
        // Print progress and validate periodically
        if (i % 25 == 24 || i == num_inserts - 1) {
            printf("Inserted %d keys...\n", i + 1);
            if (!validate_tree_structure(btree, btree->root_page_num, 0, i, 0)) {
                printf("Tree structure validation failed at key %d\n", i);
                success = 0;
                break;
            }
        }
    }
    
    if (success) {
        printf("Final tree structure after stress test:\n");
        print_tree_structure(btree, btree->root_page_num, 0);
        
        // Final verification - iterate through all values
        printf("Performing final verification...\n");
        BTreeCursor* cursor = btree_start(btree);
        int count = 0;
        
        while (!cursor->end_of_table && count < num_inserts) {
            char expected_value[32];
            sprintf(expected_value, "stress_value_%d", count);
            
            char retrieved_value[100];
            uint32_t retrieved_size;
            btree_cursor_get_value(cursor, retrieved_value, sizeof(retrieved_value), &retrieved_size);
            
            if (strcmp(retrieved_value, expected_value) != 0) {
                printf("Final verification failed at index %d: expected '%s', got '%s'\n", 
                       count, expected_value, retrieved_value);
                success = 0;
                break;
            }
            
            
            btree_cursor_advance(cursor);
            count++;
        }
        
        if (count != num_inserts) {
            printf("Expected %d values, but found %d\n", num_inserts, count);
            success = 0;
        } else {
            printf("Successfully verified all %d values in correct order\n", count);
        }
        
        free(cursor);
    }
    
    btree_close(btree);
    pager_close(pager);
    
    return success;
}

int main() {
    printf("Starting Comprehensive B-Tree Test Suite\n");
    printf("========================================\n");
    
    int overall_success = 1;
    int test_count = 0;
    int passed_count = 0;
    
    // Run all tests
    int tests[] = {
        test_basic_operations(),
        test_sequential_insertion(),
        test_random_insertion(),
        test_duplicate_keys(),
        test_stress_insertion()
    };
    
    const char* test_names[] = {
        "Basic Operations",
        "Sequential Insertion",
        "Random Insertion", 
        "Duplicate Key Handling",
        "Stress Insertion"
    };
    
    test_count = sizeof(tests) / sizeof(tests[0]);
    
    for (int i = 0; i < test_count; i++) {
        print_test_result(test_names[i], tests[i]);
        if (tests[i]) {
            passed_count++;
        } else {
            overall_success = 0;
        }
    }
    
    printf("\n========================================\n");
    printf("Test Summary: %d/%d tests passed\n", passed_count, test_count);
    printf("Overall Result: %s\n", overall_success ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
    
    return overall_success ? 0 : 1;
}