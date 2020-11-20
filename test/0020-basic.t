#!/usr/bin/env bash
. testlib
plan_tests 6

out=$(thermobench -O- -s/dev/null -- true)
ok $? "exit code"
okx grep "^time/ms" <<<$out

TODO="Notify about non-zero exit codes"
out=$(thermobench -O- -s/dev/null -- false)
is $? 1 "exit code"
unset TODO

out=$(thermobench -O- -s/dev/null --stdout -- echo ahoy)
ok $? "exit code"
is "$(sed -ne 2p <<<$out)" "time/ms,stdout" "header"
like "$(sed -ne 3p <<<$out)" "[0-9.]+,ahoy$" "stdout in CSV"
