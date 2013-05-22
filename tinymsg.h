#ifndef TINYMSG_H
#define TINYMSG_H

#ifndef CACHELINE
#define CACHELINE 64
#endif

#define __mpalign__ __attribute__((aligned(CACHELINE)))

struct tm_name_entry
{
    char name;
};

struct tm_name_header
{
    unsigned int slots;
    char padding1[0] __mpalign__;
    

    struct tm_name_entry entries[0];
};



int tm_cleanup();

void register_name();


#endif
