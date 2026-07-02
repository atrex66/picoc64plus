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
40 for y=1 to 25
50 for x=1 to 40
60 a=sin(y/5)*cos(x/5)+log(x+y)
70 poke 55296+(x-1)+(y-1)*40,abs(a*8)+24
80 next x
90 next y
