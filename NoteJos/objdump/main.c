#include <stdio.h>

void swap(int* first, int* second) {
    int temp = *first;
    *first = *second;
    *second = temp;
}

int main(int argc, char **  argv) {
    int a = 20;
    int b = 10;
    printf("a = %d, b = %d\n", a, b);
    swap(a, b);
    printf("a = %d, b = %d\n",  a, b);

    return 0;
}