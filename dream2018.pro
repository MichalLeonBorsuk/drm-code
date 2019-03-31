#-------------------------------------------------
#
# Project created by QtCreator 2018-12-29T19:03:54
#
#-------------------------------------------------

QT       += core gui network multimedia xml svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = dream2018
TEMPLATE = app
UI_DIR = $$OUT_PWD/ui
MOC_DIR = $$OUT_PWD/moc
RESOURCES = $$PWD/src/GUI-QT/res/icons.qrc
INCLUDEPATH += $$PWD/src/GUI-QT
INCLUDEPATH += $$PWD/src
INCLUDEPATH += $$PWD/include
INCLUDEPATH += $$PWD/include/qwt
DEPENDPATH += $$PWD/include
DEPENDPATH += $$PWD/include/qwt

LIBS += -L$$PWD/lib

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

DEFINES += HAVE_LIBHAMLIB HAVE_LIBZ HAVE_LIBSNDFILE

CONFIG += c++11

SOURCES += \
    src/AMDemodulation.cpp \
    src/AMSSDemodulation.cpp \
    src/DataIO.cpp \
    src/DrmReceiver.cpp \
    src/DRMSignalIO.cpp \
    src/DrmSimulation.cpp \
    src/DrmTransceiver.cpp \
    src/DrmTransmitter.cpp \
    src/InputResample.cpp \
    src/IQInputFilter.cpp \
    src/MSCMultiplexer.cpp \
    src/OFDM.cpp \
    src/Parameter.cpp \
    src/PlotManager.cpp \
    src/ReceptLog.cpp \
    src/Scheduler.cpp \
    src/ServiceInformation.cpp \
    src/SimulationParameters.cpp \
    src/TextMessage.cpp \
    src/Version.cpp \
    src/chanest/ChanEstTime.cpp \
    src/chanest/ChannelEstimation.cpp \
    src/chanest/IdealChannelEstimation.cpp \
    src/chanest/TimeLinear.cpp \
    src/chanest/TimeWiener.cpp \
    src/datadecoding/journaline/newsobject.cpp \
    src/datadecoding/journaline/newssvcdec_impl.cpp \
    src/datadecoding/journaline/NML.cpp \
    src/datadecoding/journaline/Splitter.cpp \
    src/datadecoding/journaline/crc_8_16.c \
    src/datadecoding/journaline/dabdgdec_impl.c \
    src/datadecoding/journaline/log.c \
    src/datadecoding/DABMOT.cpp \
    src/datadecoding/DataDecoder.cpp \
    src/datadecoding/DataEncoder.cpp \
    src/datadecoding/epgutil.cpp \
    src/datadecoding/Experiment.cpp \
    src/datadecoding/Journaline.cpp \
    src/datadecoding/MOTSlideShow.cpp \
    src/drmchannel/ChannelSimulation.cpp \
    src/FAC/FAC.cpp \
    src/GUI-QT/AACCodecParams.cpp \
    src/GUI-QT/AnalogDemDlg.cpp \
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
    src/GUI-QT/Logging.cpp \
    src/GUI-QT/main.cpp \
    src/GUI-QT/MultColorLED.cpp \
    src/GUI-QT/MultSettingsDlg.cpp \
    src/GUI-QT/OpusCodecParams.cpp \
    src/GUI-QT/RigDlg.cpp \
    src/GUI-QT/Schedule.cpp \
    src/GUI-QT/SlideShowViewer.cpp \
    src/GUI-QT/SoundCardSelMenu.cpp \
    src/GUI-QT/StationsDlg.cpp \
    src/GUI-QT/TransmDlg.cpp \
    src/interleaver/BlockInterleaver.cpp \
    src/interleaver/SymbolInterleaver.cpp \
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
    src/MDI/PacketSocketNull.cpp \
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
    src/ofdmcellmapping/CellMappingTable.cpp \
    src/ofdmcellmapping/OFDMCellMapping.cpp \
    src/resample/Resample.cpp \
    src/resample/ResampleFilter.cpp \
    src/SDC/SDCReceive.cpp \
    src/SDC/SDCTransmit.cpp \
    src/sound/audiofilein.cpp \
    src/sound/selectioninterface.cpp \
    src/sound/soundinterface.cpp \
    src/sound/soundnull.cpp \
    src/sourcedecoders/aac_codec.cpp \
    src/sourcedecoders/AudioCodec.cpp \
    src/sourcedecoders/AudioSourceDecoder.cpp \
    src/sourcedecoders/AudioSourceEncoder.cpp \
    src/sourcedecoders/fdk_aac_codec.cpp \
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
    src/util/CRC.cpp \
    src/util/FileTyper.cpp \
    src/util/LogPrint.cpp \
    src/util/Reassemble.cpp \
    src/util/Settings.cpp \
    src/util/Utilities.cpp \
    src/util/Hamlib.cpp \
    src/util-QT/Rig.cpp \
    src/util-QT/EPG.cpp \
    src/util-QT/epgdec.cpp \
    src/util-QT/PacketSocketQT.cpp \
    src/util-QT/Util.cpp \
    src/windows/Pacer.cpp \
    src/windows/platform_util.cpp

