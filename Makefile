NAME_SERVER=server
NAME_CLIENT=client

CFLAGS=-Wall -std=gnu99 -Wno-deprecated-declarations
LDFLAGS = -lcrypto
INCLUDES=-I./inc

SRCS_SERVER=src/errExit.c src/TriServer.c src/semaphore.c src/sha256_file.c  src/sha256_string.c

SRCS_CLIENT=src/errExit.c src/TriClient.c src/semaphore.c

OBJS_SERVER=$(SRCS_SERVER:.c=.o)
OBJS_CLIENT=$(SRCS_CLIENT:.c=.o)


all: $(NAME_SERVER) $(NAME_CLIENT)

$(NAME_SERVER): $(OBJS_SERVER)
	@echo "Making executable: "$@
	@$(CC) $^ -o $@ $(LDFLAGS)

$(NAME_CLIENT): $(OBJS_CLIENT)
	@echo "Making executable: "$@
	@$(CC) $^ -o $@


.c.o:
	@echo "Compiling: "$<
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

.PHONY: clean

clean:
	@rm -f src/*.o $(NAME_SERVER) $(NAME_CLIENT)
	@echo "Removed object files and executables..."
