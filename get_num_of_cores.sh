#! /bin/bash

sum=0

for i in `cat /proc/cpuinfo | grep "cpu cores" | grep -o -E '[0-9]+'`;do
	sum=$((sum + i ))
done

touch num_of_cores


echo $sum > num_of_cores


