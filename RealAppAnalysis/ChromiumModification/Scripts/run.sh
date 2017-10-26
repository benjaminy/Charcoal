#!/usr/bin/env bash

#
# Top Matter
#

ROOT=/Volumes/WrongConnector/Chromium
CHROMIUM_REPO=$ROOT/chromium_release/src

rm Traces/*

${CHROMIUM_REPO}/out/Default/Chromium.app/Contents/MacOS/Chromium --no-sandbox 2> stderr.txt
# ${CHROMIUM_REPO}/out/Default/Chromium.app/Contents/MacOS/Chromium
