#!/bin/bash
# copied from "https://stackoverflow.com/questions/44331836/apt-get-install-tzdata-noninteractive"
export DEBIAN_FRONTEND=noninteractive

ln -fs /usr/share/zoneinfo/America/New_York /etc/localtime
apt install -y tzdata
dpkg-reconfigure --frontend noninteractive tzdata