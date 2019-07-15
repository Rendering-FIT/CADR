#!/bin/sh

rm tut01-deviceList.zip
mkdir tmp
cp ../main.cpp tmp/
echo "cmake_minimum_required(VERSION 3.8.0)" > tmp/CMakeLists.txt
echo >> tmp/CMakeLists.txt
cat < ../CMakeLists.txt >> tmp/CMakeLists.txt
sed -i 's/# dependencies/# dependencies\nset(CMAKE_MODULE_PATH "${${APP_NAME}_SOURCE_DIR}\/;${CMAKE_MODULE_PATH}")/g' tmp/CMakeLists.txt
cp FindVulkan.cmake tmp/
cp text.html tmp/
cd tmp
zip tut01-deviceList.zip main.cpp CMakeLists.txt FindVulkan.cmake text.html
mv tut01-deviceList.zip ..
cmake .
make
./tut01-deviceList
cd ..
rm -r tmp