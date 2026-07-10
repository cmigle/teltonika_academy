#include <stdio.h>
#include <string.h>

int main(void)
{
    //writing 38 bytes into a region of size over 20 overflows the destination, increasing the buffer size to 38 fixes the problem.
    char buffer[38];
    strcpy(buffer, "Copy this text to the buffer variable");
    printf("%s\n", buffer);
    return 0;
}
