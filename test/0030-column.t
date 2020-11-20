#!/usr/bin/env bash
. testlib
plan_tests 6

out=$(thermobench -O- -s/dev/null --column=key -- echo key=value)
okx grep -E "time/ms,key" <<<$out
okx grep -E "[0-9.],value" <<<$out

out=$(thermobench -O- -s/dev/null --column=key1 --column=key2 -- printf 'key1=value1\nkey2=value2\n')
okx grep -E "time/ms,key1,key2" <<<$out
is "$(grep -o ",value.*" <<<$out)" ",value1,value2" "values 1 and 2 on the same line"

readarray -t lines <<<"$(thermobench -O- -s/dev/null --column=key{1,2,3,4} -- printf 'key1=value1\n')"
is   "${lines[1]}" "time/ms,key1,key2,key3,key4" "header has 4 key columns"
like "${lines[2]}" "[0-9.],value1,,," "row ends with empty cells"
