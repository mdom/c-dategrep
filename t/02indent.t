#!/bin/sh

printf "1..1\n"

indent -kr ../dategrep.c -o indented.c

if cmp -s indented.c ../dategrep.c;then
	printf "ok 1 "
else
	printf "not ok 1 "
fi

printf "Check indentation\n"

rm indented.c
