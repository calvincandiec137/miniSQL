#include "vm.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void print_row(Row* row) {
    printf("(%d, %s, %s)\n", row->id, row->username, row->email);
}

void serialize_row(Row* source, void* destination) {
    memcpy(destination + ID_OFFSET, &(source->id), ID_SIZE);
    memcpy(destination + USERNAME_OFFSET, source->username, USERNAME_SIZE);
    memcpy(destination + EMAIL_OFFSET, source->email, EMAIL_SIZE);
}

void deserialize_row(void* source, Row* destination) {
    memcpy(&(destination->id), source + ID_OFFSET, ID_SIZE);
    memcpy(&(destination->username), source + USERNAME_OFFSET, USERNAME_SIZE);
    memcpy(&(destination->email), source + EMAIL_OFFSET, EMAIL_SIZE);
}

PrepareResult prepare_statement(const char* input, Statement* statement) {
    if (strncmp(input, "insert", 6) == 0) {
        statement->type = STATEMENT_INSERT;
        int args_assigned = sscanf(input, "insert %d %s %s", &(statement->row_to_insert.id),
                                     statement->row_to_insert.username, statement->row_to_insert.email);
        if (args_assigned < 3) {
            return PREPARE_SYNTAX_ERROR;
        }
        return PREPARE_SUCCESS;
    }
    if (strcmp(input, "select") == 0) {
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }

    return PREPARE_UNRECOGNIZED_STATEMENT;
}

void execute_statement(Statement* statement, BTree* btree) {
    switch (statement->type) {
        case STATEMENT_INSERT:
            {
                void* value = malloc(ROW_SIZE);
                serialize_row(&(statement->row_to_insert), value);
                btree_insert(btree, statement->row_to_insert.id, value, ROW_SIZE);
                free(value);
                printf("Executed.\n");
                break;
            }
        case STATEMENT_SELECT:
            {
                BTreeCursor* cursor = btree_start(btree);

                if (cursor->end_of_table) {
                    free(cursor);
                    break;
                }

                Row row;
                void* value = malloc(ROW_SIZE);
                uint32_t value_size;

                while (!cursor->end_of_table) {
                    btree_cursor_get_value(cursor, value, ROW_SIZE, &value_size);
                    deserialize_row(value, &row);
                    print_row(&row);
                    btree_cursor_advance(cursor);
                }

                free(value);
                free(cursor);
                break;
            }
    }
}