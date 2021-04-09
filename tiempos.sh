#!/bin/bash

mpicc main.c -o perfecto

if [ $# -eq 2 ] 
then
	nproc=$1
	num=$2

elif [ $# -eq 1 ]
then
	nproc=$1
	num=335503367
	
else
	nproc=64
	num=335503367
fi

i=1
while [ $i -le $nproc ]
do
	for j in 1
	do
		mpirun --oversubscribe -np $(expr $i \+ 1) perfecto $num
	done
	
	i=$(expr $i \* 2)
done
