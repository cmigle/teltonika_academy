#include <stdio.h>
//added so strcpy works as intended
#include <string.h>
#include <stdlib.h>

//the main is trying to copy a string into a null pointer
char *mem_alloc(void)
{
   //buffer size was too small: 43 characters + the terminating null byte
   return malloc(44 * sizeof(char));
}

int main(void)
{
    char *buffer = mem_alloc();
    if (buffer == NULL) {
        return 1;
    }
    strcpy(buffer, "Some random text to be copied to the buffer");
    printf("%s\n", buffer);
    free(buffer);
    return 0;
}
