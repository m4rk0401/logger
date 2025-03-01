#!/bin/bash

# Bring up CAN1 with a bitrate of 1Mbit/s
sudo ip link set can0 up type can bitrate 1000000 
sudo ip link set can1 up type can bitrate 1000000

# Set the transmission queue length for CAN0
sudo ifconfig can0 txqueuelen 65536
sudo ifconfig can0 txqueuelen 65536

exit 0
