#ifndef TINYMSG_H
#define TINYMSG_H

#ifndef CACHELINE
#define CACHELINE 64
#endif

#define NAME_ENTRIES 1024
#define NAME_PAGE_LEN (NAME_ENTRIES * sizeof(struct tm_name_entry) + sizeof(struct tm_name_header))

#define __mpalign__ __attribute__((aligned(CACHELINE)))
#define __padout__ char pad##__LINE__[0] __mpalign__

typedef struct
{
    unsigned long name;
    unsigned long valid;
} tm_name_entry;

typedef struct 
{
    unsigned int curslot;
    __padout__;
    

    tm_name_entry entries[0];
} tm_name_header  __mpalign__;

void tm_cleanup(void);

int tm_init(unsigned long name);


#endif
