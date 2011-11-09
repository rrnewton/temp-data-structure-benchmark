#!/bin/sh

PATH=/usr/local/bin:$PATH
export PATH

./build.sh \
--clean \
--with-make gmake \
--with-boost /usr/local/include \
-x g++43 \
-l "-L/usr/local/lib" \
2>&1 | tee build-freebsd-amd64.log
