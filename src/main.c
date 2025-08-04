#include "repl.h"
#include <stdio.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Must supply a database filename.\n");
        return 1;
    }

    start_repl(argv[1]);

    return 0;
}