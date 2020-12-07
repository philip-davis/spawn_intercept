#include "exspawn.h"
#include <stdio.h>
#include <stdlib.h>

int main()
{
    char *spool_file = getenv("SPAWN_SPOOL_FILE");
    char *command;

    despool_cmd(&command, spool_file);

    if(command) {
        fprintf(stdout, "%s", command);
    } else {
        fprintf(stdout, "none");
    }

    return(0);
}
