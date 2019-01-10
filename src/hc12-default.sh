#!/bin/sh

port=$1
if [ ! -e "$port" ]; then
  echo "$port does not exist"
  exit 1
fi

stty 9600 <>$port

# "Standard" configuration is 2200bps, channel 1, mode FU3,
# power 8dBm.
# Default configuration is 9600, 20dBm.

cat $port &
pid=$!

printf "AT\r\n" >$port
sleep 1
# Return all parameters
printf "AT+RX\r\n" >$port
sleep 1
# Show firmware version
printf "AT+V\r\n" >$port
sleep 1
printf "AT+DEFAULT\r\n" >$port
sleep 2
printf "AT+RX\r\n" >$port
sleep 1

kill $pid
