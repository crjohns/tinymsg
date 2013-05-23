#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include "sglib.h"
#include "tinymsg.h"

#define MAPPING_LEN (4096*1024)

int name_fd;
tm_name_header *name_table = 0;
tm_name_entry *my_name_entry;
tm_mailbox *mailbox, *freeBox;
tm_data_buffer *dataBox;

typedef struct name_map
{
    uint32_t name;
    void *buffer;
    struct name_map *next;
} name_map;

#define TAB_SIZE 1024
#define NAME_MAP_COMP(a,b) (a->name - b->name)
unsigned int name_map_hash(name_map *nm)
{
    return nm->name;
}

SGLIB_DEFINE_LIST_PROTOTYPES(name_map, NAME_MAP_COMP, next);
SGLIB_DEFINE_LIST_FUNCTIONS(name_map, NAME_MAP_COMP, next);
SGLIB_DEFINE_HASHED_CONTAINER_PROTOTYPES(name_map, TAB_SIZE, name_map_hash);
SGLIB_DEFINE_HASHED_CONTAINER_FUNCTIONS(name_map, TAB_SIZE, name_map_hash);

name_map *mbMap[TAB_SIZE];
name_map *dataMap[TAB_SIZE];
name_map *freeMap[TAB_SIZE];


static void format_name(uint32_t name, char *cname, const char *suffix, int maxlen)
{
    snprintf(cname, maxlen, "/%x%s", name, suffix);
}

static uint32_t rand_name()
{
    return rand() % 0xFFFFFFFF;
}


static void *open_shared_mapping(const char *name)
{
    int fd = shm_open(name, O_RDWR);
    if(fd < 0)
    {
        perror("shm open");
        abort();
    }

    void *ret = mmap(0, MAPPING_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(ret == MAP_FAILED)
    {
        perror("mmap");
        abort();
    }

    return ret;
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

static void *get_shared_region(unsigned long name, name_map **table, const char *suffix)
{
    name_map *elem;
    name_map search;
    search.name = name;

    elem = sglib_hashed_name_map_find_member(table, &search);
    if(elem != NULL)
        return elem->buffer;
    else
    {
        char cname[128];
        void *ret;

        format_name(name, cname, suffix, 128);
        elem = (name_map*)malloc(sizeof(name_map));
        ret = open_shared_mapping(cname);
        elem->name = name;
        elem->buffer = ret;
        sglib_hashed_name_map_add(dataMap, elem);
        return ret;
    }
}

#define get_mailbox(name) get_shared_region(name, mbMap, "")
#define get_freebox(name) get_shared_region(name, freeMap, "_free")
#define get_data_buffer(name) get_shared_region(name, dataMap, "_data")

// Copy a message from the queue 
static int read_message(tm_mailbox *mb, tm_message *buffer)
{
    if(mb->head == mb->tail)
        return 0;

    *buffer = *(tm_message*)&mb->slots[mb->head];
    mb->slots[mb->head].message.message = 0UL;
    mb->head = (mb->head+1) % MESSAGE_SLOTS;

    return 1;
}

int tm_init(unsigned long name)
{
    char cname[128];

    sglib_hashed_name_map_init(mbMap);
    sglib_hashed_name_map_init(dataMap);
    sglib_hashed_name_map_init(freeMap);


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
    mailbox = (tm_mailbox*)create_shared_mapping(cname);
    mailbox->head = 0;
    mailbox->tail = 0;

    format_name(name, cname, "_data", 128);
    dataBox = (tm_data_buffer*)create_shared_mapping(cname);
    dataBox->limit = MESSAGE_DATA_SLOTS;
    dataBox->curslot = 0;

    format_name(name, cname, "_free", 128);
    freeBox = (tm_mailbox*)create_shared_mapping(cname);

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


    munmap(mailbox, MAPPING_LEN);
    munmap(freeBox, MAPPING_LEN);
    munmap(dataBox, MAPPING_LEN);

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

tm_data *tm_alloc(void)
{
    if(dataBox->curslot < dataBox->limit)
    {
        return &dataBox->slots[dataBox->curslot++];
    }
    else
    {
        int r;
        int stallcountdown = 100;
        tm_message message;
        while((r = read_message(freeBox, &message)) == 0)
        {
            if(--stallcountdown <= 0)
            {
                if(stallcountdown == 0)
                    fprintf(stderr, "Stalling for freed data in alloc\n");
                stallcountdown = -1;
            }
        }

        if(message.dataname != my_name_entry->name)
        {
            fprintf(stderr, "FATAL: Found free for wrong name\n");
            abort();
        }
        else
        {
            return &dataBox->slots[message.slot];
        }
    }
}
