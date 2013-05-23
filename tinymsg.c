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

tm_mailbox *my_mailbox, *my_dataBox, *my_freeBox;

static void format_name(uint32_t name, char *cname, const char *suffix, int maxlen)
{
    snprintf(cname, maxlen, "/%x%s", name, suffix);
}

static uint32_t rand_name()
{
    return rand() % 0xFFFFFFFF;
}

static void *create_shared_mapping(const char *name)
{
    int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL);
    if(fd < 0)
    {
        perror("shm open");
        abort();
    }

    ftruncate(fd, MAPPING_LEN);
    void *ret = mmap(0, MAPPING_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(ret == MAP_FAILED)
    {
        perror("mmap");
        abort();
    }

    return ret;
}

int tm_init(unsigned long name)
{
    char cname[128];


    if(!name_table)
    {
        name_fd = shm_open("/tm_names", O_RDWR);
        if(name_fd < 0 && errno == ENOENT)
        {
            // Create the shm
            name_fd = shm_open("/tm_names", O_RDWR | O_CREAT | O_EXCL);
            if(name_fd >= 0)
                ftruncate(name_fd, MAPPING_LEN);
        }

        if(name_fd < 0)
        {
            fprintf(stderr, "Failed to open shm\n");
            return errno;
        }


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


    format_name(name, cname, "_mb", 128);
    my_mailbox = (tm_mailbox*)create_shared_mapping(cname);
    format_name(name, cname, "_data", 128);
    my_dataBox= (tm_mailbox*)create_shared_mapping(cname);
    format_name(name, cname, "_free", 128);
    my_freeBox = (tm_mailbox*)create_shared_mapping(cname);

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


    munmap(my_mailbox, MAPPING_LEN);
    munmap(my_freeBox, MAPPING_LEN);
    munmap(my_dataBox, MAPPING_LEN);

    format_name(my_name_entry->name, cname, "_mb", 128);
    shm_unlink(cname);
    format_name(my_name_entry->name, cname, "_data", 128);
    shm_unlink(cname);
    format_name(my_name_entry->name, cname, "_free", 128);
    shm_unlink(cname);
    my_name_entry->valid = 0;

    munmap(name_table, MAPPING_LEN);
    shm_unlink(cname);
}
