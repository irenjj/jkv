#!/bin/sh

if test -d third-party/jraft/build; then
  if test -d build; then
           echo "build dir already exists; rm -rf build and re-run"
           rm -rf build
  fi
  mkdir build && cd build && cmake .. && make -j20
else cd third-party/jraft
     ./do_cmake.sh && ./make_output.sh
     if test -d build; then
         echo "build dir already exists; rm -rf build and re-run"
         rm -rf build
     fi
     cd ../..
     mkdir build && cd build && cmake .. && make -j20
fi
