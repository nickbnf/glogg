#!/bin/sh

./tests/logcrawler_tests $*
result=$?

ICON_OK=gtk-apply
ICON_ERROR=gtk-cancel

if [ `which notify-send` ]; then
    if [ "$result" != "0" ]; then
        notify-send -i $ICON_ERROR "glogg" "Tests failed!"
    else
        notify-send -i $ICON_OK "glogg" "Tests passed!"
    fi
fi

exit $result
