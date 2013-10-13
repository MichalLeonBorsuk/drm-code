contains(QT_VERSION, ^4\\..*) {
    CONFIG += qt qt4
    VERSION_MESSAGE = Qt 4
}
contains(QT_VERSION, ^5\\..*) {
    CONFIG += qt qt5
    VERSION_MESSAGE = Qt 5
}
qt4|qt5 {
    CONFIG(debug, debug|release) {
        DEBUG_MESSAGE = debug
    }
    else {
        DEBUG_MESSAGE = release
    }
}
console {
    QT -= core gui
    CONFIG -= qt qt4 qt5
    UI_MESSAGE = console mode
    VERSION_MESSAGE = No Qt
}
else {
    qtconsole {
        QT += xml
        QT -= gui
        UI_MESSAGE = console mode
    }
    else {
        RESOURCES = src/GUI-QT/res/icons.qrc
        UI_MESSAGE = GUI mode
    }
}
message($$VERSION_MESSAGE $$DEBUG_MESSAGE $$UI_MESSAGE)
unix {
    UNAME = $$system(uname -s)
    message(on $$UNAME)
}
TEMPLATE = app
CONFIG += warn_on
TARGET = dream
INCLUDEPATH += libs src/GUI-QT
LIBS += -Llibs
OBJECTS_DIR = obj
DEFINES += EXECUTABLE_NAME=$$TARGET
macx:QMAKE_LFLAGS += -F$$PWD/libs
qt4 {
    QT += network xml webkit
    VPATH += src/GUI-QT
    unix {
        macx {
            message("with qwt")
            INCLUDEPATH += /opt/local/Library/Frameworks/qwt.framework/Versions/6/Headers
            LIBS += -framework qwt
            CONFIG += qwt
        }
        else {
            exists(/usr/local/include/qwt.h) {
                message("with qwt")
                LIBS += -lqwt
                CONFIG += qwt
            }
            exists(/usr/local/include/qwt/qwt.h) {
                message("with qwt")
                INCLUDEPATH += /usr/local/include/qwt
                LIBS += -lqwt
                CONFIG += qwt
            }
            exists(/usr/include/qwt/qwt.h) {
                message("with qwt")
                INCLUDEPATH += /usr/include/qwt
                LIBS += -lqwt
                CONFIG += qwt
            }
            exists(/usr/include/qwt5/qwt.h) {
                message("with qwt")
                INCLUDEPATH += /usr/include/qwt5
                LIBS += -lqwt
                CONFIG += qwt
            }
            exists(/usr/include/qwt-qt4/qwt.h) {
                message("with qwt")
                INCLUDEPATH += /usr/include/qwt-qt4
                LIBS += -lqwt-qt4
                CONFIG += qwt
            }
            target.path = /usr/bin
            documentation.path = /usr/share/man/man1
            documentation.files = linux/dream.1
            INSTALLS += documentation
        }
    }
    win32 {
        RC_FILE = windows/dream.rc
        INCLUDEPATH += libs/qwt
        CONFIG( debug, debug|release ) {
            # debug
            LIBS += -lqwtd
        } else {
            # release
            LIBS += -lqwt
        }
        CONFIG += qwt
    }
}
qt5 {
    QT += network xml widgets webkitwidgets
    VPATH += src/GUI-QT
    exists(libs/qwt/qwt.h) {
        message("with qwt")
        INCLUDEPATH += libs/qwt
        unix {
            LIBS += -lqwt -L$$PWD/libs/qwt
        }
        win32 {
            CONFIG( debug, debug|release ) {
                # debug
                LIBS += -lqwtd
            } else {
                # release
                LIBS += -lqwt
            }
        }
        CONFIG += qwt
    }
    win32 {
        RC_FILE = windows/dream.rc
    }
}
macx {
    INCLUDEPATH += /Developer/dream/include /opt/local/include
    LIBS += -L/Developer/dream/lib -L/opt/local/lib
    LIBS += -framework CoreFoundation -framework CoreServices
    LIBS += -framework CoreAudio -framework AudioToolbox -framework AudioUnit
    UI_DIR = moc
    MOC_DIR = moc
    RC_FILE = src/GUI-QT/res/macicons.icns
    exists(/opt/local/include/portaudio.h) {
        CONFIG += portaudio
    }
    exists(/opt/local/include/sndfile.h) {
        CONFIG += sndfile
    }
}
exists(libs/faac.h) {
    CONFIG += faac
}
exists(libs/neaacdec.h) {
    CONFIG += faad
}
linux-* {
    LIBS -= -ldl -lrt
}
android {
    CONFIG += openSL fftw3
    SOURCES += src/android/platform_util.cpp
    HEADERS += src/android/platform_util.h
    QT -= webkitwidgets
}
unix {
# packagesExist() not available on Qt 4.6 (e.g. Debian Squeeze)
     target.path = /usr/bin
     INSTALLS += target
     CONFIG += link_pkgconfig
     # check for pulseaudio before portaudio
     exists(/usr/include/pulse/pulseaudio.h) | \
     exists(/usr/local/include/pulse/pulseaudio.h) {
     #packagesExist(libpulse) 
      CONFIG += pulseaudio
     }
     else {
       exists(/usr/include/portaudio.h) | \
       exists(/usr/local/include/portaudio.h) {
       #packagesExist(portaudio-2.0) 
          CONFIG += portaudio
       }
     }
     !qtconsole:!console {
      exists(/usr/include/hamlib/rig.h) | \
      exists(/usr/local/include/hamlib/rig.h) {
          CONFIG += hamlib
      }
     }
     exists(/usr/include/gps.h) | \
     exists(/usr/local/include/gps.h) {
      CONFIG += gps
     }
     exists(/usr/include/pcap.h) | \
     exists(/usr/local/include/pcap.h) {
      CONFIG += pcap
     }
     exists(/usr/include/sndfile.h) | \
     exists(/usr/local/include/sndfile.h) {
      CONFIG += sndfile
     }
     exists(/usr/include/opus/opus.h) | \
     exists(/usr/local/include/opus/opus.h) {
      CONFIG += opus
     }
     exists(/usr/include/speex/speex_preprocess.h) | \
     exists(/usr/local/include/speex/speex_preprocess.h) {
      CONFIG += speexdsp
     }
     exists(/usr/include/fftw3.h) | \
     exists(/usr/local/include/fftw3.h) {
      CONFIG += fftw3
     }
     exists(/usr/include/fftw.h) {
       CONFIG += fftw
       LIBS += -lfftw
       exists(/usr/include/rfftw.h):LIBS += -lrfftw
       exists(/opt/local/include/dfftw.h) {
              DEFINES += HAVE_DFFTW_H
              LIBS += -ldfftw
       }
       exists(/opt/local/include/drfftw.h) {
             DEFINES += HAVE_DRFFTW_H
             LIBS += -ldrfftw
       }
       DEFINES += HAVE_FFTW_H HAVE_RFFTW_H
       message("with fftw2")
     }
     tui:console {
      CONFIG += consoleio
     }
     LIBS += -lz
     SOURCES += src/linux/Pacer.cpp
     DEFINES += HAVE_DLFCN_H \
            HAVE_MEMORY_H \
            HAVE_STDINT_H \
            HAVE_STDLIB_H
     DEFINES += HAVE_STRINGS_H \
            HAVE_STRING_H \
            STDC_HEADERS
     DEFINES += HAVE_INTTYPES_H \
            HAVE_STDINT_H \
            HAVE_SYS_STAT_H \
            HAVE_SYS_TYPES_H \
            HAVE_UNISTD_H
     DEFINES += HAVE_LIBZ
     !macx {
      MAKEFILE = Makefile
      UI_DIR = moc
      MOC_DIR = moc
     }
}
msvc2008 {
     TEMPLATE = vcapp
     QMAKE_CXXFLAGS += /wd"4996" /wd"4521"
     QMAKE_LFLAGS_RELEASE += /NODEFAULTLIB:libcmt.lib
     QMAKE_LFLAGS_DEBUG += /NODEFAULTLIB:libcmtd.lib
     LIB += zdll.lib
}
msvc2010 {
     TEMPLATE = vc
     QMAKE_CXXFLAGS += /wd"4996" /wd"4521"
     QMAKE_LFLAGS_RELEASE += /NODEFAULTLIB:libcmt.lib
     QMAKE_LFLAGS_DEBUG += /NODEFAULTLIB:libcmtd.lib
     LIB += zdll.lib
}
win32-g++ {
     DEFINES += HAVE_STDINT_H
     LIBS += -lz
}
win32 {
     exists(libs/portaudio.h) {
      CONFIG += portaudio
     }
     else {
      CONFIG += mmsystem
     }
     exists(libs/fftw3.h) {
      CONFIG += fftw3
     }
     exists(libs/speex/speex_preprocess.h) {
      CONFIG += speexdsp
     }
     exists(libs/fftw.h) {
      DEFINES += HAVE_FFTW_H
      LIBS += -lfftw
	  exists(libs/rfftw.lib) {
		DEFINES += HAVE_RFFTW_H
		LIBS += -lrfftw
      }
      message("with fftw2")
      CONFIG += fftw
     }
     exists(libs/hamlib/rig.h) {
      CONFIG += hamlib
     }
     exists(libs/pcap.h) {
      CONFIG += pcap
     }
     exists(libs/sndfile.h) {
      CONFIG += sndfile
     }
     UI_DIR = moc
     MOC_DIR = moc
     LIBS += -lsetupapi -lwsock32 -lws2_32 -lzdll -ladvapi32 -luser32
     DEFINES += HAVE_SETUPAPI \
     HAVE_LIBZ _CRT_SECURE_NO_WARNINGS
     DEFINES -= UNICODE
     SOURCES += src/windows/Pacer.cpp src/windows/platform_util.cpp
     HEADERS += src/windows/platform_util.h
}
faad {
     DEFINES += HAVE_LIBFAAD \
     USE_FAAD2_LIBRARY
     LIBS += -lfaad_drm
     message("with FAAD2")
}
faac {
     DEFINES += HAVE_LIBFAAC \
     USE_FAAC_LIBRARY
     LIBS += -lfaac_drm
     message("with FAAC")
}
fftw3 {
      DEFINES += HAVE_FFTW3_H
      unix:LIBS += -lfftw3
      win32:LIBS += -lfftw3-3
      android:LIBS += -lfftw3
      CONFIG += fftw
      message("with fftw3")
}
opus {
     DEFINES += HAVE_LIBOPUS \
     USE_OPUS_LIBRARY
     unix:LIBS += -lopus
     message("with opus")
}
sndfile {
     DEFINES += HAVE_LIBSNDFILE
     unix:LIBS += -lsndfile
     win32:LIBS += libsndfile-1.lib
     message("with libsndfile")
}
speexdsp {
     DEFINES += HAVE_SPEEX
     unix:LIBS += -lspeexdsp
     win32:LIBS += libspeexdsp.lib
     message("with libspeexdsp")
}
gps {
     DEFINES += HAVE_LIBGPS
     unix:LIBS += -lgps
     message("with gps")
}
pcap {
     DEFINES += HAVE_LIBPCAP
     unix:LIBS += -lpcap
     win32:LIBS += wpcap.lib packet.lib
     message("with pcap")
}
hamlib {
     DEFINES += HAVE_LIBHAMLIB
     macx:LIBS += -framework IOKit
     unix:LIBS += -lhamlib
     win32:LIBS += libhamlib-2.lib
     HEADERS += src/util/Hamlib.h
     SOURCES += src/util/Hamlib.cpp
     !console {
       HEADERS += src/util-QT/Rig.h
       SOURCES += src/util-QT/Rig.cpp
       !qtconsole {
         HEADERS += src/GUI-QT/RigDlg.h
         SOURCES += src/GUI-QT/RigDlg.cpp
         FORMS += RigDlg.ui
       }
     }
     message("with hamlib")
}
alsa {
    DEFINES += USE_ALSA
    HEADERS += src/linux/soundsrc.h \
    src/linux/soundin.h \
    src/linux/soundout.h
    SOURCES += src/linux/alsa.cpp \
    src/linux/soundsrc.cpp
    CONFIG += sound
    message("with alsa")
}
mmsystem {
    HEADERS += src/windows/Sound.h
    SOURCES += src/windows/Sound.cpp
    LIBS += -lwinmm
    CONFIG += sound
    message("with mmsystem")
}
portaudio {
    DEFINES += USE_PORTAUDIO
    HEADERS += src/sound/pa_ringbuffer.h \
    src/sound/drm_portaudio.h
    SOURCES += src/sound/drm_portaudio.cpp \
    src/sound/pa_ringbuffer.c
    LIBS += -lportaudio
    CONFIG += sound
    unix:PKGCONFIG += portaudio-2.0
    message("with portaudio")
}
pulseaudio {
    DEFINES += USE_PULSEAUDIO
    HEADERS += src/sound/drm_pulseaudio.h
    SOURCES += src/sound/drm_pulseaudio.cpp
    LIBS += -lpulse
    CONFIG += sound
    unix:PKGCONFIG += libpulse
    message("with pulseaudio")
}
openSL {
    CONFIG += sound
    message("with openSL")
}
consoleio {
    DEFINES += USE_CONSOLEIO
    HEADERS += src/linux/ConsoleIO.h
    SOURCES += src/linux/ConsoleIO.cpp
    message("with terminal user interface")
}
HEADERS += \
    src/AMDemodulation.h \
    src/AMSSDemodulation.h \
    src/chanest/ChanEstTime.h \
    src/chanest/ChannelEstimation.h \
    src/chanest/IdealChannelEstimation.h \
    src/chanest/TimeLinear.h \
    src/chanest/TimeWiener.h \
    src/datadecoding/DABMOT.h \
    src/datadecoding/DataDecoder.h \
    src/datadecoding/DataEncoder.h \
    src/datadecoding/epgutil.h \
    src/datadecoding/Experiment.h \
    src/datadecoding/journaline/cpplog.h \
    src/datadecoding/journaline/crc_8_16.h \
    src/datadecoding/journaline/dabdatagroupdecoder.h \
    src/datadecoding/journaline/dabdgdec_impl.h \
    src/datadecoding/Journaline.h \
    src/datadecoding/journaline/log.h \
    src/datadecoding/journaline/newsobject.h \
    src/datadecoding/journaline/newssvcdec.h \
    src/datadecoding/journaline/newssvcdec_impl.h \
    src/datadecoding/journaline/NML.h \
    src/datadecoding/journaline/Splitter.h \
    src/datadecoding/MOTSlideShow.h \
    src/DataIO.h \
    src/drmchannel/ChannelSimulation.h \
    src/DrmReceiver.h \
    src/DRMSignalIO.h \
    src/DrmSimulation.h \
    src/DrmTransceiver.h \
    src/DrmTransmitter.h \
    src/FAC/FAC.h \
    src/GlobalDefinitions.h \
    src/InputResample.h \
    src/interleaver/BlockInterleaver.h \
    src/interleaver/SymbolInterleaver.h \
    src/IQInputFilter.h \
    src/matlib/Matlib.h \
    src/matlib/MatlibSigProToolbox.h \
    src/matlib/MatlibStdToolbox.h \
    src/MDI/AFPacketGenerator.h \
    src/MDI/MDIDecode.h \
    src/MDI/MDIDefinitions.h \
    src/MDI/MDIInBuffer.h \
    src/MDI/MDIRSCI.h \
    src/MDI/MDITagItemDecoders.h \
    src/MDI/MDITagItems.h \
    src/MDI/PacketInOut.h \
    src/MDI/PacketSinkFile.h \
    src/MDI/PacketSocket.h \
    src/MDI/PacketSourceFile.h \
    src/MDI/Pft.h \
    src/MDI/RCITagItems.h \
    src/MDI/RSCITagItemDecoders.h \
    src/MDI/RSISubscriber.h \
    src/MDI/TagItemDecoder.h \
    src/MDI/TagPacketDecoder.h \
    src/MDI/TagPacketDecoderMDI.h \
    src/MDI/TagPacketDecoderRSCIControl.h \
    src/MDI/TagPacketGenerator.h \
    src/mlc/BitInterleaver.h \
    src/mlc/ChannelCode.h \
    src/mlc/ConvEncoder.h \
    src/mlc/EnergyDispersal.h \
    src/mlc/Metric.h \
    src/mlc/MLC.h \
    src/mlc/QAMMapping.h \
    src/mlc/ViterbiDecoder.h \
    src/MSCMultiplexer.h \
    src/ofdmcellmapping/CellMappingTable.h \
    src/ofdmcellmapping/OFDMCellMapping.h \
    src/OFDM.h \
    src/Parameter.h \
    src/PlotManager.h \
    src/ReceptLog.h \
    src/resample/ResampleFilter.h \
    src/resample/Resample.h \
    src/Scheduler.h \
    src/SDC/SDC.h \
    src/ServiceInformation.h \
    src/sound/audiofilein.h \
    src/sound/selectioninterface.h \
    src/sound/sound.h \
    src/sound/soundinterface.h \
    src/sound/soundnull.h \
    src/sourcedecoders/aac_codec.h \
    src/sourcedecoders/AudioCodec.h \
    src/sourcedecoders/AudioSourceDecoder.h \
    src/sourcedecoders/AudioSourceEncoder.h \
    src/sourcedecoders/null_codec.h \
    src/sourcedecoders/opus_codec.h \
    src/sync/FreqSyncAcq.h \
    src/sync/SyncUsingPil.h \
    src/sync/TimeSyncFilter.h \
    src/sync/TimeSync.h \
    src/sync/TimeSyncTrack.h \
    src/tables/TableAMSS.h \
    src/tables/TableCarMap.h \
    src/tables/TableDRMGlobal.h \
    src/tables/TableFAC.h \
    src/tables/TableMLC.h \
    src/tables/TableQAMMapping.h \
    src/tables/TableStations.h \
    src/TextMessage.h \
    src/UpsampleFilter.h \
    src/util/AudioFile.h \
    src/util/Buffer.h \
    src/util/CRC.h \
    src/util/LibraryLoader.h \
    src/util/LogPrint.h \
    src/util/Modul.h \
    src/util/Pacer.h \
    src/util/Reassemble.h \
    src/util/Settings.h \
    src/util/Utilities.h \
    src/util/Vector.h \
    src/Version.h
