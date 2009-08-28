#!/bin/bash
# this is a utility script which renames and moves the resulting package
make package
mv ror-*-Linux-i686.tar.gz $1/ror-r$2-Linux-i686.tar.gz
echo $1/ror-r$2-Linux-i686.tar.gz