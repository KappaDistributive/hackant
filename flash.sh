#!/usr/bin/env bash
echo "Flashing ${1} onto Pico"
sudo picotool load -f $1
