#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    // fixed size array 
    char user_input[256];
    printf("Enter your text: ");
    fscanf(stdin, "%255[^\n]", user_input);
    printf("\nYour line -> %s\n", user_input);
    return 0;
}
