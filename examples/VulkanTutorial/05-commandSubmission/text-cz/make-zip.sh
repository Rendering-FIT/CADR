#!/bin/sh

NAME="$(basename "$(dirname "`pwd`")")"
echo "Testing and packing \"$NAME\" example..."
if test -f $NAME.zip; then
	rm $NAME.zip
fi
mkdir tmp
cp ../main.cpp tmp/
echo "cmake_minimum_required(VERSION 3.8.0)" > tmp/CMakeLists.txt
echo >> tmp/CMakeLists.txt
cat < ../CMakeLists.txt >> tmp/CMakeLists.txt
cp text.html tmp/
cd tmp
zip $NAME.zip main.cpp CMakeLists.txt text.html
mv $NAME.zip ..
cmake .
make
./$NAME
cd ..
#rm -r tmp
