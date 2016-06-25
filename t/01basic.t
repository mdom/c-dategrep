#!/bin/sh

PATH=..:$PATH
plan=0;

ok(){
	printf "ok $plan $description\n";
}

not_ok(){
	printf "not ok $plan $description\n";
}

check_expections(){
	test_output stderr_plan stderr &&
	test_output stdout_plan stdout &&
	ok ||
	not_ok
}

test(){
	if [ "$plan" != 0 ];then
		check_expections
	fi
	clean_up
	plan=$(( plan + 1 ))
	description=$1
}

test_output(){
	if cmp -s "$1" "$2";then
		return 0
	else
		diff -u "$1" "$2" | sed 's/^/#  /'
		return 1
	fi
}

done_testing(){
	check_expections
	clean_up
	printf "1..$plan\n"
	exit 0
}

dg(){
	dategrep "$@" input > stdout 2> stderr
}

input() { cat > input;  }
stdin() { cat > stdin;  }
stdout(){ cat > stdout_plan; }
stderr(){ cat > stderr_plan; }

clean_up(){ rm -f input stdin stdout stderr expect stdout_plan stderr_plan; }

#################
test "Empty input results in empty output"

input <<EOF
EOF

stdout <<EOF
EOF

stderr <<EOF
EOF

dg -f 2010-05-01T00:00:00 -t 2010-05-01T00:00:01

#################
test "Match all input lines"

input <<EOF
2010-05-01T00:00:00 line 1
2010-05-01T00:00:01 line 2
2010-05-01T00:00:02 line 3
EOF

stdout <<EOF
2010-05-01T00:00:00 line 1
2010-05-01T00:00:01 line 2
2010-05-01T00:00:02 line 3
EOF

stderr <<EOF
EOF

dg -f "2010-05-01T00:00:00" -t "2010-05-01T00:00:03" -F "%FT%T"

#################
test "Match no lines"

input <<EOF
2010-05-01T00:00:00 line 1
2010-05-01T00:00:01 line 2
2010-05-01T00:00:02 line 3
EOF

stdout <<EOF
EOF

stderr <<EOF
EOF

dg -f "2010-05-01T00:00:03" -F "%FT%T"

#################
test "Output single line in middle of input"

input <<EOF
2010-05-01T00:00:00 line 1
2010-05-01T00:00:01 line 2
2010-05-01T00:00:02 line 3
EOF

stdout <<EOF
2010-05-01T00:00:01 line 2
EOF

stderr <<EOF
EOF

dg -f "2010-05-01T00:00:01" -t "2010-05-01T00:00:02" -F "%FT%T"

#################
test "Skip dateless lines"

input <<EOF
2010-05-01T00:00:00 line 1
foo
2010-05-01T00:00:01 line 2
2010-05-01T00:00:02 line 3
EOF

stderr <<EOF
dategrep: Found line without date: foo
EOF

stdout <<EOF
EOF

dg -f "2010-05-01T00:00:01" -t "2010-05-01T00:00:02" -F "%FT%T"

#################
test "Skip dateless lines"

input <<EOF
2010-05-01T00:00:00 line 1
foo
2010-05-01T00:00:01 line 2
2010-05-01T00:00:02 line 3
EOF

stderr <<EOF
EOF

stdout <<EOF
2010-05-01T00:00:01 line 2
EOF

dg -s -f "2010-05-01T00:00:01" -t "2010-05-01T00:00:02" -F "%FT%T"

#################
done_testing
