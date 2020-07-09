#!/bin/bash

echo "Trying to start Spectrum 2 instances."
echo "You should mount the directory with configuration files to /etc/spectrum2/transports/."
echo "Check the http://spectrum.im/documentation for more information."
spectrum2_manager start
spectrum2_manager server &
tail -f /var/log/spectrum2/*/* 2>/dev/null
