#!/bin/bash

gcc -o 4-6 4-6.c -pthread

echo "size,operations,sequential,concurrent,speedup" > results.csv

sizes=(1000 5000 10000 50000 100000)
operations=(10000 50000 100000 500000 1000000)

for i in "${!sizes[@]}"; do
    size=${sizes[$i]}
    ops=${operations[$i]}
    
    sed -i "s/int n = [0-9]*;/int n = $size;/" 4-6.c
    sed -i "s/int m = [0-9]*;/int m = $ops;/" 4-6.c
    
    gcc -o 4-6 4-6.c -pthread
    
    output=$(./4-6 4 1)
    sequential=$(echo "$output" | grep "Tiempo versión secuencial" | awk '{print $4}')
    concurrent=$(echo "$output" | grep "Tiempo versión concurrente" | awk '{print $6}')
    speedup=$(echo "$output" | grep "Speedup:" | awk '{print $2}')
    
    echo "$size,$ops,$sequential,$concurrent,$speedup" >> results.csv
done