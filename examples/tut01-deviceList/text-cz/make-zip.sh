#!/bin/sh

rm tut01-deviceList.zip
mkdir tmp
cp ../main.cpp tmp/
echo "cmake_minimum_required(VERSION 3.8.0)" > tmp/CMakeLists.txt
echo >> tmp/CMakeLists.txt
cat < ../CMakeLists.txt >> tmp/CMakeLists.txt
cp text.html tmp/
cd tmp
zip tut01-deviceList.zip main.cpp CMakeLists.txt text.html
mv tut01-deviceList.zip ..
cmake .
make
./tut01-deviceList
cd ..
rm -r tmp