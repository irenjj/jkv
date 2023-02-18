#!/bin/sh

cd third-party/jraft
if test -d build; then
  echo "build dir already exists"
else
  rm -rf build && ./do_cmake.sh && ./make_output.sh
fi

cd ../..

if test -d build; then
  echo "build dir already exists; rm -rf build and re-run"
  rm -rf build
fi
mkdir build && cd build && cmake .. && make -j20
