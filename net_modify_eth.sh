#!/bin/bash
 
NIC_NAME=$1
IP_METGOD=$2
IP_ADDR=$3
IP_GATEWAT=$4
IP_DNS=$5

#CONNECTION_NAME=$(nmcli device | grep ${NIC_NAME} | grep -v '^GENERAL.SETTINGS' | awk '{print $4}')
CONNECTION_NAME=$(nmcli -t -f NAME,DEVICE connection show | grep "eth0" | awk -F: '{print $1}')
if [ -n "$CONNECTION_NAME" ];then
  echo ${CONNECTION_NAME}

  if [ ${IP_METGOD} = "true" ];then
    echo "auto connect"
    nmcli c m "${CONNECTION_NAME}" ipv4.address "" ipv4.gateway ""
    nmcli c m "${CONNECTION_NAME}" ipv4.method auto
  else
    echo "manual connect"
    nmcli c m "${CONNECTION_NAME}" ipv4.address $IP_ADDR ipv4.method manual ipv4.gateway $IP_GATEWAT ipv4.dns $IP_DNS
  fi

  if [ $? -ne 0 ];then
    exit 1
  fi

  nmcli c reload
  if [ $? != 0 ]; then
    exit 2
  fi

  nmcli con up "${CONNECTION_NAME}"
  if [ $? != 0 ]; then
    exit 3
  fi
  exit 0;
else
  echo "net Name error"
fi
exit 4;