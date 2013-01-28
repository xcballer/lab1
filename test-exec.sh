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
echo ..1
(true && false) || echo GOOD  # GOOD
echo ..1
((true || false) || echo BAD) && echo GOOD # GOOD
echo ..1
(true || false || echo BAD) && echo GOOD # GOOD
echo ..1
(true || (true && echo BAD)) && echo GOOD #GOOD

EOF
)


../timetrash test-e-ok.sh

cd ..

rm -rf testZone


