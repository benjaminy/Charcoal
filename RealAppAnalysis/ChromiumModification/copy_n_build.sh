#!/usr/bin/env bash

#
# Top Matter
#

ROOT=/Volumes/WrongConnector/Chromium
CHROMIUM_REPO=$ROOT/chromium/src
PATH=$PATH:$ROOT/depot_tools
echo $PATH

cp CoreProbes.cpp ${CHROMIUM_REPO}/third_party/WebKit/Source/core/probe/
cp isolate.cc     ${CHROMIUM_REPO}/v8/src/

cd ${CHROMIUM_REPO}
pwd
ninja -C out/Default chrome
