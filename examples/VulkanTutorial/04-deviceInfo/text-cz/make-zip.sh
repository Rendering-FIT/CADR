#!/bin/sh

if test -f 04-deviceInfo.zip; then
	rm 04-deviceInfo.zip
fi
mkdir tmp
cp ../main.cpp tmp/
echo "cmake_minimum_required(VERSION 3.8.0)" > tmp/CMakeLists.txt
echo >> tmp/CMakeLists.txt
cat < ../CMakeLists.txt >> tmp/CMakeLists.txt
cp text.html tmp/
cd tmp
zip 04-deviceInfo.zip main.cpp CMakeLists.txt text.html
mv 04-deviceInfo.zip ..
cmake .
make
./04-deviceInfo
cd ..
#rm -r tmp
