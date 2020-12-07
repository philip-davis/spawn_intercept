#!/usr/bin/bash

while [ true ] ; do
    cmd=$(./despooler)
    if [[ "$cmd" != "none" ]] ; then
        prefix=$(echo $cmd | awk 'BEGIN{FS=","} {print $1}')
        jid=$(echo $cmd | awk 'BEGIN{FS=","} {print $2}')
        spawn_line=$(echo $cmd | awk 'BEGIN{FS=","} 
            { printf "mpirun -np "$3" -nolocal -x LD_PRELOAD -x LD_LIBRARY_PATH -x SPAWN_SPOOL_FILE -x SPAWN_SPOOL_PREFIX --mca pml ucx "$4 ;
                $1="" ; $2="" ; $3="" ; $4="" ; print}')
        echo "Saw job $jid from $prefix. Spawning:"
        echo "  $spawn_line"  
        eval $spawn_line
        ./spawn_done_signaller $prefix $jid
    fi
    sleep 1
done
