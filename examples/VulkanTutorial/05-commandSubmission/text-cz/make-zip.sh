#!/bin/sh

if test -f 05-commandSubmission.zip; then
	rm 05-commandSubmission.zip
fi
mkdir tmp
cp ../main.cpp tmp/
echo "cmake_minimum_required(VERSION 3.8.0)" > tmp/CMakeLists.txt
echo >> tmp/CMakeLists.txt
cat < ../CMakeLists.txt >> tmp/CMakeLists.txt
cp text.html tmp/
cd tmp
zip 05-commandSubmission.zip main.cpp CMakeLists.txt text.html
mv 05-commandSubmission.zip ..
cmake .
make
./05-commandSubmission
cd ..
#rm -r tmp
