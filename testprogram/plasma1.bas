1 y=y+1 
2 for x=1to40
3 a=sin(y/5)*cos(x/5)+log(x+y)
5 poke646,abs(a*2):printchr$(84);
6 next
7 goto 1

1 print chr$(147): rem clear screen
2 poke 53295,1: rem 256 color text mode
10 for i=0 to 1000
20 poke 1024+i,208
30 next
40 FOR y=1 TO 25
50 FOR x=1 TO 40
60 A=SIN(y/5)*COS(x/5)+LOG(x+y)
70 POKE 55296+(X-1)+(Y-1)*40,abs(A*8)+24
80 POKE X
90 POKE Y
