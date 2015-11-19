#!/bin/bash

spectrum2_manager start

while :
do
	sleep 2
	tail -f /var/log/spectrum2/*/* /var/log/spectrum2/*/*/*
done
