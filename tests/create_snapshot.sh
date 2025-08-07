#!/bin/bash

chmod +x $1
test_name=$(basename $1 .in)
"./$1" > "$test_name.out" 2>&1
