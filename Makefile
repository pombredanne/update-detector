CFLAGS= -std=c11 -o2 -g -Wall -Wextra -pedantic
LDFLAGS= -lpthread

OUTPUT= main watch_database watch_database2

default: $(OUTPUT)

main: main.o rpmdb_watcher.o
main.o: main.c

rpmdb_watcher.o: rpmdb_watcher.c rpmdb_watcher.h

#watch_database: watch_database.o
#watch_database2: watch_database2.o

watch_database.o: watch_database.c
watch_database2.o: watch_database2.c

clean:
	rm -rf *.o $(OUTPUT)