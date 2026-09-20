#!/bin/bash

#change for number of clients to test with
NUM_CLIENTS=20

echo "testing with $NUM_CLIENTS clients"

for ((i=1; i<=NUM_CLIENTS; i++)); do
    echo "Launching client $i"
    ./client &
done

wait

echo "all clients finished."