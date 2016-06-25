#!/bin/sh

PATH=..:$PATH

plan=0
rc_plan=0

ok(){
	printf "ok $plan $description\n";
}

not_ok(){
	printf "not ok $plan $description\n";
}

check_expections(){
	if [ "$plan" != 0 ];then
		test_output stderr_plan stderr &&
		test_output stdout_plan stdout &&
		[ $rc -eq $rc_plan ] &&
		ok ||
		not_ok
	fi
}

test(){
	check_expections
	clean_up
	touch stdout_plan stderr_plan
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
	rc=$?
}

input() { cat > input;  }
stdin() { cat > stdin;  }
stdout(){ cat > stdout_plan; }
stderr(){ cat > stderr_plan; }

rc() { rc_plan=$1; }

clean_up(){
	rm -f input stdin stdout stderr expect stdout_plan stderr_plan;
	rc_plan=0
}

#################
test "Empty input results in empty output"

input <<EOF
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

dg -f "2010-05-01T00:00:00" -t "2010-05-01T00:00:03" -F "%FT%T"

#################
test "Match no lines"

input <<EOF
2010-05-01T00:00:00 line 1
2010-05-01T00:00:01 line 2
2010-05-01T00:00:02 line 3
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

dg -f "2010-05-01T00:00:01" -t "2010-05-01T00:00:02" -F "%FT%T"

#################
test "Skip dateless lines"

input <<EOF
2010-05-01T00:00:00 line 1
foo
2010-05-01T00:00:01 line 2
2010-05-01T00:00:02 line 3
EOF

rc 1

stderr <<EOF
dategrep: Found line without date: foo
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

stdout <<EOF
2010-05-01T00:00:01 line 2
EOF

dg -s -f "2010-05-01T00:00:01" -t "2010-05-01T00:00:02" -F "%FT%T"

#################
test "Print multine logs"

input <<EOF
2010-05-01T00:00:00 line 1
foo
2010-05-01T00:00:01 line 2
bar
2010-05-01T00:00:02 line 3
EOF

stdout <<EOF
2010-05-01T00:00:01 line 2
bar
EOF

dg -m -f "2010-05-01T00:00:01" -t "2010-05-01T00:00:02" -F "%FT%T"

#################
done_testing
