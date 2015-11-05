#!/bin/sh
sleeptime=0

filename="bus.txt"

while read filename2
do
    while read busname url
    do
#        echo $busname
#        echo $url
        d=`date +%m_%d_%H_%M_%S`
        START_TIME=`echo $(($(date +%s%N)/1000000))`
        #curl "$url" -o "data/$busname/$d" --create-dirs -# -s -m 1 --retry 1
        #wget -O "data/$busname/$d" "$url" --wait=0.1 --tries=5 --read-timeout=0.3 -q
        wget -T 0.2 --tries=1 --wait=0.1 -O "data/$busname/$d" "$url" -q
        END_TIME=`echo $(($(date +%s%N)/1000000))`
        takes=$(($END_TIME - $START_TIME))
#		echo "data/$busname/$d" >> test.txt
#        echo $busname reading takes $takes ms
        ./parser -i data/$busname/$d -d
        END_TIME2=`echo $(($(date +%s%N)/1000000))`
        takes2=$(($END_TIME2 - $END_TIME))
#        echo $busname parsing takes $takes ms
        takes3=$(($END_TIME2 - $START_TIME))
        echo $busname total takes $takes3 ms ... "($takes + $takes2)"
        total=$(($total+1))
        totaltime=$(($totaltime + $takes3))
        totaltimer=$(($totaltimer + $takes))
        totaltimep=$(($totaltimep + $takes2))
        sleep $sleeptime
        echo "====================================================="
    done <$filename2
done <$filename

echo "total takes:" $totaltime $totaltimer "+" $totaltimep
echo $total
echo "average:" $(($totaltime/$total)) $(($totaltimer/$total)) $(($totaltimep/$total))
