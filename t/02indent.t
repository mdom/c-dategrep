#!tapsig

depends_on indent

name "Check indentation"

cat dategrep.c | stdout_is

tap indent -kr -st dategrep.c

done_testing
