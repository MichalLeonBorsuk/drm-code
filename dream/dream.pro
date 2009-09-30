TEMPLATE = app
TARGET = dream
QT += network \
    xml
CONFIG += qt \
    warn_on \
    thread \
    hamlib \
    portaudio \
    pcap
INCLUDEPATH += ../include
LIBS += -L../lib
VPATH += src/GUI-QT
INCLUDEPATH += src/GUI-QT
FORMS += DRMMainWindow.ui \
    FMMainWindow.ui \
    AnalogMainWindow.ui \
    TransmitterMainWindow.ui \
    src/GUI-QT/receiverinputwidget.ui \
    src/GUI-QT/DIoutputSelector.ui \
    src/GUI-QT/rigconfigurationdialog.ui
FORMS += AMSSDlg.ui \
    SystemEvalDlg.ui \
    JLViewer.ui \
    BWSViewer.ui \
    SlideShowViewer.ui
FORMS += LiveScheduleDlg.ui \
    StationsDlg.ui \
    EPGDlg.ui \
    AboutDlg.ui
FORMS += ReceiverSettingsDlg.ui \
    LatLongEditDlg.ui
RCC_DIR = src/GUI-QT/res
RESOURCES += src/GUI-QT/res/icons.qrc
TRANSLATIONS = src/GUI-QT/languages/drm.fr.ts \
    src/GUI-QT/languages/dreamtr.es.ts
UI_DIR = moc
MOC_DIR = moc
macx { 
    RC_FILE = src/GUI-QT/res/macicons.icns
    LIBS += -framework \
        CoreFoundation \
        -framework \
        CoreServices
    LIBS += -framework \
        CoreAudio \
        -framework \
        AudioToolbox \
        -framework \
        AudioUnit
}
unix { 
    LIBS += -lsndfile \
        -lz \
        -lrfftw \
        -lfftw
    DEFINES += HAVE_RFFTW_H
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
    macx:LIBS += -lqwt
    else { 
        INCLUDEPATH += /usr/include/qwt-qt4
        LIBS += -lqwt-qt4
        INCLUDEPATH += linux
        LIBS += -lrt
        SOURCES += src/sound/shmsoundin.cpp \
            src/sound/pa_shm_ringbuffer.c
        HEADERS += src/sound/shmsoundin.h \
            src/sound/pa_shm_ringbuffer.h
    }
}
win32 { 
    DEFINES -= UNICODE
    LIBS += -lsetupapi \
        -lqwt5 \
        -lws2_32
    win32-g++ { 
        DEFINES += HAVE_STDINT_H \
            HAVE_STDLIB_H
        LIBS += -lsndfile \
            -lz \
            -lrfftw \
            -lfftw \
            -lstdc++
    }
    else { 
        DEFINES += NOMINMAX
        TEMPLATE = vcapp
        LIBS += libsndfile-1.lib \
            zdll.lib \
            qwt5.lib
        LIBS += libfftw.lib
        QMAKE_LFLAGS_RELEASE += /NODEFAULTLIB:"MSVCRTD, LIBCMT"
        QMAKE_LFLAGS_DEBUG += /NODEFAULTLIB:msvcrtd.lib
    }
}
hamlib { 
    DEFINES += HAVE_LIBHAMLIB \
        HAVE_RIG_PARSE_MODE
    HEADERS += src/util/Hamlib.h \
        src/util/rigclass.h
    SOURCES += src/util/Hamlib.cpp \
        src/util/rigclass.cc
    unix:LIBS += -lhamlib
    macx:LIBS += -framework \
        IOKit
    win32 { 
        win32-g++:LIBS += -lhamlib
        else:LIBS += libhamlib-2.lib
    }
}
pcap { 
    DEFINES += HAVE_LIBPCAP
    win32:LIBS += -lwpcap
    else:LIBS += -lpcap
}
portaudio { 
    DEFINES += USE_PORTAUDIO
    HEADERS += src/sound/pa_ringbuffer.h \
        src/sound/drm_portaudio.h
    SOURCES += src/sound/drm_portaudio.cpp \
        src/sound/pa_ringbuffer.c
    win32-g++:LIBS += ../lib/PortAudio.dll
    win32-msvc2008:LIBS += ../lib/portaudio_x86.lib
    unix:LIBS += -lportaudio
}
winmm { 
    CONFIG -= portaudio
    HEADERS += src/sound/winmm.h \
        src/sound/SoundWin.h
    SOURCES += src/sound/winmm.cpp
    LIBS += -lwinmm
}
HEADERS += src/Measurements.h \
    src/AMDemodulation.h \
    src/AMSSDemodulation.h \
    src/chanest/ChanEstTime.h \
    src/chanest/ChannelEstimation.h \
    src/chanest/IdealChannelEstimation.h \
    src/chanest/TimeLinear.h \
    src/chanest/TimeWiener.h \
    src/datadecoding/DABMOT.h \
    src/datadecoding/DataDecoder.h \
    src/datadecoding/DataApplication.h \
    src/datadecoding/DataEncoder.h \
    src/datadecoding/EPGDecoder.h \
    src/datadecoding/epg/EPG.h \
    src/datadecoding/epg/epgdec.h \
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
    src/datadecoding/Journaline.h \
    src/datadecoding/MOTSlideShow.h \
    src/DataIO.h \
    src/drmchannel/ChannelSimulation.h \
    src/ReceptLog.h \
    src/ServiceInformation.h \
    src/DrmReceiver.h \
    src/DRMSignalIO.h \
    src/DrmSimulation.h \
    src/DrmTransmitter.h \
    src/DrmEncoder.h \
    src/DrmModulator.h \
    src/FAC/FAC.h \
    src/SigProc.h \
    src/GlobalDefinitions.h \
    src/GPSData.h \
    src/GPSReceiver.h \
    src/GUI-QT/RxApp.h \
    src/GUI-QT/DialogUtil.h \
    src/GUI-QT/DRMPlot.h \
    src/GUI-QT/EPGDlg.h \
    src/GUI-QT/AnalogMainWindow.h \
    src/GUI-QT/DRMMainWindow.h \
    src/GUI-QT/FMMainWindow.h \
    src/GUI-QT/TransmitterMainWindow.h \
    src/GUI-QT/LiveScheduleDlg.h \
    src/GUI-QT/LatLongEditDlg.h \
    src/GUI-QT/MultColorLED.h \
    src/GUI-QT/JLViewer.h \
    src/GUI-QT/SlideShowViewer.h \
    src/GUI-QT/BWSViewer.h \
    src/GUI-QT/ReceiverSettingsDlg.h \
    src/GUI-QT/Loghelper.h \
    src/GUI-QT/StationsDlg.h \
    src/GUI-QT/ScheduleModel.h \
    src/GUI-QT/SystemEvalDlg.h \
    src/GUI-QT/jlbrowser.h \
    src/GUI-QT/bwsbrowser.h \
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
    src/MDI/PacketSocketNull.h \
    src/MDI/PacketSocketQT.h \
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
    src/ReceiverInterface.h \
    src/selectioninterface.h \
    src/soundinterface.h \
    src/sound.h \
    src/sound/soundnull.h \
    src/sound/soundfile.h \
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
    src/util/LogPrint.h \
    src/util/Modul.h \
    src/util/ReceiverModul.h \
    src/util/ReceiverModul_impl.h \
    src/util/SimulationModul.h \
    src/util/SimulationModul_impl.h \
    src/util/TransmitterModul.h \
    src/util/TransmitterModul_impl.h \
    src/util/Pacer.h \
    src/util/Reassemble.h \
    src/util/Settings.h \
    src/util/Utilities.h \
    src/util/Vector.h \
    src/Version.h \
    src/GUI-QT/receiverinputwidget.h \
    src/GUI-QT/DIoutputSelector.h \
    src/GUI-QT/rigconfigurationdialog.h
