#!/bin/bash
. testlib
plan_tests 1
out=$(thermobench -O- -s/dev/null -- true)
okx grep "^time/ms" <<<$out
