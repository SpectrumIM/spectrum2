#!/bin/bash

set -xe

#Check functions required
hash debuild 2>/dev/null || ( echo >&2 "apt-get install  devscripts"; exit 1; )



vercount=$(git rev-list --all | wc -l)
gitrev=$(git rev-parse --short HEAD | sed 's/\(^[0-9\.]*\)-/\1~/')
version=$vercount-$gitrev

sed -i "1s/\((.*)\)/(1:$version-1)/" debian/changelog

rm -rf spectrum2_*

git clone ../../.git spectrum2-$version

tar -czf spectrum2_$version.orig.tar.gz spectrum2-$version

cp -r debian spectrum2-$version/debian

cd spectrum2-$version
DEB_BUILD_OPTIONS=nocheck debuild -i -us -uc

cd ..

rm -rf spectrum2-$version