SOURCES += \
    src/AMDemodulation.cpp \
    src/AMSSDemodulation.cpp \
    src/chanest/ChanEstTime.cpp \
    src/chanest/ChannelEstimation.cpp \
    src/chanest/IdealChannelEstimation.cpp \
    src/chanest/TimeLinear.cpp \
    src/chanest/TimeWiener.cpp \
    src/datadecoding/DABMOT.cpp \
    src/datadecoding/DataDecoder.cpp \
    src/datadecoding/DataEncoder.cpp \
    src/datadecoding/epgutil.cpp \
    src/datadecoding/Experiment.cpp \
    src/datadecoding/Journaline.cpp \
    src/datadecoding/journaline/crc_8_16.c \
    src/datadecoding/journaline/dabdgdec_impl.c \
    src/datadecoding/journaline/log.c \
    src/datadecoding/journaline/newsobject.cpp \
    src/datadecoding/journaline/newssvcdec_impl.cpp \
    src/datadecoding/journaline/NML.cpp \
    src/datadecoding/journaline/Splitter.cpp \
    src/datadecoding/MOTSlideShow.cpp \
    src/DataIO.cpp \
    src/drmchannel/ChannelSimulation.cpp \
    src/DrmReceiver.cpp \
    src/DRMSignalIO.cpp \
    src/DrmSimulation.cpp \
    src/DrmTransmitter.cpp \
    src/FAC/FAC.cpp \
    src/GUI-QT/main.cpp \
    src/InputResample.cpp \
    src/interleaver/BlockInterleaver.cpp \
    src/interleaver/SymbolInterleaver.cpp \
    src/IQInputFilter.cpp \
    src/matlib/MatlibSigProToolbox.cpp \
    src/matlib/MatlibStdToolbox.cpp \
    src/MDI/AFPacketGenerator.cpp \
    src/MDI/MDIDecode.cpp \
    src/MDI/MDIInBuffer.cpp \
    src/MDI/MDIRSCI.cpp \
    src/MDI/MDITagItemDecoders.cpp \
    src/MDI/MDITagItems.cpp \
    src/MDI/PacketSinkFile.cpp \
    src/MDI/PacketSocket.cpp \
    src/MDI/PacketSourceFile.cpp \
    src/MDI/Pft.cpp \
    src/MDI/RCITagItems.cpp \
    src/MDI/RSCITagItemDecoders.cpp \
    src/MDI/RSISubscriber.cpp \
    src/MDI/TagPacketDecoder.cpp \
    src/MDI/TagPacketDecoderMDI.cpp \
    src/MDI/TagPacketDecoderRSCIControl.cpp \
    src/MDI/TagPacketGenerator.cpp \
    src/mlc/BitInterleaver.cpp \
    src/mlc/ChannelCode.cpp \
    src/mlc/ConvEncoder.cpp \
    src/mlc/EnergyDispersal.cpp \
    src/mlc/Metric.cpp \
    src/mlc/MLC.cpp \
    src/mlc/QAMMapping.cpp \
    src/mlc/TrellisUpdateMMX.cpp \
    src/mlc/TrellisUpdateSSE2.cpp \
    src/mlc/ViterbiDecoder.cpp \
    src/MSCMultiplexer.cpp \
    src/ofdmcellmapping/CellMappingTable.cpp \
    src/ofdmcellmapping/OFDMCellMapping.cpp \
    src/OFDM.cpp \
    src/Parameter.cpp \
    src/PlotManager.cpp \
    src/ReceptLog.cpp \
    src/resample/Resample.cpp \
    src/resample/ResampleFilter.cpp \
    src/Scheduler.cpp \
    src/SDC/SDCReceive.cpp \
    src/SDC/SDCTransmit.cpp \
    src/ServiceInformation.cpp \
    src/SimulationParameters.cpp \
    src/sound/audiofilein.cpp \
    src/sourcedecoders/aac_codec.cpp \
    src/sourcedecoders/AudioCodec.cpp \
    src/sourcedecoders/AudioSourceDecoder.cpp \
    src/sourcedecoders/AudioSourceEncoder.cpp \
    src/sourcedecoders/null_codec.cpp \
    src/sourcedecoders/opus_codec.cpp \
    src/sync/FreqSyncAcq.cpp \
    src/sync/SyncUsingPil.cpp \
    src/sync/TimeSync.cpp \
    src/sync/TimeSyncFilter.cpp \
    src/sync/TimeSyncTrack.cpp \
    src/tables/TableCarMap.cpp \
    src/tables/TableFAC.cpp \
    src/tables/TableStations.cpp \
    src/TextMessage.cpp \
    src/util/CRC.cpp \
    src/util/LogPrint.cpp \
    src/util/Reassemble.cpp \
    src/util/Settings.cpp \
    src/util/Utilities.cpp \
    src/Version.cpp
