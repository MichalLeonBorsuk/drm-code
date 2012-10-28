#!/bin/sh
mkdir dream.app/Contents/Frameworks
cp /usr/local/lib/libhamlib.2.dylib dream.app/Contents/Frameworks/
cp /opt/local/lib/libz.1.dylib dream.app/Contents/Frameworks/
cp /opt/local/lib/libportaudio.2.dylib dream.app/Contents/Frameworks/
cp /opt/local/lib/libsndfile.1.dylib dream.app/Contents/Frameworks/
install_name_tool -change /usr/local/lib/libhamlib.2.dylib @executable_path/../Frameworks/libhamlib.2.dylib dream.app/Contents/MacOS/dream 
install_name_tool -change /opt/local/lib/libportaudio.2.dylib @executable_path/../Frameworks/libportaudio.2.dylib dream.app/Contents/MacOS/dream 
install_name_tool -change /opt/local/lib/libsndfile.1.dylib @executable_path/../Frameworks/libsndfile.1.dylib dream.app/Contents/MacOS/dream 
install_name_tool -change /opt/local/lib/libz.1.dylib @executable_path/../Frameworks/libz.1.dylib dream.app/Contents/MacOS/dream 
macdeployqt dream.app -dmg
