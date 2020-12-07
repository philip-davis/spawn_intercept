# spawn_intercept
Intercept MPI_Comm_spawn calls to workaround cluster configuration

This code is intended for MPI code running on clusters where the compute node cannot/does not support the `MPI_Comm_spawn` operation. It exports spawned jobs to a spool file, which is read by the `spawn-watcher.sh` script, which dispatches new instances of mpirun.

This is tested with OpenMPI.

## Building
The Makefile is hard-coded to use mpicc.

## Usage
This works by intercepting `MPI_Comm_spawn` and `MPI_Comm_disconnect` calls to the MPI library. In order to enable intercepting these calls, the LD_PRELOAD (http://www.goldsborough.me/c/low-level/kernel/2016/08/29/16-48-53-the_-ld_preload-_trick/) environment variable must be exported to the program that will be calling Comm_spawn.

Additionally, the library needs a prefix to use (to differentiate multiple programs that use this trick at the same time) and a file to use as the spool file. These are both set by environment variables:

`SPAWN_SPOOL_PREFIX`: an arbitrary name that differentiates instances of programs using Comm_spawn. Avoid special characters
`SPAWN_SPOOL_FILE`: a file used to communicate spawn requests. The dispatch script and the programs must all agree on this file.

Once these environemnt variables are set, the dispatch script should be run. It takes no arguments:
`./spawn-watcher.sh &> spawn.log &`

Finally, launch the program, `LD_PRELOAD=<path/to>/libexspawn.so mpirun -x LD_PRELOAD python...`

## Caveats
This breaks most of the non-trivial features of Comm_spawn. The interconnector should not be used or changed between the spawn and the Disconnect. Any MPI_Info parameters will be discarded, etc...

This is not exstensively tested. Dollar-signs in command-line arguments are likely catastrophic, commas and spaces in command-line options might break it as well, permission issues might crop up, etc, etc.
