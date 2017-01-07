#!/bin/bash
#author: v.mogilin, v.mogilin@corp.mail.ru

O2_PROXY=o2proxy
BIN=/home/o2gy/repo/github/misc/o2proxy/src/BUILD/o2proxy

function is_running()
{
    # returns 0, if process is not run

    gr=`pgrep -f $1`
    if [ -z "$gr" ]
    then
        return 0
    fi
    return 1
}

while true; do

    is_running $BIN
    result=$?
    if [ $result -eq "0" ]
    then
        d=`date`
        echo "[${d}] ${O2_PROXY} not running.. restart ${O2_PROXY}"
        ulimit -c unlimited
        ${BIN} -p 7788 --syslog &
    else
        :
        #echo "[debug] process (${O2_PROXY}) runnnig OK"
    fi

    sleep 3
done

exit 0
