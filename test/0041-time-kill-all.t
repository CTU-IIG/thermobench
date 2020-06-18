#!/bin/bash
. testlib
plan_tests 1

# Test that thermobench kills also grand children processes (here
# sleep inf) and not only the direct child. Not killing grand children
# results in thermobench waiting indefinitely for closing the
# benchmark stdout pipe.
okx timeout 2s thermobench -O- -s/dev/null --time=1 -- sh -c 'sleep inf'
