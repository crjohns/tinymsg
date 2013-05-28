#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "tinymsg.h"

const int name = 0x1a1;

int main(int argc, char **argv)
{
    int r;

    r = tm_init(name);

    for(int left = MESSAGE_DATA_SLOTS; left > 0; left--)
    {
        tm_data *buf = tm_alloc();
        tm_free(buf);
    }

    tm_data *buf2 = tm_alloc();
    fprintf(stderr, "last alloc %x %d\n", buf2->owner, buf2->slot);
    if(buf2->owner != name || buf2->slot != 0)
    {
        fprintf(stderr, "Should have been %x 0\n", name);
        return 1;
    }


    return 0;
}
