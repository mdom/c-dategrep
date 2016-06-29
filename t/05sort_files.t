#!tapsig

#################
name "Sort lines from multiple files"

cat > input1 <<EOF
2010-05-01T00:00:00 file 1 line 1
2010-05-01T00:00:02 file 1 line 2
2010-05-01T00:00:04 file 1 line 3
EOF

cat > input2 <<EOF
2010-05-01T00:00:01 file 2 line 1
2010-05-01T00:00:03 file 2 line 2
2010-05-01T00:00:05 file 2 line 3
EOF

stdout_is <<EOF
2010-05-01T00:00:01 file 2 line 1
2010-05-01T00:00:02 file 1 line 2
2010-05-01T00:00:03 file 2 line 2
2010-05-01T00:00:04 file 1 line 3
EOF

tap dategrep -f "2010-05-01T00:00:01" -t "2010-05-01T00:00:05" -F "%FT%T" input2 input1

rm input1 input2

#################
done_testing
