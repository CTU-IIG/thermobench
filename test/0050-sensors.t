#!/bin/bash
. testlib
plan_tests 6

out=$(thermobench -O- -S"/proc/version" -- true)
ok $? "exit code"
is "$(sed -ne 2p <<<$out)" "time/ms,proc" "header line"

out=$(thermobench -O- -S"/proc/version version" -- true)
ok $? "exit code"
is "$(sed -ne 2p <<<$out)" "time/ms,version" "header line"

if test -d /sys/class/thermal/thermal_zone0; then
    out=$(thermobench -O- -S"/sys/class/thermal/thermal_zone0/temp" -- true)
    ok $? "exit code"
    is "$(sed -ne 2p <<<$out)" "time/ms,$(cat /sys/class/thermal/thermal_zone0/type)" "header line"
else
    skip 0 "thermal_zone0 missing" 2
fi
