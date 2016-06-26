#!tapsig

depends_on indent

tap "Check indentation"

cat dategrep.c | stdout

call indent -kr -st dategrep.c

done_testing
