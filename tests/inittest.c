#include <stdio.h>
#include "tinymsg.h"

int main(int argc, char **argv)
{
    int r = tm_init(0x1);
    return r;
}
