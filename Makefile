CC = gcc
CFLAGS = -O3 -s -flto -pthread -march=haswell -Iinc -Iinc/lib
LDFLAGS = -ld3d9 -lavformat -lavcodec -lavutil -lswscale

srcs = $(wildcard src/*.c) $(wildcard src/lib/*.c)
objs = $(addprefix build/, $(notdir $(srcs:.c=.o)))

build/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@
	
build/%.o: src/lib/%.c
	$(CC) $(CFLAGS) -c $< -o $@

vq: $(objs)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
