#!/bin/bash -e
set -e

web_dir=/var/lib/spectrum2/media

if [ -d $web_dir ]; then
  web_dir_owner="$(stat -c %u "$web_dir")"
  if [[ "$(id -u www-data)" != "$web_dir_owner" ]]; then
      usermod -u "$web_dir_owner" www-data
  fi
fi

echo "Trying to start Spectrum 2 instances."
echo "You should mount the directory with configuration files to /etc/spectrum2/transports/."
echo "Check the http://spectrum.im/documentation for more information."
spectrum2_manager start
spectrum2_manager server &
tail -f /var/log/spectrum2/*/* 2>/dev/null
