#!/usr/bin/env bash

#
# Top Matter
#

ROOT=/Volumes/WrongConnector/Chromium
CHROMIUM_REPO=$ROOT/chromium/src
V8_SRC=${CHROMIUM_REPO}/v8/src

PATH=$PATH:$ROOT/depot_tools
echo $PATH

diff CoreProbes.cpp ${CHROMIUM_REPO}/third_party/WebKit/Source/core/probe/CoreProbes.cpp
if [ $? -ne 0 ]; then
    echo "CoreProbes.cpp different!";
    cp CoreProbes.cpp ${CHROMIUM_REPO}/third_party/WebKit/Source/core/probe/
fi

diff isolate.cc ${V8_SRC}/isolate.cc
if [ $? -ne 0 ]; then
    echo "isolate.cc different!";
    cp isolate.cc ${V8_SRC}/
fi

diff execution.cc ${V8_SRC}/execution.cc
if [ $? -ne 0 ]; then
    echo "execution.cc different!";
    cp execution.cc ${V8_SRC}/
fi

cd ${CHROMIUM_REPO}
pwd
ninja -C out/Default chrome
