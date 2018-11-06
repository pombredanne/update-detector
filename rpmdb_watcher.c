#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <errno.h>
#include <poll.h>
#include <sys/inotify.h>
#include <unistd.h>

#include "rpmdb_watcher.h"

typedef enum {
    PACKAGES = 0,
    RPMLOCK,
} File;

typedef enum {
    START,
    LOCK,
    MODIFY,
    UNLOCK,
} State;

typedef struct event {
    uint32_t type;
    File file;
} Event;

#define NUMBER_OF_FILES 2
const char * file[NUMBER_OF_FILES] = { "/var/lib/rpm/Packages", "/var/lib/rpm/.rpm.lock" };


static Event handle_events(int fd, int *wd)
{
    printf("Handler\n");
    char buf[4096]
        __attribute__ ((aligned(__alignof__(struct inotify_event))));
    const struct inotify_event *event;
    ssize_t len;
    char *ptr;

    Event e;

    for (;;) {

        len = read(fd, buf, sizeof(buf));
        if (len == -1 && errno != EAGAIN) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        if (len <= 0)
            break;

        for (ptr = buf; ptr < buf + len;
                ptr += sizeof(struct inotify_event) + event->len) {

            event = (const struct inotify_event *) ptr;

            if (event->mask & IN_CLOSE_WRITE) {
                printf("IN_CLOSE_WRITE: ");
                e.type = IN_CLOSE;
            } else if (event->mask & IN_CLOSE_NOWRITE) {
                printf("IN_CLOSE_NOWRITE: ");
                e.type = IN_CLOSE;
            } else if (event->mask & IN_MODIFY) {
                printf("IN_MODIFY: ");
                e.type = IN_MODIFY;
            } else if (event->mask & IN_OPEN) {
                printf("IN_OPEN: ");
                e.type = IN_OPEN;
            } else {
                printf("Something else!!! ");
            }

            for (uint8_t i = 0 ; i < NUMBER_OF_FILES ; i++) {
                if (wd[i] == event->wd) {
                    printf("%s\n", file[i]);
                    e.file = i;
                }
            }

        }
    }

    return e;
}



static inline int validate_event(Event event) {
    if (event.file != PACKAGES && event.file != RPMLOCK) return 1;
    if (event.file == PACKAGES && event.type != IN_MODIFY) return 1;
    if (event.file == RPMLOCK && 
        !(event.type == IN_CLOSE || event.type == IN_OPEN)) return 1;
    return 0;
}

static State nextState(State current_state, Event event) {

    if (validate_event(event)) {
        return START;
    }

    switch(current_state) {
        case START:
            if (event.file == RPMLOCK && event.type == IN_OPEN) {
                return LOCK;
            } else {
                return START;
            }
            break;
        
        case LOCK:
            if (event.file == PACKAGES && event.type == IN_MODIFY) {
                return MODIFY;
            } else {
                return START;
            }
            break;

        case MODIFY:
            if (event.file == PACKAGES && event.type == IN_MODIFY) {
                return MODIFY;
            } else if (event.file == RPMLOCK && event.type == IN_CLOSE) {
                return UNLOCK;
            } else {
                return START;
            }
            break;

        case UNLOCK:
        default:
            return START;
            break;

    }
    return START;
}

static void* thread_worker() {
    int fd, poll_num;
    int wd[2];
    struct pollfd fds[1];

    fd = inotify_init1(IN_NONBLOCK);
    if (fd == -1) {
        perror("inotify_init1");
        exit(EXIT_FAILURE);
    }

    wd[0] = inotify_add_watch(fd, file[0], IN_MODIFY );
    wd[1] = inotify_add_watch(fd, file[1], IN_OPEN | IN_CLOSE );

    for (int i = 0 ; i < NUMBER_OF_FILES ; i++) {
        if (wd[i] == -1) {
            fprintf(stderr, "Cannot watch '%s'\n", file[0]);
            perror("inotify_add_watch");
            exit(EXIT_FAILURE);
        }
    }

    fds[0].fd = fd;
    fds[0].events = POLLIN;

    Event ev;
    State state = START;

    while (1) {
        poll_num = poll(fds, 1, -1);
        if (poll_num == -1) {
            if (errno == EINTR)
                continue;
            perror("poll");
            exit(EXIT_FAILURE);
        }

        if (poll_num > 0) {

            if (fds[0].revents & POLLIN) {
                ev = handle_events(fd, wd);
                state = nextState(state, ev);

                if (state == UNLOCK) {
                    printf("UPDATE!!\n");
                    state = START;
                }
            }
        }
    }

    printf("Listening for events stopped.\n");

    close(fd);
}

Watcher* create_rpmdb_watcher() {
    Watcher * w = malloc(sizeof(Watcher));
    if (w == NULL) {
        return NULL;
    }

    pthread_create(&w->thread, NULL, thread_worker, NULL);

    return w;
}

void release_rpmdb_watcher(Watcher * w) {
    pthread_join(w->thread, NULL);
    free(w);
}