TEMPLATE = app
TARGET = dream
INCLUDEPATH += libs
LIBS += -Llibs
OBJECTS_DIR = obj
DEFINES += EXECUTABLE_NAME=$$TARGET
macx:QMAKE_LFLAGS += -F$$PWD/libs
console {
	QT -= core gui
    DEFINES += USE_NO_QT
    DEFINES -= USE_QT_GUI
    UI_MESSAGE = console mode
    VERSION_MESSAGE=No Qt
}
qtconsole {
	QT -= gui
	QT += network xml
	DEFINES -= USE_QT_GUI
	UI_MESSAGE = console mode
	INCLUDEPATH += moc
}
contains(QT, gui) {
	QT += network xml widgets
	VPATH += src/GUI-QT
	INCLUDEPATH += libs/qwt
	INCLUDEPATH += src/GUI-QT
	INCLUDEPATH += moc
	DEFINES += USE_QT_GUI
	RESOURCES = src/GUI-QT/res/icons.qrc
	UI_MESSAGE = GUI mode
	HEADERS += src/GUI-QT/DRMPlot.h src/GUI-QT/EvaluationDlg.h
	HEADERS += src/GUI-QT/SlideShowViewer.h
	HEADERS += src/GUI-QT/SoundCardSelMenu.h
	SOURCES += src/GUI-QT/DRMPlot.cpp src/GUI-QT/EvaluationDlg.cpp
	SOURCES += src/GUI-QT/SlideShowViewer.cpp
	SOURCES += src/GUI-QT/SoundCardSelMenu.cpp
	FORMS += TransmDlgbase.ui AMSSDlgbase.ui systemevalDlgbase.ui StationsDlgbase.ui EPGDlgbase.ui
	FORMS += GeneralSettingsDlgbase.ui MultSettingsDlgbase.ui AboutDlgbase.ui
	FORMS += DRMMainWindow.ui FMMainWindow.ui AMMainWindow.ui LiveScheduleWindow.ui
	FORMS += SlideShowViewer.ui
	unix {
		macx {
			exists(libs/qwt.framework) {
				message("with qwt6")
				INCLUDEPATH += libs/qwt.framework/Headers
				LIBS += -framework qwt
			}
			else {
				error("no usable qwt version 6 found")
			}
		}
		else {
			exists(/usr/include/qwt/qwt.h) {
				message("with qwt")
				INCLUDEPATH += /usr/include/qwt
				exists(/usr/lib/libqwt.so) {
					LIBS += -lqwt
				}
				exists(/usr/lib/libqwt-qt5.so) {
					LIBS += -lqwt-qt5
				}
			}
			exists(/usr/include/qwt5/qwt.h) {
				message("with qwt")
				INCLUDEPATH += /usr/include/qwt5
				LIBS += -lqwt
			}
			exists(/usr/include/qwt-qt4/qwt.h) {
				message("with qwt")
				INCLUDEPATH += /usr/include/qwt-qt4
				LIBS += -lqwt-qt4
			}
	target.path = /usr/bin
	documentation.path = /usr/share/man/man1
	documentation.files = linux/dream.1
	INSTALLS += documentation
		}
	}
	win32 {
		 INCLUDEPATH += libs/qwt
		 CONFIG( debug, debug|release ) {
			 # debug
			 LIBS += -lqwtd
		 } else {
			 # release
			 LIBS += -lqwt
		 }
	}
}
macx {
    INCLUDEPATH += /usr/local/include
    LIBS += -L/usr/local/lib
    LIBS += -framework CoreFoundation -framework CoreServices
    LIBS += -framework CoreAudio -framework AudioToolbox -framework AudioUnit
    UI_DIR = moc
    MOC_DIR = moc
    RC_FILE = src/GUI-QT/res/macicons.icns
    CONFIG += fdk-aac
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
    packagesExist(fdk-aac) {
        CONFIG += fdk-aac
    }
    exists(/usr/include/pulse/pulseaudio.h) {
        CONFIG += pulseaudio
                  message("with pulseaudio")
              }
    else {
        packagesExist(portaudio-2.0) {
           CONFIG += portaudio
           PKGCONFIG += portaudio-2.0
       }
    }
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
    exists(/usr/include/sndfile.h) | exists(/usr/local/include/sndfile.h){
        CONFIG += sndfile
                  message("with libsndfile")
              }
    exists(/opt/local/include/sndfile.h) {
        CONFIG += sndfile
                  message("with libsndfile")
              }
    exists(/usr/include/fftw3.h) | exists(/usr/local/include/fftw3.h) {
        DEFINES += HAVE_FFTW3_H
                   LIBS += -lfftw3
                           message("with fftw3")
                       }
    else {
      exists(/usr/include/fftw.h) {
	message("with fftw2")
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
      }
      else {
        error("no usable fftw library found - install fftw dev package")
      }
    }
    LIBS += -lz -ldl
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
        LIBS += -lrt
        UI_DIR = moc
        MOC_DIR = moc
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
    else {
        HEADERS += src/windows/Sound.h
        SOURCES += src/windows/Sound.cpp
        message("with mmsystem")
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
    UI_DIR = moc
    MOC_DIR = moc
    LIBS += -lsetupapi \
	-ladvapi32 \
	-lzlib \
	-luser32 \
    -lwinmm \
    -lwsock32
    DEFINES += HAVE_SETUPAPI \
    HAVE_LIBZ
    SOURCES += src/windows/Pacer.cpp
}
fdk-aac {
     DEFINES += HAVE_LIBFDK_AAC
     LIBS += -lfdk-aac
     HEADERS += src/sourcedecoders/fdk_aac_codec.h
     SOURCES += src/sourcedecoders/fdk_aac_codec.cpp
     message("with fdk-aac")
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
    DEFINES += HAVE_LIBHAMLIB
    macx:LIBS += -framework IOKit
    unix:LIBS += -lhamlib
    win32:LIBS += libhamlib-2.lib
	contains(QT, gui) {
		HEADERS += src/GUI-QT/RigDlg.h
		SOURCES += src/GUI-QT/RigDlg.cpp
		FORMS += RigDlg.ui
	}
}
alsa {
    DEFINES += USE_ALSA
    HEADERS += src/linux/soundsrc.h \
    src/linux/soundin.h \
    src/linux/soundout.h
    SOURCES += src/linux/alsa.cpp \
    src/linux/soundsrc.cpp
}
portaudio {
    DEFINES += USE_PORTAUDIO
    HEADERS += src/sound/pa_ringbuffer.h \
    src/sound/drm_portaudio.h
    SOURCES += src/sound/drm_portaudio.cpp \
    src/sound/pa_ringbuffer.c
    LIBS += -lportaudio
}
pulseaudio {
    DEFINES += USE_PULSEAUDIO
    HEADERS += src/sound/drm_pulseaudio.h
    SOURCES += src/sound/drm_pulseaudio.cpp
    LIBS += -lpulse
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
   src/datadecoding/epg/epgutil.h \
   src/datadecoding/journaline/NML.h \
   src/datadecoding/journaline/Splitter.h \
   src/datadecoding/journaline/cpplog.h \
   src/datadecoding/journaline/crc_8_16.h \
   src/datadecoding/journaline/dabdatagroupdecoder.h \
   src/datadecoding/journaline/dabdgdec_impl.h \
   src/datadecoding/journaline/log.h \
   src/datadecoding/journaline/newsobject.h \
   src/datadecoding/journaline/newssvcdec.h \
   src/datadecoding/journaline/newssvcdec_impl.h \
   src/datadecoding/Experiment.h \
   src/datadecoding/Journaline.h \
   src/datadecoding/MOTSlideShow.h \
   src/DataIO.h \
   src/drmchannel/ChannelSimulation.h \
   src/ReceptLog.h \
   src/PlotManager.h \
   src/ServiceInformation.h \
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
   src/OFDM.h \
   src/ofdmcellmapping/CellMappingTable.h \
   src/ofdmcellmapping/OFDMCellMapping.h \
   src/Parameter.h \
   src/resample/Resample.h \
   src/resample/ResampleFilter.h \
   src/SDC/SDC.h \
   src/Scheduler.h \
   src/sound/audiofilein.h \
   src/sound/selectioninterface.h \
   src/sound/sound.h \
   src/sound/soundinterface.h \
   src/sound/soundnull.h \
   src/sourcedecoders/AudioSourceDecoder.h \
   src/sourcedecoders/AudioSourceEncoder.h \
   src/sync/FreqSyncAcq.h \
   src/sync/SyncUsingPil.h \
   src/sync/TimeSync.h \
   src/sync/TimeSyncFilter.h \
   src/sync/TimeSyncTrack.h \
   src/tables/TableAMSS.h \
   src/tables/TableCarMap.h \
   src/tables/TableDRMGlobal.h \
   src/tables/TableFAC.h \
   src/tables/TableMLC.h \
   src/tables/TableQAMMapping.h \
   src/tables/TableStations.h \
   src/TextMessage.h \
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
      src/datadecoding/epg/epgutil.cpp \
      src/datadecoding/journaline/NML.cpp \
      src/datadecoding/journaline/dabdgdec_impl.c \
      src/datadecoding/journaline/Splitter.cpp \
      src/datadecoding/journaline/newsobject.cpp \
      src/datadecoding/journaline/newssvcdec_impl.cpp \
      src/datadecoding/journaline/crc_8_16.c \
      src/datadecoding/journaline/log.c \
      src/datadecoding/Experiment.cpp \
      src/datadecoding/Journaline.cpp \
      src/datadecoding/MOTSlideShow.cpp \
      src/DataIO.cpp \
      src/drmchannel/ChannelSimulation.cpp \
      src/ReceptLog.cpp \
      src/PlotManager.cpp \
      src/ServiceInformation.cpp \
      src/DrmReceiver.cpp \
      src/DRMSignalIO.cpp \
      src/DrmSimulation.cpp \
      src/DrmTransmitter.cpp \
      src/FAC/FAC.cpp \
      src/InputResample.cpp \
      src/Scheduler.cpp \
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
      src/OFDM.cpp \
      src/ofdmcellmapping/CellMappingTable.cpp \
      src/ofdmcellmapping/OFDMCellMapping.cpp \
      src/Parameter.cpp \
      src/resample/Resample.cpp \
      src/resample/ResampleFilter.cpp \
      src/SDC/SDCReceive.cpp \
      src/SDC/SDCTransmit.cpp \
      src/SimulationParameters.cpp \
      src/sound/audiofilein.cpp \
      src/sourcedecoders/AudioSourceDecoder.cpp \
      src/sourcedecoders/AudioSourceEncoder.cpp \
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
      src/Version.cpp \
      src/GUI-QT/main.cpp
!console {
HEADERS += \
    src/datadecoding/epg/EPG.h \
    src/datadecoding/epg/epgdec.h \
    src/GUI-QT/Logging.h \
    src/GUI-QT/Rig.h \
    src/MDI/PacketSocketQT.h
SOURCES += \
    src/datadecoding/epg/EPG.cpp \
    src/datadecoding/epg/epgdec.cpp \
    src/GUI-QT/Logging.cpp \
    src/GUI-QT/Rig.cpp \
    src/MDI/PacketSocketQT.cpp
}
else {
HEADERS += \
    src/MDI/PacketSocketNull.h
SOURCES += \
    src/MDI/PacketSocketNull.cpp
}
!console:!qtconsole {
HEADERS += \
    src/GUI-QT/AnalogDemDlg.h \
    src/GUI-QT/DialogUtil.h \
    src/GUI-QT/EPGDlg.h \
    src/GUI-QT/fdrmdialog.h \
    src/GUI-QT/fmdialog.h \
    src/GUI-QT/GeneralSettingsDlg.h \
    src/GUI-QT/LiveScheduleDlg.h \
    src/GUI-QT/MultColorLED.h \
    src/GUI-QT/MultSettingsDlg.h \
    src/GUI-QT/StationsDlg.h \
    src/GUI-QT/TransmDlg.h
SOURCES += \
    src/GUI-QT/AnalogDemDlg.cpp \
    src/GUI-QT/DialogUtil.cpp \
    src/GUI-QT/EPGDlg.cpp \
    src/GUI-QT/fmdialog.cpp \
    src/GUI-QT/fdrmdialog.cpp \
    src/GUI-QT/GeneralSettingsDlg.cpp \
    src/GUI-QT/LiveScheduleDlg.cpp \
    src/GUI-QT/MultColorLED.cpp \
    src/GUI-QT/MultSettingsDlg.cpp \
    src/GUI-QT/StationsDlg.cpp \
    src/GUI-QT/TransmDlg.cpp
}
message($$VERSION_MESSAGE $$DEBUG_MESSAGE $$UI_MESSAGE)
