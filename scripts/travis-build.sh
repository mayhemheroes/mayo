#!/bin/bash

# Make the script fails on any command error
set -e

# # Force QMake spec in case of Clang
# if [ "${TRAVIS_COMPILER}" == "clang" ]; then
#     export QMAKESPEC=linux-clang-libc++
# fi

echo QMAKESPEC=${QMAKESPEC}

# Create out-of-source dir
mkdir build
cd build

# Run QMake
qmake --version
if [ "${TRAVIS_OS_NAME}" == "linux" ]; then
    qmake ../mayo.pro CASCADE_INC_DIR=/usr/include/opencascade
fi

if [ "${TRAVIS_OS_NAME}" == "osx" ]; then
    qmake ../mayo.pro CASCADE_INC_DIR=/usr/local/Cellar/opencascade/7.5.3/include/opencascade CASCADE_LIB_DIR=/usr/local/Cellar/opencascade/7.5.3/lib
fi

# Make
make -j2
