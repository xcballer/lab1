#! /bin/sh
# Test that our shell executes commands properly.

tmp=$0-$$.tmp
mkdir "$tmp" || exit

(
cd "$tmp" || exit
echo '1' > one
echo '2' > two

cat >test-e-ok.sh <<'EOF'
cat one                       # 1

cat two                       # 2

cat one && cat two            # 1
                              # 2

cat one || cat two            # 1

cat one | cat two             # 2

cat one > two && cat one      # 1

cat two                       # 1

echo 2 > two && cat two       # 2

grep 2 two                    # 2

grep 1 two || echo YAY        # YAY

grep 1 two || echo HI && echo YAY # HI
                                  # YAY
echo ..1
(true && false) || echo GOOD  # GOOD
echo ..1
((true || false) || echo BAD) && echo GOOD # GOOD
echo ..1
(true || false || echo BAD) && echo GOOD # GOOD
echo ..1
(true || (true && echo BAD)) && echo GOOD #GOOD

echo left;echo right    #left right

echo curls | cat | cat | cat | cat      # curls

echo faa && false && echo what       #faa

echo foo || false || echo what       #foo

(false) && echo UHOH     # no output here

false || echo YES         #YES

(false) || echo OK          #OK

EOF

cat >test.exp <<'EOF'
1
2
1
2
1
2
1
1
2
2
YAY
HI
YAY
..1
GOOD
..1
GOOD
..1
GOOD
..1
GOOD
left
right
curls
faa
foo
YES
OK
EOF

../timetrash test-e-ok.sh > test.out 2> test.err || exit

diff -u test.exp test.out || exit
test ! -s test.err || {
  cat test.err
  exit 1
}

) || exit

rm -rf "$tmp"


