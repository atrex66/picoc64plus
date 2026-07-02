echo "Building test program"
cd testprogram
cl65 plasma.c -o plasma.prg
cp plasma.prg ..
cd ..
echo "Generating header file from plasma.prg"
python3 prgtoheader.py
echo "Building project"
rm -rf build
mkdir build && cd build
cmake BOARD=pico ..
make -j4
