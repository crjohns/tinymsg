default: all

CFLAGS ?= -g

LIBS = tinymsg.a
tinymsg.a-OBJS = tinymsg.o

PROGRAMS = inittest

HEADERS = $(wildcard *.h)

$(LIBS): $(value $(join $(LIBS), -OBJS))
	$(AR) rcs $@ $^

$(PROGRAMS): %: %.o $(LIBS)
	$(CC) -o $@ $< $(LIBS)

%.o: %.c $(HEADERS)
	$(CC) $< -c -o $@ $(CFLAGS)

all: $(LIBS) $(PROGRAMS)

clean:
	rm -f *.o $(LIBS) $(PROGRAMS)
