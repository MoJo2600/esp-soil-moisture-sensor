#!/bin/bash
#---------------------------------------------------------------------
# Script for updating HomieNode firmware via OTA
#
# Must be executed from the project scripts folder as:
#	$ ./otaupdate.sh MQTT_BROKER_IP DEVICE_NAME
#
#   ./otaupdate.sh 192.168.178.10 soil01
#---------------------------------------------------------------------
set -e

#------------------------------------------------------------------------------
# main line starts here

E_SUCCESS=0         # success
E_NOARGS=65         # no arguments
E_BADPATH=66        # not running from <root>/scripts

enhanced="\e[7m"
reset="\e[0m"

MQTT_HOST=$1
DEVICENAME=$2

unameOut="$(uname -s)"
case "${unameOut}" in
    Linux*)     machine=Linux;;
    Darwin*)    machine=Mac;;
    CYGWIN*)    machine=Cygwin;;
    MINGW*)     machine=MinGw;;
    *)          machine="UNKNOWN:${unameOut}"
esac

#get current directory and check we are running from <root>/scripts.
#For this I just check that "src" folder exists
scripts_path="${PWD}"
root_path=$(dirname "${PWD}")
if [[ ! -e "${root_path}/src" ]]; then
    echo "Error: not running from <root>/scripts"
    exit $E_BADPATH
fi

#path for locating the new firmware
bin_path="${root_path}/.pio/build/soilmoisture"

echo -e "${enhanced}Starting OTA firmware update ${reset}"
echo -e "build path: ${bin_path}"

binfile="${bin_path}/firmware.bin"

echo -e "binfile: ${binfile}"

if [ "$machine" == "Mac" ]; then
  md5sum=`md5 -q $binfile | awk '{ print $1 }'`
  base64enc=`base64 $binfile`
else
  md5sum=`md5sum $binfile | awk '{ print $1 }'`
  base64enc=`base64 -w0 $binfile` 
fi

echo -e "md5sum: ${md5sum}"
#publish MD5 checksum
mosquitto_pub -d -h $MQTT_HOST -p 1883 -t "homie/$DEVICENAME/\$implementation/ota/checksum" -m "$md5sum" --retain

#send new firmware
mosquitto_pub -d -h $MQTT_HOST -p 1883 -t "homie/$DEVICENAME/\$implementation/ota/firmware"  -l <<< "$base64enc" --retain 
