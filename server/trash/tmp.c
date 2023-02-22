#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

int main()
{
    int i = 5;
    while (i)
    {
        char *this = (char *)malloc(100);
        i--;
        if (strlen(this) == 0)
        {
            printf("zero\n");
        }
    }

    return 0;
}
