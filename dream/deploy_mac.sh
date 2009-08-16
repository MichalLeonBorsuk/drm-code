#!/bin/sh
cd dream.app/Contents/

mkdir Frameworks
cp /Developer/qt/lib/libqt-mt.3.dylib Frameworks
cp /Developer/dream/lib/libqwt.4.dylib Frameworks
cp /Developer/dream/lib/libsndfile.1.dylib Frameworks
cp /Developer/dream/lib/libfaac.0.dylib Frameworks
cp /usr/local/lib/libhamlib.2.dylib Frameworks

install_name_tool -id @executable_path/../Frameworks/libqt-mt.3.dylib Frameworks/libqt-mt.3.dylib 
install_name_tool -id @executable_path/../Frameworks/libfaac.0.dylib Frameworks/libfaac.0.dylib 
install_name_tool -id @executable_path/../Frameworks/libsndfile.1.dylib Frameworks/libsndfile.1.dylib 
install_name_tool -id @executable_path/../Frameworks/libhamlib.2.dylib Frameworks/libhamlib.2.dylib 

install_name_tool -change libqt-mt.3.dylib @executable_path/../Frameworks/libqt-mt.3.dylib Frameworks/libqwt.4.dylib 

install_name_tool -change libqt-mt.3.dylib @executable_path/../Frameworks/libqt-mt.3.dylib MacOS/dream 
install_name_tool -change libqwt.4.dylib @executable_path/../Frameworks/libqwt.4.dylib MacOS/dream 
install_name_tool -change /usr/local/lib/libhamlib.2.dylib @executable_path/../Frameworks/libhamlib.2.dylib MacOS/dream 
install_name_tool -change /Developer/dream/lib/libfaac.0.dylib @executable_path/../Frameworks/libfaac.0.dylib MacOS/dream 
install_name_tool -change /Developer/dream/lib/libsndfile.1.dylib @executable_path/../Frameworks/libsndfile.1.dylib MacOS/dream 

mkdir PlugIns
cp /Developer/qt/plugins/imageformats/libqmng.dylib PlugIns
cp /Developer/qt/plugins/imageformats/libqjpeg.dylib PlugIns

cd ../..
