#!/bin/bash
. testlib
plan_tests 4

out=$(thermobench -O- -s/dev/null -E --exec="echo value" -- true)
okx grep -E "time/ms,echo" <<<$out
okx grep -E "[0-9.]+,value" <<<$out

out=$(thermobench -O- -s/dev/null -E --exec="(header)echo value" -- true)
okx grep -E "time/ms,header" <<<$out
okx grep -E "[0-9.]+,value" <<<$out
