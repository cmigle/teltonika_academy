#include <stdio.h>
//added so strcpy works as intended
#include <string.h>
#include <stdlib.h>

//the main is trying to copy a string into a null pointer
char *mem_alloc(void)
{
   //buffer size was too small
   return malloc(43 * sizeof(char));
}

int main(void)
{
    char *buffer = mem_alloc();
    strcpy(buffer, "Some random text to be copied to the buffer");
    printf("%s\n", buffer);
    free(buffer);
    return 0;
}
