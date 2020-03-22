#!/bin/sh

if test -f 03-funcPointers.zip; then
	rm 03-funcPointers.zip
fi
mkdir tmp
cp ../main.cpp tmp/
echo "cmake_minimum_required(VERSION 3.8.0)" > tmp/CMakeLists.txt
echo >> tmp/CMakeLists.txt
cat < ../CMakeLists.txt >> tmp/CMakeLists.txt
cp text.html tmp/
cd tmp
zip 03-funcPointers.zip main.cpp CMakeLists.txt text.html
mv 03-funcPointers.zip ..
cmake .
make
./03-funcPointers
cd ..
rm -r tmp
