mkdir testZone
cd testZone
echo '1' > one
echo '2' > two

# TEST ONE - SANITY CHECK ------------------------------------------------------
(
cat >test-t-0.sh <<'EOF'
cat one                       # 1
EOF
)

(
cat >test-t-0.exp <<'EOF'
1
EOF
)

(
cat >test-t-0-chk.sh << 'EOF'
diff -u testoutput.out  test-t-0.exp
EOF
)

# ------------------------------------------------------------------------------

(
cat >test-t-1.sh <<'EOF'
cat one > two && sleep 2
cat two > one
echo 2 > two
cat one && cat two
EOF
)

(
cat >test-t-1.exp <<'EOF'
1
2
EOF
)

(
cat >test-t-1-chk.sh << 'EOF'
diff -u testoutput.out  test-t-1.exp
EOF
)

# ------------------------------------------------------------------------------

(
cat >test-t-2.sh <<'EOF'
sleep 5 && echo goodbye
echo hello                      # hello should appear before goodbye
EOF
)

(
cat >test-t-2.exp <<'EOF'
hello
goodbye
EOF
)

(
cat >test-t-2-chk.sh << 'EOF'
diff -u testoutput.out  test-t-2.exp
EOF
)

# ------------------------------------------------------------------------------

(
cat >test-t-3.sh <<'EOF'
sleep 5 && ../notARealCommand 
echo yay!                     #  should still exit with code 0
EOF
)

(
cat >test-t-3.exp <<'EOF'
yay!
EOF
)

(
cat >test-t-3-chk.sh << 'EOF'
if [ $? -eq 0 ]
then
  diff -u testoutput.out  test-t-3.exp
else
  echo WRONG EXIT STATUS
  exit 1
fi
EOF
)

# ------------------------------------------------------------------------------

(
echo 'oh, hello' > a
echo 'junk' > b
echo 'junk' > c
echo 'junk' > d
echo 'junk' > e
echo 'junk' > f
echo 'junk' > g
echo '###' > test-t-4.sh
PREV='a'
echo "sleep 2 && echo nothing" >> test-t-4.sh
echo "cat one | cat | cat > new.file" >> test-t-4.sh
for CURR in b c d e f g
do
  echo "cat $PREV > $CURR && cat $CURR" >> test-t-4.sh
  PREV=$CURR
done
)

(
cat >test-t-4.exp <<'EOF'
oh, hello
oh, hello
oh, hello
oh, hello
oh, hello
oh, hello
nothing
EOF
)

(
cat >test-t-4-chk.sh << 'EOF'
cat new.file | grep 1 > /dev/null || (echo newfile bad && exit 1)
diff -u testoutput.out test-t-4.exp
EOF
)


# ------------------------------------------------------------------------------

(
cat >test-t-5.sh <<'EOF'
sleep 2 && (echo yo > new.file)
echo something
cat < new.file && sleep 1
./notARealFile || (echo hi > new.file && cat new.file | 
                    grep hi > /dev/null && echo whee)
cat < new.file
EOF
)

(
cat >test-t-5.exp <<'EOF'
something
yo
whee
hi
EOF
)

(
cat >test-t-5-chk.sh << 'EOF'
diff -u testoutput.out  test-t-5.exp
EOF
)

for N in 0 1 2 3 4 5
do
  ../timetrash -t test-t-$N.sh > testoutput.out
  ./test-t-$N-chk.sh && echo PASSED TEST SCRIPT $N ||
    echo FAILED TEST SCRIPT $N
done

cd ..

rm -rf testZone


