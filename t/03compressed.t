#!tapsig

depends_on gzip
depends_on bzcat

#################
name "Uncompress gzip file on the fly"

gzip > input.gz <<EOF
2010-05-01T00:00:00 line 1
2010-05-01T00:00:01 line 2
2010-05-01T00:00:02 line 3
EOF

stdout_is <<EOF
2010-05-01T00:00:01 line 2
EOF

tap dategrep -f "2010-05-01T00:00:01" -t "2010-05-01T00:00:02" -F "%FT%T" input.gz

#################

name "Uncompress bzip2 file on the fly"

bzip2 > input.bz2 <<EOF
2010-05-01T00:00:00 line 1
2010-05-01T00:00:01 line 2
2010-05-01T00:00:02 line 3
EOF

stdout_is <<EOF
2010-05-01T00:00:01 line 2
EOF

tap dategrep -f "2010-05-01T00:00:01" -t "2010-05-01T00:00:02" -F "%FT%T" input.bz2

#################
name "Uncompressed and compressed files"

cat > input <<EOF
2010-05-01T00:00:00 line 1
2010-05-01T00:00:01 line 2
2010-05-01T00:00:02 line 3
EOF

gzip > input.gz <<EOF
2010-05-01T00:00:00 line 1
2010-05-01T00:00:01 line 2
2010-05-01T00:00:02 line 3
EOF

stdout_is <<EOF
2010-05-01T00:00:01 line 2
2010-05-01T00:00:01 line 2
EOF

tap dategrep -f "2010-05-01T00:00:01" -t "2010-05-01T00:00:02" -F "%FT%T" input.gz input

#################
done_testing