!console {
HEADERS += \
    src/GUI-QT/Logging.h \
    src/util-QT/epgdec.h \
    src/util-QT/EPG.h \
    src/util-QT/Util.h
SOURCES += \
    src/GUI-QT/Logging.cpp \
    src/util-QT/EPG.cpp \
    src/util-QT/epgdec.cpp \
    src/util-QT/Util.cpp
}
!console:!qtconsole {
    contains(QT, webkitwidgets)|contains(QT,webkit) {
        FORMS += BWSViewer.ui
        HEADERS += src/GUI-QT/BWSViewer.h
        SOURCES += src/GUI-QT/BWSViewer.cpp
    }
FORMS += \
    AboutDlgbase.ui \
    AMMainWindow.ui \
    AMSSDlgbase.ui \
    CodecParams.ui \
    DRMMainWindow.ui \
    EPGDlgbase.ui \
    FMMainWindow.ui \
    GeneralSettingsDlgbase.ui \
    JLViewer.ui \
    LiveScheduleWindow.ui \
    MultSettingsDlgbase.ui \
    SlideShowViewer.ui \
    StationsDlgbase.ui \
    systemevalDlgbase.ui \
    TransmDlgbase.ui
HEADERS += \
    src/GUI-QT/AnalogDemDlg.h \
    src/GUI-QT/CodecParams.h \
    src/GUI-QT/CWindow.h \
    src/GUI-QT/DialogUtil.h \
    src/GUI-QT/DRMPlot.h \
    src/GUI-QT/EPGDlg.h \
    src/GUI-QT/EvaluationDlg.h \
    src/GUI-QT/fdrmdialog.h \
    src/GUI-QT/fmdialog.h \
    src/GUI-QT/GeneralSettingsDlg.h \
    src/GUI-QT/jlbrowser.h \
    src/GUI-QT/JLViewer.h \
    src/GUI-QT/LiveScheduleDlg.h \
    src/GUI-QT/MultColorLED.h \
    src/GUI-QT/MultSettingsDlg.h \
    src/GUI-QT/Schedule.h \
    src/GUI-QT/SlideShowViewer.h \
    src/GUI-QT/SoundCardSelMenu.h \
    src/GUI-QT/StationsDlg.h \
    src/GUI-QT/TransmDlg.h
SOURCES += \
    src/GUI-QT/AnalogDemDlg.cpp \
    src/GUI-QT/CodecParams.cpp \
    src/GUI-QT/CWindow.cpp \
    src/GUI-QT/DialogUtil.cpp \
    src/GUI-QT/DRMPlot.cpp \
    src/GUI-QT/EPGDlg.cpp \
    src/GUI-QT/EvaluationDlg.cpp \
    src/GUI-QT/fdrmdialog.cpp \
    src/GUI-QT/fmdialog.cpp \
    src/GUI-QT/GeneralSettingsDlg.cpp \
    src/GUI-QT/jlbrowser.cpp \
    src/GUI-QT/JLViewer.cpp \
    src/GUI-QT/LiveScheduleDlg.cpp \
    src/GUI-QT/MultColorLED.cpp \
    src/GUI-QT/MultSettingsDlg.cpp \
    src/GUI-QT/Schedule.cpp \
    src/GUI-QT/SlideShowViewer.cpp \
    src/GUI-QT/SoundCardSelMenu.cpp \
    src/GUI-QT/StationsDlg.cpp \
    src/GUI-QT/TransmDlg.cpp
}
!sound {
    error("no usable audio interface found - install pulseaudio or portaudio dev package")
}
!fftw {
    error("no usable fftw library found - install fftw dev package")
}
!qwt:!console:!qtconsole {
    error("no usable qwt library found - install qwt dev package")
}

