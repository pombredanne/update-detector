#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>

#define NUMBER_OF_FILES 2
const char * file[NUMBER_OF_FILES] = { "/var/lib/rpm/Packages", "/var/lib/rpm/.rpm.lock" };

static void
handle_events(int fd, int *wd)
{
    printf("Handler\n");
    char buf[4096]
        __attribute__ ((aligned(__alignof__(struct inotify_event))));
    const struct inotify_event *event;
    ssize_t len;
    char *ptr;

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
            } else if (event->mask & IN_CLOSE_NOWRITE) {
                printf("IN_CLOSE_NOWRITE: ");
            } else if (event->mask & IN_MODIFY) {
                printf("IN_MODIFY: ");
            } else if (event->mask & IN_OPEN) {
                printf("IN_OPEN: ");
            } else {
                printf("Something else!!! ");
            }

            for (int i = 0 ; i < NUMBER_OF_FILES ; i++) {
                if (wd[i] == event->wd) {
                    printf("%s", file[i]);
                }
            }

            if (event->mask & IN_ISDIR)
                printf(" [directory]\n");
            else
                printf(" [file]\n");
        }
    }
}

int
main()
{
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
                handle_events(fd, wd);
            }
        }
    }

    printf("Listening for events stopped.\n");

    close(fd);

    exit(EXIT_SUCCESS);
}