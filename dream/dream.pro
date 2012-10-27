console {
    message("console mode")
    QT -= gui
    DEFINES -= USE_QT_GUI
}
else {
    DEFINES += USE_QT_GUI
    RESOURCES = common/GUI-QT/res/icons.qrc
}
TEMPLATE = app
TARGET = dream
CONFIG += qt warn_on debug thread qt3support
INCLUDEPATH += common/GUI-QT
INCLUDEPATH += libs
#LIBS += -L$$PWD/libs
macx:QMAKE_LFLAGS += -F$$PWD/libs
contains(QT_VERSION, ^4\\..*) {
    message("Qt 4")
    QT += network xml qt3support
    VPATH += common/GUI-QT
    !console {
        HEADERS += common/GUI-QT/DRMPlot-qwt6.h common/GUI-QT/EvaluationDlg.h
        HEADERS += common/GUI-QT/SlideShowViewer.h common/GUI-QT/JLViewer.h common/GUI-QT/BWSViewer.h
	HEADERS += common/GUI-QT/bwsbrowser.h common/GUI-QT/jlbrowser.h
        SOURCES += common/GUI-QT/DRMPlot-qwt6.cpp common/GUI-QT/EvaluationDlg.cpp
        SOURCES += common/GUI-QT/SlideShowViewer.cpp common/GUI-QT/JLViewer.cpp common/GUI-QT/BWSViewer.cpp
	SOURCES += common/GUI-QT/bwsbrowser.cpp common/GUI-QT/jlbrowser.cpp
        HEADERS += common/GUI-QT/CSoundCardSelMenu.h
        SOURCES += common/GUI-QT/CSoundCardSelMenu.cpp
        FORMS += DRMMainWindow.ui FMMainWindow.ui AMMainWindow.ui LiveScheduleWindow.ui
        FORMS += JLViewer.ui BWSViewer.ui SlideShowViewer.ui
        unix {
	  macx {
            exists(libs/qwt.framework) {
	      message("with qwt6")
              INCLUDEPATH += libs/qwt
              LIBS += -framework qwt
            }
            else {
              error("no usable qwt version 6 found")
            }
          }
          else {
            exists($$(QWT)) {
	      message("with qwt6")
              INCLUDEPATH += $$(QWT)/include
              LIBS += $$(QWT)/lib/libqwt.so
            }
            else {
              exists(/usr/lib/libqwt.so.6) {
	        message("with qwt6")
                LIBS += /usr/lib/libqwt.so.6
                exists(/usr/include/qwt6) {
                  INCLUDEPATH += /usr/include/qwt6
                }
                else {
                  exists(libs/qwt) {
                    INCLUDEPATH += libs/qwt
                  } else {
                    INCLUDEPATH += /usr/include/qwt
                  }
                }
              }
              else {
                error("no usable qwt version 6 found")
              }
            }
          }
        }
        win32 {
            exists($$(QWT)) {
	      message("with qwt6")
              INCLUDEPATH += $$(QWT)/include
              LIBS += $$(QWT)/lib/libqwt.so
            }
            else {
              exists(libs/qwt) {
                INCLUDEPATH += libs/qwt
                CONFIG( debug, debug|release ) {
                    # debug
                    LIBS += -lqwtd
                } else {
                    # release
                    LIBS += -lqwt
                }
              }
              else {
                error("no usable qwt version found")
              }
            }
        }
    }
}
count(QT_VERSION, 0) {
    message("Qt 3")
    CONFIG += old
    VPATH += common/GUI-QT/qt2
    !console {
        HEADERS += common/GUI-QT/DRMPlot.h common/GUI-QT/systemevalDlg.h common/GUI-QT/MultimediaDlg.h
        SOURCES += common/GUI-QT/DRMPlot.cpp common/GUI-QT/systemevalDlg.cpp common/GUI-QT/MultimediaDlg.cpp
        FORMS += fdrmdialogbase.ui fmdialogbase.ui AnalogDemDlgbase.ui LiveScheduleDlgbase.ui
        FORMS += MultimediaDlgbase.ui
        unix {
              exists(/usr/lib/libqwt.so.4) {
	        message("with qwt4")
                INCLUDEPATH += /usr/include/qwt
                LIBS += /usr/lib/libqwt.so.4
              }
              else {
                error("no usable qwt version found")
              }
        }
        win32 {
            INCLUDEPATH += libs/qwt
        }
    }
}
!console {
    FORMS += TransmDlgbase.ui
    FORMS += AMSSDlgbase.ui \
    systemevalDlgbase.ui \
    StationsDlgbase.ui \
    EPGDlgbase.ui
    FORMS += GeneralSettingsDlgbase.ui \
    MultSettingsDlgbase.ui \
    AboutDlgbase.ui
}
macx {
    OBJECTS_DIR = darwin
    INCLUDEPATH += darwin
    INCLUDEPATH += /Developer/dream/include /opt/local/include
    LIBS += -L/Developer/dream/lib -L/opt/local/lib
    LIBS += -framework CoreFoundation -framework CoreServices
    LIBS += -framework CoreAudio -framework AudioToolbox -framework AudioUnit
    UI_DIR = darwin/moc
    MOC_DIR = darwin/moc
    RC_FILE = common/GUI-QT/res/macicons.icns
}
exists(libs/faac.h) {
    CONFIG += faac
              message("with FAAC")
          }
