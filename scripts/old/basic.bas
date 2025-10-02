10 open 2,8,2,"dump.seq,s,r"
20 open 4,4
30 get#2,a$ : if st<>0 then 60
40 print#4,a$; : goto 30
60 close 2 : close 4
70 poke 2,1
80 end