OTHER_FILES += \
    android/AndroidManifest.xml \
    android/res/layout/splash.xml \
    android/res/values/libs.xml \
    android/res/values/strings.xml \
    android/res/values-de/strings.xml \
    android/res/values-el/strings.xml \
    android/res/values-es/strings.xml \
    android/res/values-et/strings.xml \
    android/res/values-fa/strings.xml \
    android/res/values-fr/strings.xml \
    android/res/values-id/strings.xml \
    android/res/values-it/strings.xml \
    android/res/values-ja/strings.xml \
    android/res/values-ms/strings.xml \
    android/res/values-nb/strings.xml \
    android/res/values-nl/strings.xml \
    android/res/values-pl/strings.xml \
    android/res/values-pt-rBR/strings.xml \
    android/res/values-ro/strings.xml \
    android/res/values-rs/strings.xml \
    android/res/values-ru/strings.xml \
    android/res/values-zh-rCN/strings.xml \
    android/res/values-zh-rTW/strings.xml \
    android/src/org/kde/necessitas/ministro/IMinistro.aidl \
    android/src/org/kde/necessitas/ministro/IMinistroCallback.aidl \
    android/src/org/qtproject/qt5/android/bindings/QtActivity.java \
    android/src/org/qtproject/qt5/android/bindings/QtApplication.java \
    android/version.xml
