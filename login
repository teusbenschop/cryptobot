#!/bin/bash

BOT=$1
source bot${BOT}
if [ $? -ne 0 ]; then exit; fi
ssh $USER@$IP

# Information about how to connect to the local network from outside the NAT:
# https://superuser.com/questions/277218/ssh-access-to-office-host-behind-nat-router
