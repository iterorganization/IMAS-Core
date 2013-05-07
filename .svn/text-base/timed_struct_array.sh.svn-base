#! /bin/sh
grep 'struct_array.*timed="yes"' $UAL/xml/CPODef.xml \
    | awk '$4 ~ /path\=/ { print substr($4, 6) "," }' \
    | sort -u > timed_struct_array.h
