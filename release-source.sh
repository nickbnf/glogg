#!/bin/bash

VERSION=$(git describe | sed -e "s/^v//")
echo "echo \"$VERSION\" > .tarball-version"
echo 'touch --date="$(git log -n 1 --pretty=format:%ci)" .tarball-version'
echo "tmp_tar=\"$(mktemp)\""
echo "git archive --format=tar --prefix=glogg-$VERSION/ v$VERSION >\$tmp_tar"
echo "tmp_dir=\"$(mktemp -d)\""
echo "tar xf \$tmp_tar --directory \$tmp_dir"
echo "cp -p .tarball-version \$tmp_dir/glogg-$VERSION/"
echo "tar czf glogg-$VERSION.tar.gz --directory \$tmp_dir glogg-$VERSION/"
echo "rm .tarball-version \$tmp_tar"
