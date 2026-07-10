#include <stdio.h>

int main(void)
{
    char copy_from[30] = "original_file.txt";
    char copy_to[30] = "copied_file.txt";

    FILE *original = NULL;
    FILE *copy = NULL;

    original = fopen(copy_from, "r");
    
    // if the original file can't be opened fopen might return null
    if(original == NULL)
    {
	fprintf(stderr, "Could not open %s\n", copy_from);
	return 1;
    }

    char line[500];
    
    //the file was being reopened on every iteration
    copy = fopen(copy_to, "w");

    //if the file to copy to was not created
    if(copy == NULL)
    {
	fprintf(stderr, "Could not open %s\n", copy_to);
	fclose(original);
	return 1;
    }

    while (fgets(line, sizeof(line), original)) {
        fprintf(copy, "%s", line);
    }

    //the files are now being closed only outside of the iterations as well
    fclose(copy);
    fclose(original);
    return 0;
}
