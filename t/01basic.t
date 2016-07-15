#!tapsig

#################
name "Empty input results in empty output"

cat > input <<EOF
EOF

tap dategrep -f "2010-05-01T00:00:00" -t "2010-05-01T00:00:01" input

#################
name "Match all input lines"

cat > input <<EOF
2010-05-01T00:00:00 line 1
2010-05-01T00:00:01 line 2
2010-05-01T00:00:02 line 3
EOF

stdout_is <<EOF
2010-05-01T00:00:00 line 1
2010-05-01T00:00:01 line 2
2010-05-01T00:00:02 line 3
EOF

tap dategrep -f "2010-05-01T00:00:00" -t "2010-05-01T00:00:03" -F "%FT%T" input

#################
name "Match no lines"

tap dategrep -f "2010-05-01T00:00:03" -F "%FT%T" input

#################
name "Output single line in middle of input"

stdout_is <<EOF
2010-05-01T00:00:01 line 2
EOF

tap dategrep -f "2010-05-01T00:00:01" -t "2010-05-01T00:00:02" -F "%FT%T" input

#################
name "Skip dateless lines"

cat > input <<EOF
2010-05-01T00:00:00 line 1
foo
2010-05-01T00:00:01 line 2
2010-05-01T00:00:02 line 3
EOF

rc_is 1

stderr_is <<EOF
dategrep: Found line without date: foo
EOF

tap dategrep -f "2010-05-01T00:00:01" -t "2010-05-01T00:00:02" -F "%FT%T" input

#################
name "Skip dateless lines"

stdout_is <<EOF
2010-05-01T00:00:01 line 2
EOF

tap dategrep -s -f "2010-05-01T00:00:01" -t "2010-05-01T00:00:02" -F "%FT%T" input

#################
name "Print multine logs"

cat > input <<EOF
2010-05-01T00:00:00 line 1
foo
2010-05-01T00:00:01 line 2
bar
2010-05-01T00:00:02 line 3
EOF

stdout_is <<EOF
2010-05-01T00:00:01 line 2
bar
EOF

tap dategrep -f "2010-05-01T00:00:01" -t "2010-05-01T00:00:02"  -F "%FT%T" -m input

#################
name "Error without format"

cat > input <<EOF
2010-05-01T00:00:01 line 2
EOF

stderr_is <<EOF
dategrep: Found line without date: 2010-05-01T00:00:01 line 2
EOF

rc_is 1

tap dategrep -f "2010-05-01T00:00:01" -t "2010-05-01T00:00:02" input

#################
name "Getting format from environment"

cat > input <<EOF
2010-05-01T00:00:01 line 2
EOF

stdout_is <<EOF
2010-05-01T00:00:01 line 2
EOF

export DATEGREP_FORMAT="%FT%T"
tap dategrep -f "2010-05-01T00:00:01" -t "2010-05-01T00:00:02" input
unset -v DATEGREP_FORMAT


#################

done_testing
