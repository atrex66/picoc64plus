echo "Building project"
rm -rf build
mkdir build && cd build
cmake BOARD=pico ..
make -j4