SOURCES += src/Measurements.cpp \
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
    src/datadecoding/EPGDecoder.cpp \
    src/datadecoding/epg/EPG.cpp \
    src/datadecoding/epg/epgdec.cpp \
    src/datadecoding/epg/epgutil.cpp \
    src/datadecoding/journaline/NML.cpp \
    src/datadecoding/journaline/dabdgdec_impl.c \
    src/datadecoding/journaline/Splitter.cpp \
    src/datadecoding/journaline/newsobject.cpp \
    src/datadecoding/journaline/newssvcdec_impl.cpp \
    src/datadecoding/journaline/crc_8_16.c \
    src/datadecoding/journaline/log.c \
    src/datadecoding/Journaline.cpp \
    src/datadecoding/MOTSlideShow.cpp \
    src/DataIO.cpp \
    src/drmchannel/ChannelSimulation.cpp \
    src/ReceptLog.cpp \
    src/ServiceInformation.cpp \
    src/DrmReceiver.cpp \
    src/DRMSignalIO.cpp \
    src/DrmSimulation.cpp \
    src/DrmTransmitter.cpp \
    src/DrmEncoder.cpp \
    src/DrmModulator.cpp \
    src/FAC/FAC.cpp \
    src/GPSData.cpp \
    src/GPSReceiver.cpp \
    src/GUI-QT/RxApp.cpp \
    src/GUI-QT/DialogUtil.cpp \
    src/GUI-QT/DRMPlot.cpp \
    src/GUI-QT/EPGDlg.cpp \
    src/GUI-QT/AnalogMainWindow.cpp \
    src/GUI-QT/DRMMainWindow.cpp \
    src/GUI-QT/FMMainWindow.cpp \
    src/GUI-QT/TransmitterMainWindow.cpp \
    src/GUI-QT/LiveScheduleDlg.cpp \
    src/GUI-QT/main.cpp \
    src/GUI-QT/LatLongEditDlg.cpp \
    src/GUI-QT/MultColorLED.cpp \
    src/GUI-QT/JLViewer.cpp \
    src/GUI-QT/SlideShowViewer.cpp \
    src/GUI-QT/BWSViewer.cpp \
    src/GUI-QT/ReceiverSettingsDlg.cpp \
    src/GUI-QT/Loghelper.cpp \
    src/GUI-QT/ScheduleModel.cpp \
    src/GUI-QT/StationsDlg.cpp \
    src/GUI-QT/SystemEvalDlg.cpp \
    src/GUI-QT/jlbrowser.cpp \
    src/GUI-QT/bwsbrowser.cpp \
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
    src/MDI/PacketSourceFile.cpp \
    src/MDI/PacketSocketNull.cpp \
    src/MDI/PacketSocketQT.cpp \
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
    src/sound/soundfile.cpp \
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
    src/util/Pacer.cpp \
    src/util/Reassemble.cpp \
    src/util/Settings.cpp \
    src/util/Utilities.cpp \
    src/Version.cpp \
    src/GUI-QT/receiverinputwidget.cpp \
    src/GUI-QT/DIoutputSelector.cpp \
    src/GUI-QT/rigconfigurationdialog.cpp
