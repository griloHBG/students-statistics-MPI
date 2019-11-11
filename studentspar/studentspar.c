#include <stdio.h>

#define EXAMPLE

#ifdef EXAMPLE
    double matrix[] = { 9,   8,   4,   5,
                        4,   12,  20,  40,
                        8,   8,   4,   4,
                        8,   12,  4,   21,
                        33,  44,  20,  1,
                        10,  18,  17,  10,
                        };
#else
    double* matrix;
#endif

int main()
{
    printf("Hello, World!\n");
    return 0;
}