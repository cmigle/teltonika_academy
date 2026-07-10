#include <stdio.h>


void number_printer(int *var)
{
    // program needs to print value not the adress the pointer is pointing to 
    int value = *var;
    printf("Number is - %d\n", value);
}

int main(void)
{
    int var = 345;
    number_printer(&var);
    return 0;
}
