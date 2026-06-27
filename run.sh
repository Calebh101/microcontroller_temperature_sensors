#!/bin/bash
set -e

DEVICE=/dev/cu.usbserial-0001
FQBN=esp8266:esp8266:generic
BAUD=115200

arduino-cli compile --verbose --fqbn $FQBN .
arduino-cli upload -p $DEVICE --fqbn $FQBN .
arduino-cli monitor -p $DEVICE -c baudrate=$BAUD | ts