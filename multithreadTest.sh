#!/bin/bash

NUM_CLIENTS=3

echo "starting test with $NUM_CLIENTS clients..."

for ((i=1; i<=NUM_CLIENTS; i++))
do
    echo "launching client $i"
    ./client &
done

wait

echo "all clients done!!!"