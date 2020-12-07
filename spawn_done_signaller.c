#include "exspawn.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    char *spawn_prefix;
    int jid;

    if(argc != 3) {
        fprintf(stderr, "Usage: spawn_done_signaller <prefix> <job_id>\n");
        return(-1);
    }

    spawn_prefix = argv[1];
    jid = atoi(argv[2]);

    signal_spawn_done(spawn_prefix, jid);

    return(0);
}