exists(libs/neaacdec.h) {
    CONFIG += faad
              message("with FAAD2")
          }
unix {
    target.path = /usr/bin
    INSTALLS += target
    CONFIG += link_pkgconfig
    #packagesExist(portaudio-2.0) {
      CONFIG += portaudio
      PKGCONFIG += portaudio-2.0
    exists(/usr/include/hamlib/rig.h) {
        CONFIG += hamlib
                  message("with hamlib")
              }
    exists(/usr/local/include/hamlib/rig.h) {
        CONFIG += hamlib
                  message("with hamlib")
              }
    exists(/usr/include/gps.h) {
        CONFIG += gps
                  message("with gps")
              }
    exists(/usr/include/pcap.h) {
        CONFIG += pcap
                  message("with pcap")
              }
    exists(/usr/include/sndfile.h) {
        CONFIG += sndfile
                  message("with libsndfile")
              }
    exists(/opt/local/include/sndfile.h) {
        CONFIG += sndfile
                  message("with libsndfile")
              }
    exists(/usr/include/fftw3.h) {
        DEFINES += HAVE_FFTW3_H
                   LIBS += -lfftw3
                           message("with fftw3")
                       }
    else {
      exists(/usr/include/fftw.h) {
	message("with fftw2")
        LIBS += -lfftw
        exists(/usr/include/rfftw.h):LIBS += -lrfftw
        DEFINES += HAVE_FFTW_H HAVE_RFFTW_H
      }
      else {
        exists(/opt/local/include/dfftw.h) {
	  message("with fftw2")
          DEFINES += HAVE_DFFTW_H
          LIBS += -ldfftw
          exists(/opt/local/include/drfftw.h) {
           DEFINES += HAVE_DRFFTW_H
           LIBS += -ldrfftw
          }
        }
        else {
          error("no usable fftw library found - install fftw dev package")
        }
      }
    }
    LIBS += -lz \
            -ldl
    SOURCES += linux/source/Pacer.cpp
    HEADERS += linux/source/shmsoundin.h \
        linux/source/pa_shm_ringbuffer.h
    SOURCES += linux/source/shmsoundin.cpp \
	  linux/source/pa_shm_ringbuffer.c
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
        INCLUDEPATH += linux
        LIBS += -lrt
        OBJECTS_DIR = linux
        UI_DIR = linux/moc
        UI_DIR = linux/moc
        MOC_DIR = linux/moc
    }
}
msvc2008 {
    TEMPLATE = vcapp
    QMAKE_CXXFLAGS += /wd"4996" /wd"4521"
    QMAKE_LFLAGS_RELEASE += /NODEFAULTLIB:libcmt.lib
    QMAKE_LFLAGS_DEBUG += /NODEFAULTLIB:libcmtd.lib
    LIB += zlib.lib
}
msvc2010 {
    TEMPLATE = vc
    QMAKE_CXXFLAGS += /wd"4996" /wd"4521"
    QMAKE_LFLAGS_RELEASE += /NODEFAULTLIB:libcmt.lib
    QMAKE_LFLAGS_DEBUG += /NODEFAULTLIB:libcmtd.lib
    LIB += zlib.lib
}
win32-g++ {
    DEFINES += HAVE_STDINT_H
    LIBS += -lz
}
win32 {
    exists(libs/portaudio.h) {
        CONFIG += portaudio
        message("with portaudio")
    }
    exists(libs/fftw3.h) {
        DEFINES += HAVE_FFTW3_H
        LIBS += -lfftw3-3
        message("with fftw3")
    }
    else {
        exists(libs/fftw.h) {
            DEFINES += HAVE_FFTW_H
            LIBS += -lfftw
			exists(libs/rfftw.lib) {
				DEFINES += HAVE_RFFTW_H
				LIBS += -lrfftw
			}
        message("with fftw2")
        }
        else {
            error("no usable fftw version 2 or 3 found")
        }
    }
    exists(libs/hamlib/rig.h) {
        CONFIG += hamlib
        message("with hamlib")
    }
    exists(libs/pcap.h) {
        CONFIG += pcap
        message("with pcap")
    }
    exists(libs/sndfile.h) {
        CONFIG += sndfile
        message("with libsndfile")
    }
    OBJECTS_DIR = windows
    UI_DIR = windows/moc
    MOC_DIR = windows/moc
    LIBS += -lsetupapi \
    -lwinmm \
    -lwsock32
    DEFINES += HAVE_SETUPAPI \
    HAVE_LIBZ
    DEFINES -= UNICODE
    HEADERS += windows/Source/Sound.h windows/Source/SoundWin.h
    SOURCES += windows/Source/Pacer.cpp windows/Source/Sound.cpp
}
faad {
    DEFINES += HAVE_LIBFAAD \
    USE_FAAD2_LIBRARY
    LIBS += -lfaad_drm
}
faac {
    DEFINES += HAVE_LIBFAAC \
    USE_FAAC_LIBRARY
    LIBS += -lfaac_drm
}
sndfile {
    DEFINES += HAVE_LIBSNDFILE
    unix:LIBS += -lsndfile
    win32:LIBS += libsndfile-1.lib
}
gps {
    DEFINES += HAVE_LIBGPS
    unix:LIBS += -lgps
}
pcap {
    DEFINES += HAVE_LIBPCAP
    unix:LIBS += -lpcap
    win32:LIBS += wpcap.lib packet.lib
}
hamlib {
    DEFINES += HAVE_LIBHAMLIB \
    HAVE_RIG_PARSE_MODE
    macx:LIBS += -framework IOKit
    unix:LIBS += -lhamlib
    win32:LIBS += libhamlib-2.lib
    !console:!old {
        HEADERS += common/GUI-QT/RigDlg.h
        SOURCES += common/GUI-QT/RigDlg.cpp
        FORMS += RigDlg.ui
    }
}
alsa {
    DEFINES += USE_ALSA
    HEADERS += linux/source/soundcommon.h \
    linux/source/soundin.h \
    linux/source/soundout.h
    SOURCES += linux/source/alsa.cpp \
    linux/source/soundcommon.cpp
}
portaudio {
    DEFINES += USE_PORTAUDIO
    HEADERS += common/sound/pa_ringbuffer.h \
    common/sound/drm_portaudio.h
    SOURCES += common/sound/drm_portaudio.cpp \
    common/sound/pa_ringbuffer.c
    LIBS += -lportaudio
}
HEADERS += common/AMDemodulation.h \
   common/AMSSDemodulation.h \
   common/audiofilein.h \
   common/chanest/ChanEstTime.h \
   common/chanest/ChannelEstimation.h \
   common/chanest/IdealChannelEstimation.h \
   common/chanest/TimeLinear.h \
   common/chanest/TimeWiener.h \
   common/datadecoding/DABMOT.h \
   common/datadecoding/DataDecoder.h \
   common/datadecoding/DataEncoder.h \
   common/datadecoding/epg/EPG.h \
   common/datadecoding/epg/epgdec.h \
   common/datadecoding/epg/epgutil.h \
   common/datadecoding/journaline/NML.h \
   common/datadecoding/journaline/Splitter.h \
   common/datadecoding/journaline/cpplog.h \
   common/datadecoding/journaline/crc_8_16.h \
   common/datadecoding/journaline/dabdatagroupdecoder.h \
   common/datadecoding/journaline/dabdgdec_impl.h \
   common/datadecoding/journaline/log.h \
   common/datadecoding/journaline/newsobject.h \
   common/datadecoding/journaline/newssvcdec.h \
   common/datadecoding/journaline/newssvcdec_impl.h \
   common/datadecoding/Experiment.h \
   common/datadecoding/Journaline.h \
   common/datadecoding/MOTSlideShow.h \
   common/DataIO.h \
   common/drmchannel/ChannelSimulation.h \
   common/ReceptLog.h \
   common/PlotManager.h \
   common/ServiceInformation.h \
   common/DrmReceiver.h \
   common/DRMSignalIO.h \
   common/DrmSimulation.h \
   common/DrmTransmitter.h \
   common/FAC/FAC.h \
   common/GlobalDefinitions.h \
   common/InputResample.h \
   common/interleaver/BlockInterleaver.h \
   common/interleaver/SymbolInterleaver.h \
   common/IQInputFilter.h \
   common/matlib/Matlib.h \
   common/matlib/MatlibSigProToolbox.h \
   common/matlib/MatlibStdToolbox.h \
   common/MDI/AFPacketGenerator.h \
   common/MDI/MDIDecode.h \
   common/MDI/MDIDefinitions.h \
   common/MDI/MDIInBuffer.h \
   common/MDI/MDIRSCI.h \
   common/MDI/MDITagItemDecoders.h \
   common/MDI/MDITagItems.h \
   common/MDI/PacketInOut.h \
   common/MDI/PacketSinkFile.h \
   common/MDI/PacketSourceFile.h \
   common/MDI/PacketSocketNull.h \
   common/MDI/PacketSocketQT.h \
   common/MDI/Pft.h \
   common/MDI/RCITagItems.h \
   common/MDI/RSCITagItemDecoders.h \
   common/MDI/RSISubscriber.h \
   common/MDI/TagItemDecoder.h \
   common/MDI/TagPacketDecoder.h \
   common/MDI/TagPacketDecoderMDI.h \
   common/MDI/TagPacketDecoderRSCIControl.h \
   common/MDI/TagPacketGenerator.h \
   common/mlc/BitInterleaver.h \
   common/mlc/ChannelCode.h \
   common/mlc/ConvEncoder.h \
   common/mlc/EnergyDispersal.h \
   common/mlc/Metric.h \
   common/mlc/MLC.h \
   common/mlc/QAMMapping.h \
   common/mlc/ViterbiDecoder.h \
   common/MSCMultiplexer.h \
   common/OFDM.h \
   common/ofdmcellmapping/CellMappingTable.h \
   common/ofdmcellmapping/OFDMCellMapping.h \
   common/Parameter.h \
   common/resample/Resample.h \
   common/resample/ResampleFilter.h \
   common/SDC/SDC.h \
   common/selectioninterface.h \
   common/soundinterface.h \
   common/sound.h \
   common/sound/soundnull.h \
   common/sourcedecoders/AudioSourceDecoder.h \
   common/sourcedecoders/AudioSourceEncoder.h \
   common/sync/FreqSyncAcq.h \
   common/sync/SyncUsingPil.h \
   common/sync/TimeSync.h \
   common/sync/TimeSyncFilter.h \
   common/sync/TimeSyncTrack.h \
   common/tables/TableAMSS.h \
   common/tables/TableCarMap.h \
   common/tables/TableDRMGlobal.h \
   common/tables/TableFAC.h \
   common/tables/TableMLC.h \
   common/tables/TableQAMMapping.h \
   common/tables/TableStations.h \
   common/TextMessage.h \
   common/util/AudioFile.h \
   common/util/Buffer.h \
   common/util/CRC.h \
   common/util/LogPrint.h \
   common/util/Modul.h \
   common/util/Pacer.h \
   common/util/Reassemble.h \
   common/util/Settings.h \
   common/util/Utilities.h \
   common/util/Vector.h \
   common/GUI-QT/Rig.h \
   common/GUI-QT/Logging.h \
   common/Version.h
