#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    /*
     *could create segfaults because it was writing to the memory it did not own
     *I could just give the buffer actual memory on the stack, 
     *similarly to the last code, but there is another way of fixing this  
     *by adding malloc I can allocate memory dynamically, 
     *must later free the buffer.
     */
    char *buffer = malloc(50* sizeof(char));
    if(buffer == NULL)
    { 
	return 1;
    }
    strcpy(buffer, "Some random text");
    printf("%s\n", buffer);
    free(buffer);
    return 0;
}
