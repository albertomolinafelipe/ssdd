#!/bin/bash

if [ $# -lt 1 ]; then
    echo "Usage: $0 [-c <input_file>] | [-s] | [-r]"
    exit 1
fi

MODE="$1"

if [ "$MODE" = "-c" ]; then
    if [ $# -ne 2 ]; then
        echo "Usage for client mode: $0 -c <input_file>"
        exit 1
    fi

    INPUT_FILE="$2"

    # Prepare container
    apt install pip -y > /dev/null 2>&1
    source .venv/bin/activate 
    pip install flask > /dev/null 2>&1

    # SERVER WILL RUN ON DOCKER 1
    IP=$(head -n 1 ../machines)

    python3 date.py > /dev/null 2>&1 &
    sleep 1
    cat "$INPUT_FILE" | python3 client.py -s "$IP" -p 6677

elif [ "$MODE" = "-s" ]; then
    make clean > /dev/null 2>&1
    make > /dev/null 2>&1

    # LOGGER WILL RUN ON DOCKER 2
    LOG_RPC_IP=$(sed -n '2p' ../machines)
    env LOG_RPC_IP="$LOG_RPC_IP" ./bin/server -p 6677

elif [ "$MODE" = "-r" ]; then
    make clean > /dev/null 2>&1
    make > /dev/null 2>&1

    ./bin/logger_server

else
    echo "Invalid mode: $MODE"
    echo "Usage: $0 [-c <input_file>] | [-s]"
    exit 1
fi
