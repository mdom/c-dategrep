#!tapsig

command indent

tap "Check indentation"

cat dategrep.c | stdout

call -kr -st dategrep.c

done_testing
