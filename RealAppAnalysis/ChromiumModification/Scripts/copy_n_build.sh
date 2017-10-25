#!/usr/bin/env bash

#
# Top Matter
#

# Change ROOT to be the directory/folder where the Chromium repo is installed

ROOT=/Volumes/WrongConnector/Chromium
CHROMIUM_REPO=$ROOT/chromium/src
V8_SRC=${CHROMIUM_REPO}/v8/src

PATH=$PATH:$ROOT/depot_tools
echo $PATH

diff Source/CoreProbes.cpp ${CHROMIUM_REPO}/third_party/WebKit/Source/core/probe/CoreProbes.cpp
if [ $? -ne 0 ]; then
    echo "CoreProbes.cpp different!";
    cp Sourse/CoreProbes.cpp ${CHROMIUM_REPO}/third_party/WebKit/Source/core/probe/
fi

diff Source/isolate.cc ${V8_SRC}/isolate.cc
if [ $? -ne 0 ]; then
    echo "isolate.cc different!";
    cp Source/isolate.cc ${V8_SRC}/
fi

diff Source/execution.cc ${V8_SRC}/execution.cc
if [ $? -ne 0 ]; then
    echo "execution.cc different!";
    cp Source/execution.cc ${V8_SRC}/
fi

cd ${CHROMIUM_REPO}
pwd
time ninja -C out/Default chrome
osascript -e "beep"
