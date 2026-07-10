#include <stdio.h>
//added so strcpy works
#include <string.h>
#include <stdlib.h>

//added a function prototype before main so the function can be called;
void text_printer(char *msg);

int main(void)
{
    char *buffer;
    buffer = malloc(25);
    strcpy(buffer, "Some random text");
    text_printer(buffer);
    free(buffer);
    return 0;
}

/*
 * text_printer frees the memory that buffer points to. 
 * For this I must choose only one place to free, either the main 
 * or the function, while both are viable it is generally the best 
 * practice to free the memory where you allocate it, 
 * thus i left it in main only.
 */

void text_printer(char *msg)
{
    printf("%s\n", msg);
}


