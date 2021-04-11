#!/bin/sh

if test -f 07-robustImage.zip; then
	rm 07-robustImage.zip
fi
mkdir tmp
cp ../main.cpp tmp/
echo "cmake_minimum_required(VERSION 3.8.0)" > tmp/CMakeLists.txt
echo >> tmp/CMakeLists.txt
cat < ../CMakeLists.txt >> tmp/CMakeLists.txt
cp text.html tmp/
cd tmp
zip 07-robustImage.zip main.cpp CMakeLists.txt text.html
mv 07-robustImage.zip ..
cmake .
make
./07-robustImage
cd ..
#rm -r tmp
