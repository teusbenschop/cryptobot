#!/bin/bash


# source botfront
# if [ $? -ne 0 ]; then exit; fi
# FRONTSSHIP=$IP
# FRONTSSHUSER=$USER
# source botback
# if [ $? -ne 0 ]; then exit; fi
# BACKSSHIP=$IP
# BACKSSHUSER=$USER


# Get the VPS running the proxy.
# It is the first argument passed to the script.
VPS=$1
echo Proxy: $VPS


# echo Get the IP addresses of macOS as the developer machine
# MACIPV4=`curl -s ipv4bot.whatismyipaddress.com`
# if [ $? -ne 0 ]; then exit; fi
# MACIPV6=`curl -s ipv6bot.whatismyipaddress.com`
# if [ $? -ne 0 ]; then exit; fi
# echo macOS IPv4: $MACIPV4
# echo macOS IPv6: $MACIPV6


# echo Get the IP addresses of the front bot
# FRONTBOTIPV4=`ssh $FRONTSSHUSER@${FRONTSSHIP} 'curl -s ipv4bot.whatismyipaddress.com'`
# if [ $? -ne 0 ]; then exit; fi
# FRONTBOTIPV6=`ssh $FRONTSSHUSER@${FRONTSSHIP} 'curl -s ipv6bot.whatismyipaddress.com'`
# if [ $? -ne 0 ]; then exit; fi
# echo front bot IPv4: $FRONTBOTIPV4
# echo front bot IPv6: $FRONTBOTIPV6


# echo Get the IP addresses of the back bot
# BACKBOTIPV4=`ssh $BACKSSHUSER@${BACKSSHIP} 'curl -s ipv4bot.whatismyipaddress.com'`
# if [ $? -ne 0 ]; then exit; fi
# BACKBOTIPV6=`ssh $BACKSSHUSER@${BACKSSHIP} 'curl -s ipv6bot.whatismyipaddress.com'`
# if [ $? -ne 0 ]; then exit; fi
# echo back bot IPv4: $BACKBOTIPV4
# echo back bot IPv6: $BACKBOTIPV6


echo Clean the VPS and install the software
# ssh root@$VPS "apt update"
# if [ $? -ne 0 ]; then exit; fi
ssh root@$VPS "apt --yes --assume-yes purge apache* postfix unattended-upgrades ufw"
if [ $? -ne 0 ]; then exit; fi
# ssh root@$VPS "apt --yes --assume-yes install tinyproxy"
# if [ $? -ne 0 ]; then exit; fi
ssh root@$VPS "apt --yes --assume-yes install nano fail2ban"
if [ $? -ne 0 ]; then exit; fi
ssh root@$VPS "apt --yes --assume-yes install libmysqlclient-dev libcurl4-openssl-dev libssl-dev libsqlite3-dev"
if [ $? -ne 0 ]; then exit; fi


# echo Generate the proxy configuration file
# This is taken from a default configuration file, with required modifications applied.
# The bots used to have timeout and connection issues with the proxies.
# It was discovered that after increasing the maximum allowed simultaneous connections
# the connection issues improved greatly.
LOCALCONF=/tmp/tinyproxy.conf
rm -f $LOCALCONF
echo 'User nobody' >> $LOCALCONF
echo 'Group nogroup' >> $LOCALCONF
echo 'Port 8888' >> $LOCALCONF
echo 'Listen ::' >> $LOCALCONF
echo 'BindSame yes' >> $LOCALCONF
echo 'Timeout 600' >> $LOCALCONF
echo 'DefaultErrorFile "/usr/share/tinyproxy/default.html"' >> $LOCALCONF
echo 'StatFile "/usr/share/tinyproxy/stats.html"' >> $LOCALCONF
echo 'Logfile "/var/log/tinyproxy/tinyproxy.log"' >> $LOCALCONF
echo 'LogLevel Info' >> $LOCALCONF
echo 'PidFile "/var/run/tinyproxy/tinyproxy.pid"' >> $LOCALCONF
echo 'MaxClients 400' >> $LOCALCONF
echo 'MinSpareServers 20' >> $LOCALCONF
echo 'MaxSpareServers 60' >> $LOCALCONF
echo 'StartServers 40' >> $LOCALCONF
echo 'MaxRequestsPerChild 0' >> $LOCALCONF
echo 'ViaProxyName "tinyproxy"' >> $LOCALCONF
echo 'ConnectPort 443' >> $LOCALCONF
echo 'ConnectPort 563' >> $LOCALCONF
echo 'DisableViaHeader Yes' >> $LOCALCONF
echo 'Filter "/etc/whitelist"' >> $LOCALCONF
echo 'FilterDefaultDeny Yes' >> $LOCALCONF


# echo Copy the proxy configuration into place
# scp $LOCALCONF root@[$VPS]:/etc/tinyproxy.conf
# if [ $? -ne 0 ]; then exit; fi


