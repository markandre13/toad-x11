#!/bin/sh

set -e

rm -rf doxylinks
mkdir -p doxylinks/include
for x in `(cd .. ; find src -type d)` ; do mkdir doxylinks/$x ; done
mv doxylinks/src doxylinks/include/toad
for x in `(cd .. ; find src -type d)` ; do mkdir doxylinks/$x ; done
for x in `(cd ../src ; find . -type f -name "*\.hh")` ; do 
  ln -s "`pwd`/../src/$x" doxylinks/include/toad/$x
done
for x in `(cd ../src ; find . -type f -name "*\.cc")` ; do 
  ln -s "`pwd`/../src/$x" doxylinks/src/$x
done
