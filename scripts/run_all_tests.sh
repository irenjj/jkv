#!/bin/sh

cd build/test

cd byte_buffer_test && ./byte_buffer_test && cd ..
cd snapper_test && ./snapper_test && cd ..
cd wal_test && ./wal_test && cd ..