echo Generate the proxy whitelist
WHITELIST=/tmp/whitelist
rm -f $WHITELIST
echo 'website.nl' >> $WHITELIST
echo 'ipv4bot.whatismyipaddress.com' >> $WHITELIST
echo 'ipv6bot.whatismyipaddress.com' >> $WHITELIST
echo 'api.bitfinex.com' >> $WHITELIST
echo 'bittrex.com' >> $WHITELIST
echo 'api.bl3p.eu' >> $WHITELIST
echo 'www.cryptopia.co.nz' >> $WHITELIST
echo 'api.kraken.com' >> $WHITELIST
echo 'poloniex.com' >> $WHITELIST
echo 'tradesatoshi.com' >> $WHITELIST
echo 'yobit.net' >> $WHITELIST


# echo Copy the proxy whitelist into place
# scp $WHITELIST root@[$VPS]:/etc/whitelist
# if [ $? -ne 0 ]; then exit; fi


# echo Let the proxy restart
# Only doing a reload is not enough, it won't listen on IPv6 then.
# Use
# $ netstat --listen
# to check it.
# ssh root@$VPS "/etc/init.d/tinyproxy restart"
# if [ $? -ne 0 ]; then exit; fi


# echo Enable the firewall
# ssh root@$VPS "ufw allow ssh"
# if [ $? -ne 0 ]; then exit; fi
# ssh root@$VPS "echo 'y' | ufw enable"
# if [ $? -ne 0 ]; then exit; fi
# ssh root@$VPS "ufw allow from $MACIPV4"
# if [ $? -ne 0 ]; then exit; fi
# ssh root@$VPS "ufw allow from $MACIPV6"
# if [ $? -ne 0 ]; then exit; fi
# ssh root@$VPS "ufw allow from $FRONTBOTIPV4"
# if [ $? -ne 0 ]; then exit; fi
# ssh root@$VPS "ufw allow from $FRONTBOTIPV6"
# if [ $? -ne 0 ]; then exit; fi
# ssh root@$VPS "ufw allow from $BACKBOTIPV4"
# if [ $? -ne 0 ]; then exit; fi
# ssh root@$VPS "ufw allow from $BACKBOTIPV6"
# if [ $? -ne 0 ]; then exit; fi


# echo Getting the IPv4 and IPv6 addresses of proxy $VPS
# VPSIPV4=`ssh root@$VPS "curl -s ipv4bot.whatismyipaddress.com"`
# if [ $? -ne 0 ]; then exit; fi
# VPSIPV6=`ssh root@$VPS "curl -s ipv6bot.whatismyipaddress.com"`
# if [ $? -ne 0 ]; then exit; fi


# echo Test whether the proxy works from this IP address
# echo "curl -x http://$VPSIPV4:8888 https://ipv4bot.whatismyipaddress.com"
# curl -x http://$VPSIPV4:8888 https://ipv4bot.whatismyipaddress.com
# echo ''
# echo "curl -x http://[$VPSIPV6]:8888 https://ipv6bot.whatismyipaddress.com"
# curl -x http://[$VPSIPV6]:8888 https://ipv6bot.whatismyipaddress.com
# echo ''


# echo Test whether the proxy works from the front bot
# echo "curl -x http://$VPSIPV4:8888 https://ipv4bot.whatismyipaddress.com"
# ssh $FRONTSSHUSER@${FRONTSSHIP} "curl -sS -x http://$VPSIPV4:8888 https://ipv4bot.whatismyipaddress.com"
# echo ''
# echo "curl -x http://[$VPSIPV6]:8888 https://ipv6bot.whatismyipaddress.com"
# ssh $FRONTSSHUSER@${FRONTSSHIP} "curl -sS -x http://[$VPSIPV6]:8888 https://ipv6bot.whatismyipaddress.com"
# echo ''


# echo Test whether the proxy works from the back bot
# echo "curl -x http://$VPSIPV4:8888 https://ipv4bot.whatismyipaddress.com"
# ssh $BACKSSHUSER@${BACKSSHIP} "curl -sS -x http://$VPSIPV4:8888 https://ipv4bot.whatismyipaddress.com"
# echo ''
# echo "curl -x http://[$VPSIPV6]:8888 https://ipv6bot.whatismyipaddress.com"
# ssh $BACKSSHUSER@${BACKSSHIP} "curl -sS -x http://[$VPSIPV6]:8888 https://ipv6bot.whatismyipaddress.com"
# echo ''


# Crontab entry:
# */15 * * * * sleep 50; ./satellite.sh
# The reason for the delay of 50 seconds is this:
# Right at the start of the minute, the bot needs the market summaries and order books.
# So if the satellite is restarted right then,
# this leads to failures to find the satellite live,
# which results in one full minute of getting data without a satellite,
# which again leads to API's refusing to provide data due to rate limiting.
echo Installing cron job
ssh root@$VPS "echo '*/15 * * * * sleep 50; ./satellite.sh' | crontab"
if [ $? -ne 0 ]; then exit; fi


# echo Proxy IP addresses:
# echo $VPSIPV4
# echo $VPSIPV6


echo Done
