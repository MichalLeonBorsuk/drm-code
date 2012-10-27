#!/bin/sh
cd dream.app/Contents/

mkdir Frameworks
cp /opt/local/lib/libsndfile.1.dylib Frameworks
cp /opt/local/lib/libz.1.dylib Frameworks
cp /opt/local/lib/libqwt.5.1.2.dylib Frameworks
cp /usr/local/lib/libhamlib.2.dylib Frameworks
cp /opt/local/lib/libportaudio.2.dylib Frameworks
cp /opt/local/libexec/qt4-mac/lib/QtXml.framework/Versions/4/QtXml Frameworks
cp /opt/local/libexec/qt4-mac/lib/QtGui.framework/Versions/4/QtGui Frameworks
cp /opt/local/libexec/qt4-mac/lib/QtNetwork.framework/Versions/4/QtNetwork Frameworks
cp /opt/local/libexec/qt4-mac/lib/QtCore.framework/Versions/4/QtCore Frameworks

install_name_tool -id @executable_path/../Frameworks/libqt-mt.3.dylib Frameworks/libqt-mt.3.dylib 
install_name_tool -id @executable_path/../Frameworks/libfaac.0.dylib Frameworks/libfaac.0.dylib 
install_name_tool -id @executable_path/../Frameworks/libsndfile.1.dylib Frameworks/libsndfile.1.dylib 
install_name_tool -id @executable_path/../Frameworks/libhamlib.2.dylib Frameworks/libhamlib.2.dylib

install_name_tool -change /opt/local/lib/libsndfile.1.dylib @executable_path/../Frameworks/libsndfile.1.dylib MacOS/dream
install_name_tool -change /opt/local/lib/libz.1.dylib @executable_path/../Frameworks/libz.1.dylib MacOS/dream
install_name_tool -change /opt/local/lib/libqwt.5.1.2.dylib @executable_path/../Frameworks/libqwt.5.1.2.dylib MacOS/dream
install_name_tool -change /usr/local/lib/libhamlib.2.dylib @executable_path/../Frameworks/libhamlib.2.dylib MacOS/dream
install_name_tool -change /opt/local/lib/libportaudio.2.dylib @executable_path/../Frameworks/libportaudio.2.dylib MacOS/dream
install_name_tool -change /opt/local/libexec/qt4-mac/lib/QtXml.framework/Versions/4/QtXml @executable_path/../Frameworks/QtXml MacOS/dream
install_name_tool -change /opt/local/libexec/qt4-mac/lib/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui MacOS/dream
install_name_tool -change /opt/local/libexec/qt4-mac/lib/QtNetwork.framework/Versions/4/QtNetwork @executable_path/../Frameworks/QtNetwork MacOS/dream
install_name_tool -change /opt/local/libexec/qt4-mac/lib/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore MacOS/dream
cd ../.. MacOS/dream
