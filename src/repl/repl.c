#include "repl.h"
#include "pager.h"
#include "btree.h"
#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_prompt() { printf("db > "); }

void read_input(char* buffer) {
    if (fgets(buffer, 255, stdin) == NULL) {
        printf("\n");
        exit(EXIT_SUCCESS);
    }
    buffer[strcspn(buffer, "\n")] = 0;
}

void start_repl(const char* db_filename) {
    Pager* pager = pager_open(db_filename);
    BTree* btree = btree_open(pager);
    char input_buffer[256];

    while (1) {
        print_prompt();
        read_input(input_buffer);

        if (strcmp(input_buffer, ".exit") == 0) {
            btree_close(btree);
            pager_close(pager);
            exit(EXIT_SUCCESS);
        } else {
            Statement statement;
            PrepareResult prepare_result = prepare_statement(input_buffer, &statement);

            switch (prepare_result) {
                case PREPARE_SUCCESS:
                    execute_statement(&statement, btree);
                    break;
                case PREPARE_SYNTAX_ERROR:
                    printf("Syntax error. Could not parse statement.\n");
                    break;
                case PREPARE_UNRECOGNIZED_STATEMENT:
                    printf("Unrecognized keyword at start of '%s'.\n", input_buffer);
                    break;
            }
        }
    }
}
