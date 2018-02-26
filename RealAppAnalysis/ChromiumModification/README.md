Building Chromium

git checkout tags/62.0.3202.71
gclient sync --with_branch_heads

By default the Chromium built doesn't have proprietary media codecs and it's dog slow.
Below are some very sketchy notes about how to fix that.
After making changes, need to gclient sync again ... maybe something else

/src/build/config/features.gni
  -->   proprietary_codecs = true
/src/build/config/BUILDCONFIG.gn
  -->   is_official_build = false
/src/third_party/ffmpeg/ffmpeg_options.gni
  -->   ffmpeg_branding = "Chrome"

cd into directory to git commit



Collecting Traces

- Install the extension
  - Run Chromium
  - navigate to chrome://extensions
  - click on Developer mode (or something like that... "advanced" or whatever)
  - choose "load unpacked extension"
  - find the directory with the extension and choose it
  - Note: the current analysis script doesn't actually use the on/off thing, but it's still important to install the extension because it prints information about process types

- Do a collection run
  - make sure you're in a directory/folder with subdirectories called "tmp" and "Traces"
  - On mac or linux you can just run ./Scripts/run.sh from the ChromiumModification directory
  - That script leaves the files from a run in a directory called Traces/[Timestamp]  You might want to rename the timestamp directory to something more descriptive (like FacebookLogin)
  - On Windows, for now you have to do the equivalent of the script stuff by hand.  Ben doesn't know how to write Windows scripts