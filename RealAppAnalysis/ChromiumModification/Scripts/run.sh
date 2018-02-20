#!/usr/bin/env bash

#
# Top Matter
#

ROOT=/Volumes/WrongConnector/Chromium
CHROMIUM_REPO=$ROOT/chromium_release/src

FLAGS="--enable-logging=stderr --no-sandbox"
# FLAGS=

rm -f tmp/*

${CHROMIUM_REPO}/out/Default/Chromium.app/Contents/MacOS/Chromium ${FLAGS} 2> stderr.txt

STAMP=$(date "+%Y.%m.%d-%H.%M.%S")
mkdir -p Traces/$STAMP/Traces
mv -v tmp/* Traces/$STAMP/Traces/
mv stderr.txt Traces/$STAMP/
