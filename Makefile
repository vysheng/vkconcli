CFLAGS=-O2 -Wall -Wextra -c
LDFLAGS=-l:libcurl.so -lm -l:libjansson.so

all: bin/vkconcli

bin/vkconcli: obj/main.o obj/md5.o obj/structures.o
	cc ${LDFLAGS} obj/main.o obj/md5.o obj/structures.o -o bin/vkconcli

obj/main.o: src/main.c src/structures.h
	cc ${CFLAGS} src/main.c -o obj/main.o

obj/md5.o: src/md5.c
	cc ${CFLAGS} src/md5.c -o obj/md5.o

obj/structures.o: src/structures.c src/structures.h
	cc ${CFLAGS} src/structures.c -o obj/structures.o

clean:
	rm obj/* bin/*
