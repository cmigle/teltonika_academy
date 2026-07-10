#include <stdio.h>
#include <stdlib.h>

//added string. h for strcat
#include <string.h>

#include <time.h>

void genRandomText(int count, char *buffer)
{
    int i;
    for (i = 0; i < count; i++) {
	//fixed off by one overflow
        char symbol[2];
        int num = (rand() % (90 - 65 + 1)) + 65;
        sprintf(symbol,"%c", num);
        strcat(buffer, symbol);
    }
    buffer[count] = '\0';
}

int main()
{
    char *buffer = NULL;
    int i = 1;
    srand(time(0));
  
    while(i < 7) {
	// initializing the buffer as an empty string 
        buffer = malloc(i * sizeof(char) + 1);
	// initializing the buffer as an empty string
        buffer[0]= '\0';
	genRandomText(i, buffer);
        printf("Buffer - %s\n", buffer);
	//free at the end of every iteration
	free(buffer);
        i++;
    }

    return 0;
}
