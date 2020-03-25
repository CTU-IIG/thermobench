#!/bin/bash
. testlib
plan_tests 5

out=$(thermobench -O- -s/dev/null -E --exec="echo value" -- true)
okx grep -E "time/ms,echo" <<<$out
okx grep -E "[0-9.]+,value" <<<$out

out=$(thermobench -O- -s/dev/null -E --exec="(header)echo value" -- true)
okx grep -E "time/ms,header" <<<$out
okx grep -E "[0-9.]+,value" <<<$out

# Since "sleep inf" is spawned by a shell, we check that thermobench
# kills not only the shell but its children too.
timeout 1s thermobench -O/dev/null -s/dev/null --exec="sleep inf" -- true
is $? 0 "--exec process is properly terminated when benchmark exists"