HEADERS += \
    src/AMDemodulation.h \
    src/AMSSDemodulation.h \
    src/DataIO.h \
    src/DrmReceiver.h \
    src/DRMSignalIO.h \
    src/DrmSimulation.h \
    src/DrmTransceiver.h \
    src/DrmTransmitter.h \
    src/GlobalDefinitions.h \
    src/InputResample.h \
    src/IQInputFilter.h \
    src/MSCMultiplexer.h \
    src/OFDM.h \
    src/Parameter.h \
    src/PlotManager.h \
    src/ReceptLog.h \
    src/Scheduler.h \
    src/ServiceInformation.h \
    src/TextMessage.h \
    src/UpsampleFilter.h \
    src/UpsampleFilter.octave \
    src/Version.h \
    src/chanest/ChanEstTime.h \
    src/chanest/ChannelEstimation.h \
    src/chanest/IdealChannelEstimation.h \
    src/chanest/TimeLinear.h \
    src/chanest/TimeWiener.h \
    src/datadecoding/journaline/cpplog.h \
    src/datadecoding/journaline/crc_8_16.h \
    src/datadecoding/journaline/dabdatagroupdecoder.h \
    src/datadecoding/journaline/dabdgdec_impl.h \
    src/datadecoding/journaline/log.h \
    src/datadecoding/journaline/newsobject.h \
    src/datadecoding/journaline/newssvcdec.h \
    src/datadecoding/journaline/newssvcdec_impl.h \
    src/datadecoding/journaline/NML.h \
    src/datadecoding/journaline/Splitter.h \
    src/datadecoding/DABMOT.h \
    src/datadecoding/DataDecoder.h \
    src/datadecoding/DataEncoder.h \
    src/datadecoding/epgutil.h \
    src/datadecoding/Experiment.h \
    src/datadecoding/Journaline.h \
    src/datadecoding/MOTSlideShow.h \
    src/drmchannel/ChannelSimulation.h \
    src/FAC/FAC.h \
    src/GUI-QT/RigDlg.h \
    src/GUI-QT/AACCodecParams.h \
    src/GUI-QT/AnalogDemDlg.h \
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
    src/GUI-QT/Logging.h \
    src/GUI-QT/MultColorLED.h \
    src/GUI-QT/MultSettingsDlg.h \
    src/GUI-QT/OpusCodecParams.h \
    src/GUI-QT/RigDlg.h \
    src/GUI-QT/Schedule.h \
    src/GUI-QT/SlideShowViewer.h \
    src/GUI-QT/SoundCardSelMenu.h \
    src/GUI-QT/StationsDlg.h \
    src/GUI-QT/TransmDlg.h \
    src/interleaver/BlockInterleaver.h \
    src/interleaver/SymbolInterleaver.h \
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
    src/MDI/PacketSocketNull.h \
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
    src/ofdmcellmapping/CellMappingTable.h \
    src/ofdmcellmapping/OFDMCellMapping.h \
    src/resample/Resample.h \
    src/resample/ResampleFilter.h \
    src/SDC/SDC.h \
    src/sound/audiofilein.h \
    src/sound/LatencyFilter.h \
    src/sound/selectioninterface.h \
    src/sound/sound.h \
    src/sound/soundinterface.h \
    src/sound/soundnull.h \
    src/sourcedecoders/aac_codec.h \
    src/sourcedecoders/AudioCodec.h \
    src/sourcedecoders/AudioSourceDecoder.h \
    src/sourcedecoders/AudioSourceEncoder.h \
    src/sourcedecoders/faac_dll.h \
    src/sourcedecoders/fdk_aac_codec.h \
    src/sourcedecoders/neaacdec_dll.h \
    src/sourcedecoders/null_codec.h \
    src/sourcedecoders/opus_codec.h \
    src/sourcedecoders/opus_dll.h \
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
    src/util/AudioFile.h \
    src/util/Buffer.h \
    src/util/CRC.h \
    src/util/FileTyper.h \
    src/util/LibraryLoader.h \
    src/util/LogPrint.h \
    src/util/Modul.h \
    src/util/Pacer.h \
    src/util/Reassemble.h \
    src/util/Settings.h \
    src/util/Utilities.h \
    src/util/Vector.h \
    src/util/Hamlib.h \
    src/util-QT/Rig.h \
    src/util-QT/EPG.h \
    src/util-QT/epgdec.h \
    src/util-QT/PacketSocketQT.h \
    src/util-QT/Rig.h \
    src/util-QT/Util.h \
    src/windows/platform_util.h

