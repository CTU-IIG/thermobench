#!/bin/bash
. testlib
plan_tests 17

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

out=$(thermobench -O- -s/dev/null -E --exec="(key=) echo key=val" -- true)
ok $? "exit code"
readarray -t line <<<$out
is "${line[1]}" "time/ms,key"
like "${line[2]}" "[0-9.]+,val$"

out=$(thermobench -O- -s/dev/null -E --exec='(key=) printf "other\nkey=val\n"' -- true)
ok $? "exit code"
readarray -t line <<<$out
is "${line[1]}" "time/ms,key"
like "${line[2]}" "[0-9.]+,val$"

out=$(thermobench -O- -s/dev/null -E --exec='(other,key=) printf "other\nkey=val\n"' -- true)
ok $? "exit code"
readarray -t line <<<$out
is "${line[1]}" "time/ms,key,other"
like "${line[2]}" "[0-9.]+,val,other$"

out=$(thermobench -O- -s/dev/null -E --exec='(other,key1=,key2=) printf "other\nkey2=val2\nkey1=val1\n"' -- true)
ok $? "exit code"
readarray -t line <<<$out
is "${line[1]}" "time/ms,key1,key2,other"
like "${line[2]}" "[0-9.]+,val1,val2,other$"
