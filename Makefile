default: all

CFLAGS ?= -g

LIBS = tinymsg.a
tinymsg.a-OBJS = tinymsg.o

TESTS = $(subst .c, , $(wildcard tests/*.c))

HEADERS = $(wildcard includes/*.h)

$(LIBS): $(value $(join $(LIBS), -OBJS))
	$(AR) rcs $@ $^

$(TESTS): %: %.o $(LIBS)
	$(CC) -o $@ $< $(LIBS)

$(PROGRAMS): %: %.o $(LIBS)
	$(CC) -o $@ $< $(LIBS)

%.o: %.c $(HEADERS)
	$(CC) $< -c -o $@ $(CFLAGS) -I$(shell pwd)/include

all: $(LIBS) $(PROGRAMS) $(TESTS)

test: $(TESTS)
	@for program in $(TESTS); do echo "--- Running $$program ---"; sudo $$program; if [ $$? != 0 ]; then echo "--- FAILED ---"; else echo "--- SUCCESS ---"; fi; done

clean:
	rm -f *.o $(LIBS) $(PROGRAMS) $(TESTS) $(wildcard tests/*.o)
