#!/bin/bash

set -e

CURRENT_PATH=$(pwd)
INCLUDE_PATH="/usr/local/include/yieldemuduo"

if [ ! -d ${CURRENT_PATH}/build ]; then
  mkdir ${CURRENT_PATH}/build
fi

if [ ! -d ${CURRENT_PATH}/lib ]; then
  mkdir ${CURRENT_PATH}/lib
fi

cd ${CURRENT_PATH}/build

cmake ../src/

make -j 4
cd ${CURRENT_PATH}

# 放到系统中
if [ ! -d ${INCLUDE_PATH} ]; then
  mkdir ${INCLUDE_PATH}
fi

for h in $(ls src/*.h); do
  cp ${h} ${INCLUDE_PATH}
done

cp ${CURRENT_PATH}/lib/libyieldemuduo.so /usr/local/lib
echo "-----------------"
echo "------done-------"
echo "-----------------"
