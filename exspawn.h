#ifndef EXSPAWN_H
#define EXSPAWN_H

int signal_spawn_done(const char *spool_prefix, int jid);

int despool_cmd(char **command, const char *spool);

int enspool_cmd(int jid, const char *command, char *argv[], int nprocs, const char *spool, const char *prefix);

#endif
