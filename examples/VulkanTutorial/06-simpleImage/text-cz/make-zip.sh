#!/bin/sh

if test -f 03-simpleImage.zip; then
	rm 03-simpleImage.zip
fi
mkdir tmp
cp ../main.cpp tmp/
echo "cmake_minimum_required(VERSION 3.8.0)" > tmp/CMakeLists.txt
echo >> tmp/CMakeLists.txt
cat < ../CMakeLists.txt >> tmp/CMakeLists.txt
cp text.html tmp/
cd tmp
zip 03-simpleImage.zip main.cpp CMakeLists.txt text.html
mv 03-simpleImage.zip ..
cmake .
make
./03-simpleImage
cd ..
#rm -r tmp