SOURCES += common/AMDemodulation.cpp \
      common/AMSSDemodulation.cpp \
      common/audiofilein.cpp \
      common/chanest/ChanEstTime.cpp \
      common/chanest/ChannelEstimation.cpp \
      common/chanest/IdealChannelEstimation.cpp \
      common/chanest/TimeLinear.cpp \
      common/chanest/TimeWiener.cpp \
      common/datadecoding/DABMOT.cpp \
      common/datadecoding/DataDecoder.cpp \
      common/datadecoding/DataEncoder.cpp \
      common/datadecoding/epg/EPG.cpp \
      common/datadecoding/epg/epgdec.cpp \
      common/datadecoding/epg/epgutil.cpp \
      common/datadecoding/journaline/NML.cpp \
      common/datadecoding/journaline/dabdgdec_impl.c \
      common/datadecoding/journaline/Splitter.cpp \
      common/datadecoding/journaline/newsobject.cpp \
      common/datadecoding/journaline/newssvcdec_impl.cpp \
      common/datadecoding/journaline/crc_8_16.c \
      common/datadecoding/journaline/log.c \
      common/datadecoding/Experiment.cpp \
      common/datadecoding/Journaline.cpp \
      common/datadecoding/MOTSlideShow.cpp \
      common/DataIO.cpp \
      common/drmchannel/ChannelSimulation.cpp \
      common/ReceptLog.cpp \
      common/PlotManager.cpp \
      common/ServiceInformation.cpp \
      common/DrmReceiver.cpp \
      common/DRMSignalIO.cpp \
      common/DrmSimulation.cpp \
      common/DrmTransmitter.cpp \
      common/FAC/FAC.cpp \
      common/InputResample.cpp \
      common/interleaver/BlockInterleaver.cpp \
      common/interleaver/SymbolInterleaver.cpp \
      common/IQInputFilter.cpp \
      common/matlib/MatlibSigProToolbox.cpp \
      common/matlib/MatlibStdToolbox.cpp \
      common/MDI/AFPacketGenerator.cpp \
      common/MDI/MDIDecode.cpp \
      common/MDI/MDIInBuffer.cpp \
      common/MDI/MDIRSCI.cpp \
      common/MDI/MDITagItemDecoders.cpp \
      common/MDI/MDITagItems.cpp \
      common/MDI/PacketSinkFile.cpp \
      common/MDI/PacketSourceFile.cpp \
      common/MDI/PacketSocketNull.cpp \
      common/MDI/PacketSocketQT.cpp \
      common/MDI/Pft.cpp \
      common/MDI/RCITagItems.cpp \
      common/MDI/RSCITagItemDecoders.cpp \
      common/MDI/RSISubscriber.cpp \
      common/MDI/TagPacketDecoder.cpp \
      common/MDI/TagPacketDecoderMDI.cpp \
      common/MDI/TagPacketDecoderRSCIControl.cpp \
      common/MDI/TagPacketGenerator.cpp \
      common/mlc/BitInterleaver.cpp \
      common/mlc/ChannelCode.cpp \
      common/mlc/ConvEncoder.cpp \
      common/mlc/EnergyDispersal.cpp \
      common/mlc/Metric.cpp \
      common/mlc/MLC.cpp \
      common/mlc/QAMMapping.cpp \
      common/mlc/TrellisUpdateMMX.cpp \
      common/mlc/TrellisUpdateSSE2.cpp \
      common/mlc/ViterbiDecoder.cpp \
      common/MSCMultiplexer.cpp \
      common/OFDM.cpp \
      common/ofdmcellmapping/CellMappingTable.cpp \
      common/ofdmcellmapping/OFDMCellMapping.cpp \
      common/Parameter.cpp \
      common/resample/Resample.cpp \
      common/resample/ResampleFilter.cpp \
      common/SDC/SDCReceive.cpp \
      common/SDC/SDCTransmit.cpp \
      common/SimulationParameters.cpp \
      common/sourcedecoders/AudioSourceDecoder.cpp \
      common/sourcedecoders/AudioSourceEncoder.cpp \
      common/sync/FreqSyncAcq.cpp \
      common/sync/SyncUsingPil.cpp \
      common/sync/TimeSync.cpp \
      common/sync/TimeSyncFilter.cpp \
      common/sync/TimeSyncTrack.cpp \
      common/tables/TableCarMap.cpp \
      common/tables/TableFAC.cpp \
      common/tables/TableStations.cpp \
      common/TextMessage.cpp \
      common/util/CRC.cpp \
      common/util/LogPrint.cpp \
      common/util/Reassemble.cpp \
      common/util/Settings.cpp \
      common/util/Utilities.cpp \
      common/Version.cpp \
      common/GUI-QT/Rig.cpp \
      common/GUI-QT/Logging.cpp \
      common/GUI-QT/main.cpp
!console {
    HEADERS += common/GUI-QT/AnalogDemDlg.h \
    common/GUI-QT/DialogUtil.h \
    common/GUI-QT/EPGDlg.h \
    common/GUI-QT/fdrmdialog.h \
    common/GUI-QT/fmdialog.h \
    common/GUI-QT/GeneralSettingsDlg.h \
    common/GUI-QT/LiveScheduleDlg.h \
    common/GUI-QT/MultColorLED.h \
    common/GUI-QT/MultSettingsDlg.h \
    common/GUI-QT/StationsDlg.h \
    common/GUI-QT/TransmDlg.h
    SOURCES += common/GUI-QT/AnalogDemDlg.cpp \
    common/GUI-QT/DialogUtil.cpp \
    common/GUI-QT/EPGDlg.cpp \
    common/GUI-QT/fmdialog.cpp \
    common/GUI-QT/fdrmdialog.cpp \
    common/GUI-QT/GeneralSettingsDlg.cpp \
    common/GUI-QT/LiveScheduleDlg.cpp \
    common/GUI-QT/MultColorLED.cpp \
    common/GUI-QT/MultSettingsDlg.cpp \
    common/GUI-QT/StationsDlg.cpp \
    common/GUI-QT/TransmDlg.cpp
}
