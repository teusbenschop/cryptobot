#!/bin/bash

SOURCE=`dirname $0`
cd $SOURCE
if [ $? -ne 0 ]; then exit; fi


# Get the VPS IP address.
# It is the first argument passed to the script.
VPS=$1
echo Setting firewall for VPS $VPS


source botfront
if [ $? -ne 0 ]; then exit; fi
FRONTSSHIP=$IP
FRONTSSHUSER=$USER
source botback
if [ $? -ne 0 ]; then exit; fi
BACKSSHIP=$IP
BACKSSHUSER=$USER


echo Get the IP addresses of macOS as the developer machine
MACIPV4=`curl -s ipv4bot.whatismyipaddress.com`
if [ $? -ne 0 ]; then exit; fi
MACIPV6=`curl -s ipv6bot.whatismyipaddress.com`
if [ $? -ne 0 ]; then exit; fi
echo macOS IPv4: $MACIPV4
echo macOS IPv6: $MACIPV6


echo Get the IP addresses of the front bot
FRONTBOTIPV4=`ssh $FRONTSSHUSER@${FRONTSSHIP} 'curl -s ipv4bot.whatismyipaddress.com'`
if [ $? -ne 0 ]; then exit; fi
FRONTBOTIPV6=`ssh $FRONTSSHUSER@${FRONTSSHIP} 'curl -s ipv6bot.whatismyipaddress.com'`
if [ $? -ne 0 ]; then exit; fi
echo front bot IPv4: $FRONTBOTIPV4
echo front bot IPv6: $FRONTBOTIPV6


echo Get the IP addresses of the back bot
BACKBOTIPV4=`ssh $BACKSSHUSER@${BACKSSHIP} 'curl -s ipv4bot.whatismyipaddress.com'`
if [ $? -ne 0 ]; then exit; fi
BACKBOTIPV6=`ssh $BACKSSHUSER@${BACKSSHIP} 'curl -s ipv6bot.whatismyipaddress.com'`
if [ $? -ne 0 ]; then exit; fi
echo back bot IPv4: $BACKBOTIPV4
echo back bot IPv6: $BACKBOTIPV6



echo Allow SSH through the firewall
ssh root@$VPS "ufw allow ssh"
if [ $? -ne 0 ]; then exit; fi
echo Enable the firewall
ssh root@$VPS "echo 'y' | ufw enable"
if [ $? -ne 0 ]; then exit; fi
echo Allow macOS through the firewall
ssh root@$VPS "ufw allow from $MACIPV4"
if [ $? -ne 0 ]; then exit; fi
ssh root@$VPS "ufw allow from $MACIPV6"
if [ $? -ne 0 ]; then exit; fi
echo Allow front bot through the firewall
ssh root@$VPS "ufw allow from $FRONTBOTIPV4"
if [ $? -ne 0 ]; then exit; fi
ssh root@$VPS "ufw allow from $FRONTBOTIPV6"
if [ $? -ne 0 ]; then exit; fi
echo Allow back bot through the firewall
ssh root@$VPS "ufw allow from $BACKBOTIPV4"
if [ $? -ne 0 ]; then exit; fi
ssh root@$VPS "ufw allow from $BACKBOTIPV6"
if [ $? -ne 0 ]; then exit; fi
echo Reload the firewall
ssh root@$VPS "ufw reload"
if [ $? -ne 0 ]; then exit; fi
# Firewall status
ssh root@$VPS "ufw status"
if [ $? -ne 0 ]; then exit; fi


echo Done
