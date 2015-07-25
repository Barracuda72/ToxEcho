OUTPUT = toxecho
OBJ = main.o
LIBS = libtoxcore

CFLAGS = -std=gnu99 -Wall
LDFLAGS = $(shell pkg-config --libs $(LIBS))

all: $(OBJ)
	@$(CC) $(CFLAGS) $(LDFLAGS) $(OBJ) -o $(OUTPUT)

%.o: %.c
	@$(CC) $(CFLAGS) -c $*.c -o $*.o

clean:
	rm *.o $(OUTPUT)
