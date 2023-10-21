#!/usr/bin/expect

set timeout 10
spawn espflash monitor {*}$argv
expect "Commands:"
sleep 0.1
send "\x12"
interact