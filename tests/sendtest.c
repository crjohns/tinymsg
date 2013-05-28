#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "tinymsg.h"

int main(int argc, char **argv)
{
    int name = 0x111;

    int r;

    if(fork())
    {
        r = tm_init(name);
        if(r)
        {
            perror("parent");
            abort();
        }
    }
    else
    {
        r = tm_init(0x1);
        if(r)
        {
            perror("child");
            abort();
        }
    }

    return 0;
}
