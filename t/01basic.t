#!tapsig

#################
tap "Empty input results in empty output"

input <<EOF
EOF

call dategrep -f 2010-05-01T00:00:00 -t 2010-05-01T00:00:01

#################
tap "Match all input lines"

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

call dategrep -f "2010-05-01T00:00:00" -t "2010-05-01T00:00:03" -F "%FT%T"

#################
tap "Match no lines"

input <<EOF
2010-05-01T00:00:00 line 1
2010-05-01T00:00:01 line 2
2010-05-01T00:00:02 line 3
EOF

call dategrep -f "2010-05-01T00:00:03" -F "%FT%T"

#################
tap "Output single line in middle of input"

input <<EOF
2010-05-01T00:00:00 line 1
2010-05-01T00:00:01 line 2
2010-05-01T00:00:02 line 3
EOF

stdout <<EOF
2010-05-01T00:00:01 line 2
EOF

call dategrep -f "2010-05-01T00:00:01" -t "2010-05-01T00:00:02" -F "%FT%T"

#################
tap "Skip dateless lines"

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

call dategrep -f "2010-05-01T00:00:01" -t "2010-05-01T00:00:02" -F "%FT%T"

#################
tap "Skip dateless lines"

input <<EOF
2010-05-01T00:00:00 line 1
foo
2010-05-01T00:00:01 line 2
2010-05-01T00:00:02 line 3
EOF

stdout <<EOF
2010-05-01T00:00:01 line 2
EOF

call dategrep -s -f "2010-05-01T00:00:01" -t "2010-05-01T00:00:02" -F "%FT%T"

#################
tap "Print multine logs"

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

call dategrep -f "2010-05-01T00:00:01" -t "2010-05-01T00:00:02"  -F "%FT%T" -m

#################
tap "Error without format"

input <<EOF
2010-05-01T00:00:01 line 2
EOF

stderr <<EOF
dategrep: Found line without date: 2010-05-01T00:00:01 line 2
EOF

rc 1

call dategrep -f "2010-05-01T00:00:01" -t "2010-05-01T00:00:02"

#################
tap "Getting format from environment"

input <<EOF
2010-05-01T00:00:01 line 2
EOF

stdout <<EOF
2010-05-01T00:00:01 line 2
EOF

export DATEGREP_FORMAT="%FT%T"
call dategrep -f "2010-05-01T00:00:01" -t "2010-05-01T00:00:02"
unset -v DATEGREP_FORMAT


#################
done_testing
