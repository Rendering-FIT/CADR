#!/bin/sh

if test -f 01-deviceList.zip; then
	rm 01-deviceList.zip
fi
mkdir tmp
cp ../main.cpp tmp/
echo "cmake_minimum_required(VERSION 3.8.0)" > tmp/CMakeLists.txt
echo >> tmp/CMakeLists.txt
cat < ../CMakeLists.txt >> tmp/CMakeLists.txt
cp text.html tmp/
cd tmp
zip 01-deviceList.zip main.cpp CMakeLists.txt text.html
mv 01-deviceList.zip ..
cmake .
make
./01-deviceList
cd ..
rm -r tmp
