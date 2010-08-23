#!/bin/bash

VERSION=$(git describe | sed -e "s/^v//")
echo "echo \"$VERSION\" > .tarball-version"
echo 'touch --date="$(git log -n 1 --pretty=format:%ci)" .tarball-version'
echo "git archive --format=tar --prefix=glogg-$VERSION/ v$VERSION >glogg-$VERSION.tmp"
echo "tar --append -f glogg-$VERSION.tmp --transform s_^_glogg-$VERSION/_ .tarball-version"
echo "gzip < glogg-$VERSION.tmp > glogg-$VERSION.tar.gz"
echo "rm .tarball-version glogg-$VERSION.tmp"