FORMS += \
    src/GUI-QT/AACCodecParams.ui \
    src/GUI-QT/AboutDlgbase.ui \
    src/GUI-QT/AMMainWindow.ui \
    src/GUI-QT/AMSSDlgbase.ui \
    src/GUI-QT/DRMMainWindow.ui \
    src/GUI-QT/EPGDlgbase.ui \
    src/GUI-QT/FMMainWindow.ui \
    src/GUI-QT/GeneralSettingsDlgbase.ui \
    src/GUI-QT/JLViewer.ui \
    src/GUI-QT/LiveScheduleWindow.ui \
    src/GUI-QT/MultSettingsDlgbase.ui \
    src/GUI-QT/OpusCodecParams.ui \
    src/GUI-QT/RigDlg.ui \
    src/GUI-QT/SlideShowViewer.ui \
    src/GUI-QT/StationsDlgbase.ui \
    src/GUI-QT/systemevalDlgbase.ui \
    src/GUI-QT/TransmDlgbase.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES +=

win32 {
    RC_FILE = windows/dream.rc
    DEFINES += HAVE_SETUPAPI
    LIBS += -lzlib -lfdk-aac -llibfftw3-3 -llibhamlib-2 -llibsndfile-1
    CONFIG(release, debug|release) {
        LIBS += -lqwt
        LIBS += -lsetupapi -lwsock32 -lws2_32 -ladvapi32 -luser32
        QMAKE_LFLAGS_RELEASE += /NODEFAULTLIB:libcmt.lib
        OBJECTS_DIR = $$OUT_PWD/release/obj
    }
    CONFIG(debug, debug|release) {
        LIBS += -lqwtd
        LIBS += -lsetupapi -lwsock32 -lws2_32 -ladvapi32 -luser32
        QMAKE_LFLAGS_DEBUG += /NODEFAULTLIB:libcmtd.lib
        QMAKE_LFLAGS_DEBUG += /NODEFAULTLIB:libcmt.lib
        #QMAKE_LFLAGS_DEBUG += /NODEFAULTLIB:msvcrt.lib
        OBJECTS_DIR = $$OUT_PWD/debug/obj
    }
}
unix {
    LIBS += -lqwt -lz -lfdk-aac -lfftw3  -lhamlib2 -lsndfile
    OBJECTS_DIR = $$OUT_PWD/obj
}

message($$QT)
