#/bin/bash


mkdir build
cp test/doc.txt build
cd build
cmake ..
make
