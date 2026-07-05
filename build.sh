echo "Building project"
cd CustomBasicCommands
make clean
make
cd ..
rm -rf build
mkdir build && cd build
cmake BOARD=pico ..
make -j4
