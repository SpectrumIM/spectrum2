#!/bin/sh
DIRNAME=spectrum2

echo "Cleaning up old sources ..."
rm -rf spectrum2-*

echo "Checking out a fresh copy ..."
rm -rf $DIRNAME
git clone ../../.git $DIRNAME
rm -rf $DIRNAME/.git

echo "Creating tarball ..."
tar czf $DIRNAME.tar.gz $DIRNAME

echo "Building package"
rpmbuild -ta $DIRNAME.tar.gz
