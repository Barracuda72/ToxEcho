OUTPUT = toxecho
OBJ = main.o
LIBS = toxcore

CFLAGS = -std=c11 -Wall -Wextra -pedantic -O2
LDFLAGS = $(shell pkg-config --libs $(LIBS))

all: $(OBJ)
	@$(CC) $(CFLAGS) $(LDFLAGS) $(OBJ) -o $(OUTPUT)

%.o: %.c
	@$(CC) $(CFLAGS) -c $*.c -o $*.o

clean:
	rm *.o $(OUTPUT)
