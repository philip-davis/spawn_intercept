#include<errno.h>
#include<fcntl.h>
#include<inttypes.h>
#include<stdio.h>
#include<stdlib.h>
#include<mpi.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>

#define SPAWN_DONE_EXT ".done"

struct comm_job_list {
    struct comm_job_list *prev, *next;
    int jid;
    void *comm;
};

struct comm_job_list *cj_list = NULL;

int signal_spawn_done(const char *spool_prefix, int jid)
{
    struct flock lock = { .l_type = F_WRLCK,
                          .l_whence = SEEK_SET,
                          .l_start = 0,
                          .l_len = 0 };
    const char *done_ext = SPAWN_DONE_EXT;
    char *done_name;
    int done_file;

    done_name = malloc(sizeof(spool_prefix) + sizeof(done_ext) + 35);
    sprintf(done_name, "%s.%d.done", spool_prefix, jid);
    done_file = creat(done_name, S_IRUSR | S_IWUSR);
    if(done_file < 0) {
        fprintf(stderr, "ERROR: %s: unable to create done file '%s'", __func__, done_name);
        perror(NULL);
        return(0);
    }

    close(done_file);

     return 0;
}


int MPI_Comm_disconnect(MPI_Comm *comm)
{
    struct comm_job_list *node = cj_list;
    char *spool_prefix = getenv("SPAWN_SPOOL_PREFIX");
    const char *done_ext = SPAWN_DONE_EXT;
    char *done_file;
    struct stat statbuf;
    int ret;

    if(!node) {
        fprintf(stderr, "ERROR: %s: no known spawned jobs outstanding.\n", __func__);
        return 0;
    }

    while(1) {
        if(node->comm == comm) {
            break;
        }
        node = node->next;
        if(node == cj_list) {
            fprintf(stderr, "ERROR: %s: unknown communicator.\n", __func__);
            return 0;
        }
    }

    done_file = malloc(sizeof(spool_prefix) + sizeof(done_ext) + 35);
    sprintf(done_file, "%s.%d.done", spool_prefix, node->jid);

    do {
        ret = stat(done_file, &statbuf);
        if(ret)
            sleep(1);
    } while(ret);

    unlink(done_file);

    return 0;
}

void insert_comm_job(MPI_Comm *comm, int jid)
{
    struct comm_job_list *node = malloc(sizeof(*node));

    node->jid = jid;
    node->comm = comm;

    if(cj_list) {
        node->prev = cj_list->prev;
        node->next = cj_list;
        cj_list->prev->next = node;
        cj_list->prev = node;
    } else {
        node->prev = node;
        node->next = node;
        cj_list = node;
    }
}

int despool_cmd(char **command, const char *spool)
{
   struct flock lock = { .l_type = F_WRLCK,
                          .l_whence = SEEK_SET,
                          .l_start = 0,
                          .l_len = 0 };
    int file;
    int ret;
    struct stat statbuf;
    int linelen, filelen;
    char *rest;
    char c;
    int pos = 0;

    *command = NULL;

    file = open(spool, O_RDWR);
    if(file < 0) {
        if(errno != ENOENT) {
            fprintf(stderr, "could not open %s: ", spool);
            perror(NULL);
        }
        return(-errno);
    }

    if(fcntl(file, F_SETLKW, &lock) < 0) {
        fprintf(stderr, "ERROR: %s: unable to lock '%s': ", __func__, spool);
        perror(NULL);
        close(file);
        return 0;
    }

    ret = stat(spool, &statbuf);
    filelen = statbuf.st_size;
    if(filelen) {
        linelen = 0;
        do {
            read(file, &c, 1);
            linelen++;
        } while(c != '\n');
        *command = malloc(linelen);
        rest = malloc(filelen - linelen);
        lseek(file, 0, SEEK_SET);
        read(file, *command, linelen);
        (*command)[linelen-1] = '\0';
        read(file, rest, filelen - linelen);
        lseek(file, 0, SEEK_SET);
        write(file, rest, filelen - linelen);
        ftruncate(file, filelen - linelen);
        free(rest);
        fsync(file);        
    }
    lock.l_type = F_UNLCK;
    fcntl(file, F_SETLK, &lock);
    close(file);


    return 0;
}

int enspool_cmd(int jid, const char *command, char *argv[], int nprocs, const char *spool, const char *prefix)
{
    struct flock lock = { .l_type = F_WRLCK,
                          .l_whence = SEEK_SET,
                          .l_start = 0,
                          .l_len = 0 };
    int file;
    char nproc_str[33];
    char jid_str[33];
    char **arg = argv;

    file = open(spool, O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR);
    if(file < 0) {
        fprintf(stderr, "ERROR: %s: unable to open spool file '%s' for writing: ", __func__, spool);
        perror(NULL);
        return(0);
    }

    if(fcntl(file, F_SETLK, &lock) < 0) {
        fprintf(stderr, "ERROR: %s: unable to lock '%s': ", __func__, spool);
        perror(NULL);
        close(file);
        return 0;
    }

    sprintf(nproc_str, "%d", nprocs);
    sprintf(jid_str, "%d", jid);

    write(file, prefix, strlen(prefix));
    write(file, ",", 1);
    write(file, jid_str, strlen(jid_str));
    write(file, ",", 1);
    write(file, nproc_str, strlen(nproc_str));
    write(file, ",", 1);
    write(file, command, strlen(command));
    while(*arg) {
        write(file, ",", 1);
        write(file, *arg, strlen(*arg));
        arg++;
    }
    write(file, "\n", 1);

    fsync(file);

    lock.l_type = F_UNLCK;
    fcntl(file, F_SETLK, &lock);

    close(file);
    return 0;
}

int MPI_Comm_spawn(const char *command, char *argv[], int maxprocs,
            MPI_Info info, int root, MPI_Comm comm,
                MPI_Comm *intercomm, int array_of_errcodes[])
{
    struct flock lock = { .l_type = F_WRLCK,
                          .l_whence = SEEK_SET,
                          .l_start = 0,
                          .l_len = 0 };
    char *spawn_spool = getenv("SPAWN_SPOOL_FILE");
    char *spool_prefix = getenv("SPAWN_SPOOL_PREFIX");
    static int jid = 0;

    enspool_cmd(jid, command, argv, maxprocs, spawn_spool, spool_prefix);    

    insert_comm_job(intercomm, jid);

    jid++;

    return 0;
}
