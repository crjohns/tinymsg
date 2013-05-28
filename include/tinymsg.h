#ifndef TINYMSG_H
#define TINYMSG_H

#include <stdint.h>

#ifndef CACHELINE
#define CACHELINE 64
#endif

#define NAME_ENTRIES 1024
#define NAME_PAGE_LEN (NAME_ENTRIES * sizeof(struct tm_name_entry) + sizeof(struct tm_name_header))

#define MESSAGE_SLOTS 4096

#define MESSAGE_DATA_SIZE 4096
#define MESSAGE_DATA_SLOTS 1024

#define PASTE(x,y) x ## y
#define PASTE2(x,y) PASTE(x,y)
#define __mpalign__ __attribute__((aligned(CACHELINE)))
#define __padout__ char PASTE2(pad, __COUNTER__)[0] __mpalign__

typedef struct
{
    uint32_t name;
    uint32_t valid;
    __padout__;
} tm_name_entry __mpalign__;

typedef struct 
{
    uint32_t curslot;
    __padout__;
    
    tm_name_entry entries[0];
} tm_name_header  __mpalign__;

typedef struct 
{
    union
    {
        struct
        {
            uint32_t dataname;
            uint32_t slot;
        };
        uint64_t message;
    };
} tm_message;

typedef struct
{
    tm_message message;
    __padout__;
} padded_message;

#define MAKE_MESSAGE(datamap, slot) ((datamap << 32) | slot)

typedef struct
{
    unsigned long head __mpalign__;
    __padout__;
    unsigned long tail __mpalign__;
    __padout__;

    padded_message slots[0];

} tm_mailbox;

typedef struct
{
    uint32_t owner;
    uint16_t slot;
    uint16_t len;
    char data[MESSAGE_DATA_SIZE - sizeof(uint16_t) - sizeof(uint16_t) - sizeof(uint32_t)];
} tm_data;

#define MAX_DATA_LEN (MESSAGE_DATA_SIZE - sizeof(uint16_t))

typedef struct
{
    unsigned long curslot;
    unsigned long limit;
    tm_data slots[MESSAGE_DATA_SLOTS] __attribute__((aligned(4096)));
} tm_data_buffer;


void tm_cleanup(void);
int tm_init(unsigned long name);

tm_data *tm_alloc(void);
void tm_free(tm_data *);


tm_data *tm_poll(void);


#endif
