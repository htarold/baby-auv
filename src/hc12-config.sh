#!/bin/sh -vx

port=$1
if [ ! -e "$port" ]; then
  echo "$port does not exist"
  exit 1
fi

stty 9600 <>$port  # May need to init port using picocom.

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
# Set 2400 BPS
printf "AT+B2400\r\n" >$port
sleep 1
# Set channel 1 (433.4MHz)
printf "AT+C001\r\n" >$port
sleep 1
# FU3 mode 4-80ms latency
printf "AT+FU3\r\n" >$port
sleep 1
# P4 is 8dBm/6.3mW
printf "AT+P4\r\n" >$port
sleep 1

kill $pid
