#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include "tinymsg.h"

#define MAPPING_LEN (4096*1024)

int name_fd;
tm_name_header *name_table = 0;
tm_name_entry *my_name_entry;

static void format_name(unsigned int name, char *cname, int maxlen)
{
    snprintf(cname, maxlen, "/%x", name);
}

int tm_init(unsigned long name)
{
    char cname[128];

    format_name(name, cname, 128);

    if(!name_table)
    {
        name_fd = shm_open("/tm_names", O_RDWR | O_CREAT);
        if(name_fd < 0)
        {
            fprintf(stderr, "Failed to open shm\n");
            return errno;
        }

        ftruncate(name_fd, MAPPING_LEN);

        name_table = (tm_name_header*) mmap(0, MAPPING_LEN, PROT_READ | PROT_WRITE, MAP_SHARED,
                name_fd, 0);
        if(name_table == MAP_FAILED)
        {
            perror("failed to mmap");
            shm_unlink(cname);
            return errno;
        }
    }

    unsigned int myslot = __sync_fetch_and_add(&name_table->curslot, 1);

    if(myslot >= NAME_ENTRIES)
    {
        return -1;
    }

    my_name_entry = &name_table->entries[myslot];
    my_name_entry->name = name;
    __sync_synchronize();
    my_name_entry->valid = 1;

    atexit(tm_cleanup);

    return 0;
}

void tm_cleanup(void)
{
    char cname[128];
    format_name(my_name_entry->name, cname, 128);
    my_name_entry->valid = 0;
    munmap(name_table, MAPPING_LEN);
    shm_unlink(cname);
}
