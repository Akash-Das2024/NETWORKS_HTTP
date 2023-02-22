#include <stdio.h>
#include <sys/stat.h> // for mkdir()
#include <unistd.h>

int main()
{

    char * args[] = {"mkdir", "-p", "folder1/folder2/folder3", NULL};
    execvp(args[0], args);
    return 0;
}
