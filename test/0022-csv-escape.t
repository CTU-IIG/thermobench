#!/bin/bash
. testlib
plan_tests 4

out=$(thermobench -O- -S'/dev/null comma,comma' -- true)
ok $? "exit code"
is "$(sed -ne 2p <<<$out)" 'time/ms,"comma,comma"' "comma escaped by quoting"

out=$(thermobench -O- -S'/dev/null quote"quote' -- true)
ok $? "exit code"
is "$(sed -ne 2p <<<$out)" 'time/ms,"quote""quote"' "quote escaped by quoting and doubling"
