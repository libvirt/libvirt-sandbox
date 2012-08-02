#!/bin/bash
#
# Simple script to setup hundreds of containers at the same time
#
# In order to create 100 containers execute
# containers_test.sh create apache 100
# Start
# containers_test.sh start apache 100
# Stop
# containers_test.sh stop apache 100
# Delete
# containers_test.sh delete apache 100
#

create() {
    virt-sandbox-service create -C -l s0:c$2 -u httpd.service $1
}

delete() {
    virt-sandbox-service delete $1
}

start() {
    systemctl start httpd@$1.service
}

stop() {
    systemctl stop httpd@$1.service
}

command=$1
name=$2
repeat=$3
for i in $(seq 1 $repeat)
do
    eval $command $name$i $i
done
