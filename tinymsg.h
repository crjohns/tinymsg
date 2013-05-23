#ifndef TINYMSG_H
#define TINYMSG_H

#include <stdint.h>

#ifndef CACHELINE
#define CACHELINE 64
#endif

#define NAME_ENTRIES 1024
#define NAME_PAGE_LEN (NAME_ENTRIES * sizeof(struct tm_name_entry) + sizeof(struct tm_name_header))

#define MESSAGE_SLOTS 4096

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
            uint32_t datamap;
            uint32_t slot;
        };
        uint64_t message;
    };
} tm_message;

#define MAKE_MESSAGE(datamap, slot) ((datamap << 32) | slot)

typedef struct
{
    unsigned long head __mpalign__;
    __padout__;
    unsigned long tail __mpalign__;
    __padout__;

    tm_message slots[0];

} tm_mailbox;


void tm_cleanup(void);
int tm_init(unsigned long name);


#endif
