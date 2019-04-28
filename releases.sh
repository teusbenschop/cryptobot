#!/bin/bash

./release front &

./release back &

# ./release proxy01 &

# ./release proxy02 &

# ./release proxy03 &

# ./release proxy04 &

# ./release proxy05 &

wait
echo

source botfront
if [ $? -ne 0 ]; then exit; fi
echo Start satellite at ${IP}
ssh $USER@${IP} 'cryptobot/satellite' &
if [ $? -ne 0 ]; then exit; fi
ssh $USER@${IP} "crontab -l"

source botproxy01
if [ $? -ne 0 ]; then exit; fi
# echo Start satellite at ${IP}
# ssh $USER@${IP} 'cryptobot/satellite' &
if [ $? -ne 0 ]; then exit; fi
# ssh $USER@${IP} "crontab -l"

source botproxy02
if [ $? -ne 0 ]; then exit; fi
# echo Start satellite at ${IP}
# ssh $USER@${IP} 'cryptobot/satellite' &
if [ $? -ne 0 ]; then exit; fi
# ssh $USER@${IP} "crontab -l"

source botproxy03
if [ $? -ne 0 ]; then exit; fi
# echo Start satellite at ${IP}
# ssh $USER@${IP} 'cryptobot/satellite' &
if [ $? -ne 0 ]; then exit; fi
# ssh $USER@${IP} "crontab -l"

source botproxy04
if [ $? -ne 0 ]; then exit; fi
# echo Start satellite at ${IP}
# ssh $USER@${IP} 'cryptobot/satellite' &
if [ $? -ne 0 ]; then exit; fi
# ssh $USER@${IP} "crontab -l"

source botproxy05
if [ $? -ne 0 ]; then exit; fi
# echo Start satellite at ${IP}
# ssh $USER@${IP} 'cryptobot/satellite' &
if [ $? -ne 0 ]; then exit; fi
# ssh $USER@${IP} "crontab -l"

echo

