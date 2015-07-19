#include <stdio.h>
#include <string.h>

# include "parse.h"

unsigned char *test=(unsigned char*)"<a\\Aa 'bb' cc>";

int main (int argc, char *argv[]) {
    unsigned char *inp;
    int rc;

    if (argc > 1 && strcmp(argv[1], "-g") == 0) {
        printg();
        return 0;
    }

    inp = test;
    
    rc = parse(inp);

    return rc;
}
