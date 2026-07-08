echo "Building project"
cd CustomBasicCommands
make clean
make
cd ..
rm -rf build
mkdir build && cd build
cmake -DPICO_BOARD=pico2 ..
make -j4
cd ..