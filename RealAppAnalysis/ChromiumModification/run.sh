#!/usr/bin/env bash

#
# Top Matter
#

ROOT=/Volumes/WrongConnector/Chromium
CHROMIUM_REPO=$ROOT/chromium/src

rm Traces/*

${CHROMIUM_REPO}/out/Default/Chromium.app/Contents/MacOS/Chromium --no-sandbox
# ${CHROMIUM_REPO}/out/Default/Chromium.app/Contents/MacOS/Chromium
