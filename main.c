#include <stdio.h>
#include "rpmdb_watcher.h"

void update() {
    printf("Update callback!!!\n");
}

int main()
{
    Watcher * w = create_rpmdb_watcher();

    printf("Main body\n");

    release_rpmdb_watcher(w);

    printf("End of main\n");
    return 0;
}
