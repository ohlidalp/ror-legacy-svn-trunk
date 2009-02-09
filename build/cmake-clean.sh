#!/bin/sh

for F in $(find . -name "CMakeCache.txt")
do
echo "removing $F ..."
rm -f $F
done

for F in $(find . -name "CMakeFiles")
do
echo "removing $F ..."
rm -rf $F
done

echo "cmake files removed!"