#ifndef RPMDB_WATCHER_H
#define RPMDB_WATCHER_H

#include <stdio.h>
#include <pthread.h>

typedef struct watcher {
    pthread_t thread;
} Watcher;

Watcher* create_rpmdb_watcher();
void release_rpmdb_watcher(Watcher * watcher);

#endif