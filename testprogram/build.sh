export PATH=$PATH:/usr/share/cc65/include
cl65 -o plasma.prg -t c64 plasma.c -I cc65_support
python3 prgtoheader.py
cp plasma.prg.h ../src/include/plasma.prg.h
