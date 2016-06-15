#!/bin/sh

PATH=..:$PATH
plan=0;

ok(){
	printf "ok $plan $description\n";
}

not_ok(){
	printf "not ok $plan $description\n";
}

test(){
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
	printf "1..$plan\n"
	clean_up
}

dg(){
	dategrep "$@" input > stdout 2> stderr
}

stderr_empty(){ [ ! -s stderr ]; }
stdout_empty(){ [ ! -s stdout ]; }

input() { cat > input;  }
stdin() { cat > stdin;  }
stdout(){ cat > stdout; }
stderr(){ cat > stderr; }
expect(){ cat > expect; }

clean_up(){ rm -f input stdin stdout stderr expect; }

#################
test "Empty input results in empty output"

input <<EOF
EOF

dg -f 2010-05-01T00:00:00 -t 2010-05-01T00:00:01 \
&& stdout_empty \
&& stderr_empty \
&& ok \
|| not_ok

#################
test "Match all input lines"

input <<EOF
2010-05-01T00:00:00 line 1
2010-05-01T00:00:01 line 2
2010-05-01T00:00:02 line 3
EOF

expect <<EOF
2010-05-01T00:00:00 line 1
2010-05-01T00:00:01 line 2
2010-05-01T00:00:02 line 3
EOF

dg -f "2010-05-01T00:00:00" -t "2010-05-01T00:00:03" -F "%FT%T" \
&& test_output stdout expect \
&& stderr_empty \
&& ok \
|| not_ok

#################
test "Match no lines"

input <<EOF
2010-05-01T00:00:00 line 1
2010-05-01T00:00:01 line 2
2010-05-01T00:00:02 line 3
EOF

expect <<EOF
EOF

dg -f "2010-05-01T00:00:03" -F "%FT%T" \
&& test_output stdout expect \
&& stderr_empty \
&& ok \
|| not_ok

#################
test "Output single line in middle of input"

input <<EOF
2010-05-01T00:00:00 line 1
2010-05-01T00:00:01 line 2
2010-05-01T00:00:02 line 3
EOF

expect <<EOF
2010-05-01T00:00:01 line 2
EOF

dg -f "2010-05-01T00:00:01" -t "2010-05-01T00:00:02" -F "%FT%T" \
&& test_output stdout expect \
&& stderr_empty \
&& ok \
|| not_ok

#################
done_testing
