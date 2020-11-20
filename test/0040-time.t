#!/usr/bin/env bash
. testlib
plan_tests 1
okx timeout 2s thermobench -O- -s/dev/null --time=1 -- sleep inf
