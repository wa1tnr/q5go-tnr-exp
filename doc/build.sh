#!/bin/sh

# cd ../../../q5go-tnr-exp/doc
# mkdir ../build-me.d
# cd    ../build-me.d

qmake ../src/q5go.pro PREFIX=/home/mylogin/bin

exit 0
  make
  make install
