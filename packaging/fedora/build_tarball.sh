#!/bin/sh
set -eu

DIRNAME=spectrum2
REPODIR=$(readlink -f $(dirname $0)/../../.git)

echo "Cleaning up old sources ..."
rm -rf spectrum2-*

echo "Checking out a fresh copy ..."
rm -rf $DIRNAME
git clone $REPODIR $DIRNAME && rm -rf $DIRNAME/.git

echo "Creating tarball ..."
tar czf $DIRNAME.tar.gz $DIRNAME && rm -rf $DIRNAME
