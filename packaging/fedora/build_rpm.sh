#!/bin/sh
set -eu

sh $(readlink -f $(dirname $0))/build_tarball.sh

echo "Building package"
rpmbuild -ta spectrum2.tar.gz
