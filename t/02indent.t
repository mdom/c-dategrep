#!tapsig

depends_on indent

name "Check indentation"

stdout_is < "$current_dir/dategrep.c"

tap indent -kr -st "$current_dir/dategrep.c"

done_testing
