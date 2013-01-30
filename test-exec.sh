mkdir testZone
cd testZone
echo '1' > one
echo '2' > two

(
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
(true && false) || echo GOOD1  # GOOD1
((true || false) || echo BAD) && echo GOOD2 # GOOD2
(true || false || echo BAD) && echo GOOD3 # GOOD3
(true || (true && echo BAD)) && echo GOOD4 #GOOD4
(false) || echo GOOD5
(echo hello > temp) && (cat two | grep 2) #2
(echo bye) | cat < temp #hello
(echo bye > temp) && cat temp #bye

EOF
)
(
cat >testoutput.exp <<'EOF'
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
GOOD1
GOOD2
GOOD3
GOOD4
GOOD5
2
hello
bye
EOF
)


../timetrash test-e-ok.sh > testoutput.out

diff -u testoutput.out testoutput.exp && echo PASSED ALL TESTS || echo TEST FAILED

cd ..

rm -rf testZone


