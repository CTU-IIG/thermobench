#!/bin/bash

DIR="$( cd "$(dirname "$0")" ; pwd -P )"
CMD="$DIR/ramsmp -b "

for i in {1..6}
do
    $CMD $i
done
