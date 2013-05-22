default: all

LIBS = tinymsg.a
tinymsg.a-OBJS = tinymsg.o

PROGRAMS = inittest

$(LIBS): $(value $(join $(LIBS), -OBJS))
	$(AR) rcs $@ $^

$(PROGRAMS): %: %.o $(LIBS)
	$(CC) -o $@ $< $(LIBS)

%.o: %.c $(HEADERS)
	$(CC) $< -c -o $@ $(CFLAGS)

all: $(LIBS) $(PROGRAMS)

clean:
	rm -f *.o $(LIBS) $(PROGRAMS)
