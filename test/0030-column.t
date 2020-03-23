#!/bin/bash
. testlib
plan_tests 4

out=$(thermobench -O- -s/dev/null --column=key -- echo key=value)
okx grep -E "time/ms,key" <<<$out
okx grep -E "[0-9.],value" <<<$out

out=$(thermobench -O- -s/dev/null --column=key1 --column=key2 -- printf 'key1=value1\nkey2=value2\n')
okx grep -E "time/ms,key1,key2" <<<$out
TODO="fix this"
is "$(grep -o ",value.*" <<<$out)" ",value1,value2"
unset TODO
