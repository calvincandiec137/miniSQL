#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vm.h"
#include "btree.h"
#include "pager.h"

void print_test_result(const char* test_name, int success) {
    printf("%s: %s\n", test_name, success ? "PASSED" : "FAILED");
}

int test_prepare_statement() {
    printf("\n=== Testing Prepare Statement ===\n");
    Statement statement;
    int success = 1;

    // Test valid insert
    PrepareResult result = prepare_statement("insert 1 user1 user1@example.com", &statement);
    if (result != PREPARE_SUCCESS || statement.type != STATEMENT_INSERT) {
        printf("Valid insert statement failed.\n");
        success = 0;
    }

    // Test valid select
    result = prepare_statement("select", &statement);
    if (result != PREPARE_SUCCESS || statement.type != STATEMENT_SELECT) {
        printf("Valid select statement failed.\n");
        success = 0;
    }

    // Test unrecognized statement
    result = prepare_statement("delete", &statement);
    if (result != PREPARE_UNRECOGNIZED_STATEMENT) {
        printf("Unrecognized statement test failed.\n");
        success = 0;
    }

    // Test syntax error
    result = prepare_statement("insert 1 user1", &statement);
    if (result != PREPARE_SYNTAX_ERROR) {
        printf("Syntax error test failed.\n");
        success = 0;
    }

    return success;
}

int test_execute_statement() {
    printf("\n=== Testing Execute Statement ===\n");
    Pager* pager = pager_open("test_vm.db");
    BTree* btree = btree_open(pager);
    Statement statement;
    int success = 1;

    // Test insert
    prepare_statement("insert 1 user1 user1@example.com", &statement);
    execute_statement(&statement, btree);

    // Verify insert
    BTreeCursor* cursor = btree_find(btree, 1);
    if (cursor->end_of_table) {
        printf("Insert verification failed.\n");
        success = 0;
    }
    free(cursor);

    // Test select
    // This is harder to test automatically without redirecting stdout.
    // For now, we'll just execute it and check for crashes.
    prepare_statement("select", &statement);
    execute_statement(&statement, btree);

    btree_close(btree);
    pager_close(pager);
    remove("test_vm.db");


    return success;
}

int test_insert_and_select() {
    printf("\n=== Testing Insert and Select ===\n");
    Pager* pager = pager_open("test_db.db");
    BTree* btree = btree_open(pager);
    Statement statement;
    int success = 1;

    // Insert some rows
    prepare_statement("insert 1 user1 user1@example.com", &statement);
    execute_statement(&statement, btree);
    prepare_statement("insert 2 user2 user2@example.com", &statement);
    execute_statement(&statement, btree);
    prepare_statement("insert 3 user3 user3@example.com", &statement);
    execute_statement(&statement, btree);

    // Select the rows and verify them
    BTreeCursor* cursor = btree_start(btree);
    Row row;
    void* value = malloc(ROW_SIZE);
    uint32_t value_size;
    uint32_t i = 0;
    while(!cursor->end_of_table) {
        i++;
        btree_cursor_get_value(cursor, value, ROW_SIZE, &value_size);
        deserialize_row(value, &row);
        if (row.id != i) {
            printf("Select verification failed: expected id %d, got %d\n", i, row.id);
            success = 0;
        }
        char username[COLUMN_USERNAME_SIZE];
        sprintf(username, "user%d", i);
        if (strcmp(row.username, username) != 0) {
            printf("Select verification failed: expected username %s, got %s\n", username, row.username);
            success = 0;
        }
        char email[COLUMN_EMAIL_SIZE];
        sprintf(email, "user%d@example.com", i);
        if (strcmp(row.email, email) != 0) {
            printf("Select verification failed: expected email %s, got %s\n", email, row.email);
            success = 0;
        }
        btree_cursor_advance(cursor);
    }
    free(value);
    free(cursor);

    if (i != 3) {
        printf("Select verification failed: expected 3 rows, got %d\n", i);
        success = 0;
    }


    btree_close(btree);
    pager_close(pager);
    remove("test_db.db");

    return success;
}

int test_long_strings() {
    printf("\n=== Testing Long Strings ===\n");
    Pager* pager = pager_open("test_long.db");
    BTree* btree = btree_open(pager);
    Statement statement;
    int success = 1;

    char long_username[COLUMN_USERNAME_SIZE];
    memset(long_username, 'a', COLUMN_USERNAME_SIZE - 1);
    long_username[COLUMN_USERNAME_SIZE - 1] = '\0';

    char long_email[COLUMN_EMAIL_SIZE];
    memset(long_email, 'b', COLUMN_EMAIL_SIZE - 1);
    long_email[COLUMN_EMAIL_SIZE - 1] = '\0';

    char insert_statement[512];
    sprintf(insert_statement, "insert 1 %s %s", long_username, long_email);

    prepare_statement(insert_statement, &statement);
    execute_statement(&statement, btree);

    BTreeCursor* cursor = btree_start(btree);
    Row row;
    void* value = malloc(ROW_SIZE);
    uint32_t value_size;

    btree_cursor_get_value(cursor, value, ROW_SIZE, &value_size);
    deserialize_row(value, &row);

    if (row.id != 1 || strcmp(row.username, long_username) != 0 || strcmp(row.email, long_email) != 0) {
        printf("Long string test failed.\n");
        success = 0;
    }

    free(value);
    free(cursor);
    btree_close(btree);
    pager_close(pager);
    remove("test_long.db");

    return success;
}

int test_duplicate_key() {
    printf("\n=== Testing Duplicate Key ===\n");
    Pager* pager = pager_open("test_duplicate.db");
    BTree* btree = btree_open(pager);
    Statement statement;
    int success = 1;

    prepare_statement("insert 1 user1 user1@example.com", &statement);
    execute_statement(&statement, btree);

    // Try to insert the same key again
    int result = btree_insert(btree, 1, &statement.row_to_insert, ROW_SIZE);
    if (result != -1) {
        printf("Duplicate key test failed.\n");
        success = 0;
    }

    btree_close(btree);
    pager_close(pager);
    remove("test_duplicate.db");

    return success;
}


int main() {
    printf("Starting VM Test Suite\n");
    printf("======================\n");

    int overall_success = 1;
    int test_count = 0;
    int passed_count = 0;

    int (*tests[])() = {
        test_prepare_statement,
        test_execute_statement,
        test_insert_and_select,
        test_long_strings,
        test_duplicate_key
    };

    const char* test_names[] = {
        "Prepare Statement",
        "Execute Statement",
        "Insert and Select",
        "Long Strings",
        "Duplicate Key"
    };

    test_count = sizeof(tests) / sizeof(tests[0]);

    for (int i = 0; i < test_count; i++) {
        int result = tests[i]();
        print_test_result(test_names[i], result);
        if (result) {
            passed_count++;
        } else {
            overall_success = 0;
        }
    }

    printf("\n======================\n");
    printf("Test Summary: %d/%d tests passed\n", passed_count, test_count);
    printf("Overall Result: %s\n", overall_success ? "ALL TESTS PASSED" : "SOME TESTS FAILED");

    return overall_success ? 0 : 1;
}
