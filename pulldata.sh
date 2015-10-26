#!/bin/sh
sleeptime=0.3

filename="bus.txt"
exec < $filename

while read filename2
do
    exec < $filename2
    while read busname url
    do
#        echo $busname
#        echo $url
        d=`date +%m_%d_%H_%M_%S`
        START_TIME=`echo $(($(date +%s%N)/1000000))`
        curl "$url" -o "data/$busname/$d" --create-dirs -# -s
        END_TIME=`echo $(($(date +%s%N)/1000000))`
        takes=$(($END_TIME - $START_TIME))
#        echo $busname reading takes $takes ms
        ./parser data/$busname/$d
        END_TIME2=`echo $(($(date +%s%N)/1000000))`
        takes2=$(($END_TIME2 - $END_TIME))
#        echo $busname parsing takes $takes ms
        takes3=$(($END_TIME2 - $START_TIME))
        echo $busname total takes $takes3 ms ... "($takes + $takes2)"
        sleep $sleeptime
        echo "====================================================="
    done
done


