# Microsoft Developer Studio Project File - Name="FDRM" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **


# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=FDRM - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run


!MESSAGE 
!MESSAGE NMAKE /f "FDRM.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:


!MESSAGE 
!MESSAGE NMAKE /f "FDRM.mak" CFG="FDRM - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:

!MESSAGE 
!MESSAGE "FDRM - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "FDRM - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 
# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "FDRM - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /GX /O2 /I "$(QTDIR)\include" /I "../libs" /I "../libs/qwt" /I "../src/GUI-QT" /I "./moc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "QT_DLL" /D "QT_THREAD_SUPPORT" /D "USE_QT_GUI" /D "FREEIMAGE_LIB" /D "HAVE_LIBFREEIMAGE" /D "HAVE_LIBHAMLIB" /D "HAVE_SETUPAPI" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 libhamlib-2.lib FreeImage.lib libfftw.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib imm32.lib wsock32.lib winspool.lib winmm.lib $(QTDIR)\lib\qt-mt230nc.lib $(QTDIR)\lib\qtmain.lib libqwt.lib setupapi.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"MSVCRTD" /out:"Release/Dream.exe" /libpath:"../libs"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "FDRM - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /Gm /GX /ZI /Od /I "$(QTDIR)\include" /I "../libs" /I "../libs/qwt" /I "../src/GUI-QT" /I "./moc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "QT_DLL" /D "QT_THREAD_SUPPORT" /D "USE_QT_GUI" /D "FREEIMAGE_LIB" /D "HAVE_LIBFREEIMAGE" /D "HAVE_LIBHAMLIB" /D "HAVE_SETUPAPI" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 libhamlib-2.lib FreeImage.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib imm32.lib wsock32.lib winspool.lib winmm.lib $(QTDIR)\lib\qt-mt230nc.lib $(QTDIR)\lib\qtmain.lib libfftw.lib libqwt.lib setupapi.lib /nologo /subsystem:windows /debug /machine:I386 /out:"Debug/Dream.exe" /pdbtype:sept /libpath:"../libs"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "FDRM - Win32 Release"
# Name "FDRM - Win32 Debug"
# Begin Group "Quellcodedateien"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;for;f90"
# Begin Group "Source GUI, App"

# PROP Default_Filter ""
# Begin Group "Do not modify these files!"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\moc\AboutDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\AMSSDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\AnalogDemDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\EPGDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\fdrmdialogbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\fmdialogbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\GeneralSettingsDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\LiveScheduleDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_AboutDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_AMSSDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_AnalogDemDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_AnalogDemDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_DialogUtil.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_DRMPlot.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_EPGDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_EPGDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_fdrmdialog.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_fdrmdialogbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_fmdialog.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_fmdialogbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_GeneralSettingsDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_GeneralSettingsDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_LiveScheduleDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_LiveScheduleDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_Logging.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_MultColorLED.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_MultimediaDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_MultimediaDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_MultSettingsDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_MultSettingsDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_PacketSocketQT.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_Rig.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_StationsDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_StationsDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_systemevalDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_systemevalDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_TransmDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_TransmDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\MultimediaDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\MultSettingsDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\StationsDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\systemevalDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\TransmDlgbase.cpp
# End Source File
# End Group
# Begin Source File

SOURCE="..\src\GUI-QT\AnalogDemDlg.cpp"
# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\DialogUtil.cpp"
# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\qt2\DRMPlot.cpp"
# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\EPGDlg.cpp"
# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\fdrmdialog.cpp"

!IF  "$(CFG)" == "FDRM - Win32 Release"

# ADD CPP /MD

!ELSEIF  "$(CFG)" == "FDRM - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\fmdialog.cpp"
# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\GeneralSettingsDlg.cpp"
# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\LiveScheduleDlg.cpp"
# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\Logging.cpp"
# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\main.cpp"
# End Source File
# Begin Source File

SOURCE=.\MocGUI.bat
# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\MultColorLED.cpp"
# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\MultimediaDlg.cpp"
# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\MultSettingsDlg.cpp"
# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\Rig.cpp"
# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\StationsDlg.cpp"
# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\systemevalDlg.cpp"

!IF  "$(CFG)" == "FDRM - Win32 Release"

# ADD CPP /MD

!ELSEIF  "$(CFG)" == "FDRM - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\TransmDlg.cpp"
# End Source File
# End Group
# Begin Group "Source MLC"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\mlc\BitInterleaver.cpp
# End Source File
# Begin Source File

SOURCE=..\src\mlc\ChannelCode.cpp
# End Source File
# Begin Source File

SOURCE=..\src\mlc\ConvEncoder.cpp
# End Source File
# Begin Source File

SOURCE=..\src\mlc\EnergyDispersal.cpp
# End Source File
# Begin Source File

SOURCE=..\src\mlc\Metric.cpp
# End Source File
# Begin Source File

SOURCE=..\src\mlc\MLC.cpp
# End Source File
# Begin Source File

SOURCE=..\src\mlc\QAMMapping.cpp
# End Source File
# Begin Source File

SOURCE=..\src\mlc\TrellisUpdateMMX.cpp
# End Source File
# Begin Source File

SOURCE=..\src\mlc\TrellisUpdateSSE2.cpp
# End Source File
# Begin Source File

SOURCE=..\src\mlc\ViterbiDecoder.cpp

!IF  "$(CFG)" == "FDRM - Win32 Release"

# ADD CPP /G6

!ELSEIF  "$(CFG)" == "FDRM - Win32 Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "Source Tables"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\tables\TableCarMap.cpp
# End Source File
# Begin Source File

SOURCE=..\src\tables\TableFAC.cpp
# End Source File
# End Group
# Begin Group "Source FAC"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\FAC\FAC.cpp
# End Source File
# End Group
# Begin Group "Source SDC"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\SDC\SDCReceive.cpp
# End Source File
# Begin Source File

SOURCE=..\src\SDC\SDCTransmit.cpp
# End Source File
# End Group
# Begin Group "Source sync"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\sync\FreqSyncAcq.cpp
# End Source File
# Begin Source File

SOURCE=..\src\sync\SyncUsingPil.cpp
# End Source File
# Begin Source File

SOURCE=..\src\sync\TimeSync.cpp
# End Source File
# Begin Source File

SOURCE=..\src\sync\TimeSyncFilter.cpp
# End Source File
# Begin Source File

SOURCE=..\src\sync\TimeSyncTrack.cpp
# End Source File
# End Group
# Begin Group "Source ChannelEstimation"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\chanest\ChanEstTime.cpp
# End Source File
# Begin Source File

SOURCE=..\src\chanest\ChannelEstimation.cpp
# End Source File
# Begin Source File

SOURCE=..\src\chanest\IdealChannelEstimation.cpp
# End Source File
# Begin Source File

SOURCE=..\src\chanest\TimeLinear.cpp
# End Source File
# Begin Source File

SOURCE=..\src\chanest\TimeWiener.cpp
# End Source File
# End Group
# Begin Group "Source Matlib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\matlib\MatlibSigProToolbox.cpp
# End Source File
# Begin Source File

SOURCE=..\src\matlib\MatlibStdToolbox.cpp
# End Source File
# End Group
# Begin Group "Source OFDMCellMapping"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\ofdmcellmapping\CellMappingTable.cpp
# End Source File
# Begin Source File

SOURCE=..\src\ofdmcellmapping\OFDMCellMapping.cpp
# End Source File
# End Group
# Begin Group "Source datadecoding"

# PROP Default_Filter ".c .cpp"
# Begin Group "epg"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\datadecoding\epg\EPG.cpp
# End Source File
# Begin Source File

SOURCE=..\src\datadecoding\epg\epgdec.cpp
# End Source File
# Begin Source File

SOURCE=..\src\datadecoding\epg\epgutil.cpp
# End Source File
# End Group
# Begin Group "journaline"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\datadecoding\journaline\crc_8_16.c
# End Source File
# Begin Source File

SOURCE=..\src\datadecoding\journaline\dabdgdec_impl.c
# End Source File
# Begin Source File

SOURCE=..\src\datadecoding\journaline\log.c
# End Source File
# Begin Source File

SOURCE=..\src\datadecoding\journaline\newsobject.cpp
# End Source File
# Begin Source File

SOURCE=..\src\datadecoding\journaline\newssvcdec_impl.cpp
# End Source File
# Begin Source File

SOURCE=..\src\datadecoding\journaline\NML.cpp
# End Source File
# Begin Source File

SOURCE=..\src\datadecoding\journaline\Splitter.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=..\src\datadecoding\DABMOT.cpp
# End Source File
# Begin Source File

SOURCE=..\src\datadecoding\DataDecoder.cpp
# End Source File
# Begin Source File

SOURCE=..\src\datadecoding\DataEncoder.cpp
# End Source File
# Begin Source File

SOURCE=..\src\datadecoding\Experiment.cpp
# End Source File
# Begin Source File

SOURCE=..\src\datadecoding\Journaline.cpp
# End Source File
# Begin Source File

SOURCE=..\src\datadecoding\MOTSlideShow.cpp
# End Source File
# End Group
# Begin Group "Source Utilities"

# PROP Default_Filter ".c .cpp"
# Begin Source File

SOURCE=..\src\util\CRC.cpp
# End Source File
# Begin Source File

SOURCE=..\src\util\LogPrint.cpp
# End Source File
# Begin Source File

SOURCE=..\src\util\Reassemble.cpp
# End Source File
# Begin Source File

SOURCE=..\src\util\Settings.cpp
# End Source File
# Begin Source File

SOURCE=..\src\util\Utilities.cpp
# End Source File
# End Group
# Begin Group "Source MDI"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\MDI\AFPacketGenerator.cpp
# End Source File
# Begin Source File

SOURCE=..\src\sourcedecoders\AudioSourceEncoder.cpp
# End Source File
# Begin Source File

SOURCE=..\src\MDI\MDIDecode.cpp
# End Source File
# Begin Source File

SOURCE=..\src\MDI\MDIInBuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\src\MDI\MDIRSCI.cpp
# End Source File
# Begin Source File

SOURCE=..\src\MDI\MDITagItemDecoders.cpp
# End Source File
# Begin Source File

SOURCE=..\src\MDI\MDITagItems.cpp
# End Source File
# Begin Source File

SOURCE=..\src\MDI\PacketSinkFile.cpp
# End Source File
# Begin Source File

SOURCE=..\src\MDI\PacketSocketNull.cpp
# End Source File
# Begin Source File

SOURCE=..\src\MDI\PacketSocketQT.cpp
# End Source File
# Begin Source File

SOURCE=..\src\MDI\PacketSourceFile.cpp
# End Source File
# Begin Source File

SOURCE=..\src\MDI\Pft.cpp
# End Source File
# Begin Source File

SOURCE=..\src\MDI\RCITagItems.cpp
# End Source File
# Begin Source File

SOURCE=..\src\MDI\RSCITagItemDecoders.cpp
# End Source File
# Begin Source File

SOURCE=..\src\MDI\RSISubscriber.cpp
# End Source File
# Begin Source File

SOURCE=..\src\MDI\TagPacketDecoder.cpp
# End Source File
# Begin Source File

SOURCE=..\src\MDI\TagPacketDecoderMDI.cpp
# End Source File
# Begin Source File

SOURCE=..\src\MDI\TagPacketDecoderRSCIControl.cpp
# End Source File
# Begin Source File

SOURCE=..\src\MDI\TagPacketGenerator.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=..\src\AMDemodulation.cpp
# End Source File
# Begin Source File

SOURCE=..\src\AMSSDemodulation.cpp
# End Source File
# Begin Source File

SOURCE=..\src\sound\audiofilein.cpp
# End Source File
# Begin Source File

SOURCE=..\src\sourcedecoders\AudioSourceDecoder.cpp
# End Source File
# Begin Source File

SOURCE=..\src\interleaver\BlockInterleaver.cpp
# End Source File
# Begin Source File

SOURCE=..\src\drmchannel\ChannelSimulation.cpp
# End Source File
# Begin Source File

SOURCE=..\src\DataIO.cpp
# End Source File
# Begin Source File

SOURCE=..\src\DrmReceiver.cpp
# End Source File
# Begin Source File

SOURCE=..\src\DRMSignalIO.cpp
# End Source File
# Begin Source File

SOURCE=..\src\DrmSimulation.cpp
# End Source File
# Begin Source File

SOURCE=..\src\DrmTransmitter.cpp
# End Source File
# Begin Source File

SOURCE=..\src\InputResample.cpp
# End Source File
# Begin Source File

SOURCE=..\src\IQInputFilter.cpp
# End Source File
# Begin Source File

SOURCE=..\src\MSCMultiplexer.cpp
# End Source File
# Begin Source File

SOURCE=..\src\OFDM.cpp
# End Source File
# Begin Source File

SOURCE=..\src\windows\Pacer.cpp
# End Source File
# Begin Source File

SOURCE=..\src\Parameter.cpp
# End Source File
# Begin Source File

SOURCE=..\src\PlotManager.cpp
# End Source File
# Begin Source File

SOURCE=..\src\ReceptLog.cpp
# End Source File
# Begin Source File

SOURCE=..\src\resample\Resample.cpp
# End Source File
# Begin Source File

SOURCE=..\src\resample\ResampleFilter.cpp
# End Source File
# Begin Source File

SOURCE=..\src\Scheduler.cpp
# End Source File
# Begin Source File

SOURCE=..\src\ServiceInformation.cpp
# End Source File
# Begin Source File

SOURCE=..\src\SimulationParameters.cpp
# End Source File
# Begin Source File

SOURCE=..\src\windows\Sound.cpp
# End Source File
# Begin Source File

SOURCE=..\src\interleaver\SymbolInterleaver.cpp
# End Source File
# Begin Source File

SOURCE=..\src\tables\TableStations.cpp
# End Source File
# Begin Source File

SOURCE=..\src\TextMessage.cpp
# End Source File
# Begin Source File

SOURCE=..\src\Version.cpp
# End Source File
# End Group
# Begin Group "Header-Dateien"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Group "Header GUI, App"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\src\GUI-QT\AnalogDemDlg.h"
# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\DialogUtil.h"
# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\qt2\DRMPlot.h"
# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\EPGDlg.h"
# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\fdrmdialog.h"
# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\fmdialog.h"
# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\GeneralSettingsDlg.h"
# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\LiveScheduleDlg.h"
# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\Logging.h"
# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\MultColorLED.h"
# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\MultimediaDlg.h"
# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\MultSettingsDlg.h"
# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\Rig.h"
# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\StationsDlg.h"
# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\systemevalDlg.h"
# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\TransmDlg.h"
# End Source File
# End Group
# Begin Group "Header MLC"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\mlc\BitInterleaver.h
# End Source File
# Begin Source File

SOURCE=..\src\mlc\ChannelCode.h
# End Source File
# Begin Source File

SOURCE=..\src\mlc\ConvEncoder.h
# End Source File
# Begin Source File

SOURCE=..\src\mlc\EnergyDispersal.h
# End Source File
# Begin Source File

SOURCE=..\src\mlc\Metric.h
# End Source File
# Begin Source File

SOURCE=..\src\mlc\MLC.h
# End Source File
# Begin Source File

SOURCE=..\src\mlc\QAMMapping.h
# End Source File
# Begin Source File

SOURCE=..\src\mlc\ViterbiDecoder.h
# End Source File
# End Group
# Begin Group "Header Tables"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\tables\TableAMSS.h
# End Source File
# Begin Source File

SOURCE=..\src\tables\TableCarMap.h
# End Source File
# Begin Source File

SOURCE=..\src\tables\TableDRMGlobal.h
# End Source File
# Begin Source File

SOURCE=..\src\tables\TableFAC.h
# End Source File
# Begin Source File

SOURCE=..\src\tables\TableMLC.h
# End Source File
# Begin Source File

SOURCE=..\src\tables\TableQAMMapping.h
# End Source File
# Begin Source File

SOURCE=..\src\tables\TableStations.h
# End Source File
# End Group
# Begin Group "Header FAC"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\FAC\FAC.h
# End Source File
# End Group
# Begin Group "Header SDC"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\SDC\SDC.h
# End Source File
# End Group
# Begin Group "Header sync"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\sync\FreqSyncAcq.h
# End Source File
# Begin Source File

SOURCE=..\src\sync\SyncUsingPil.h
# End Source File
# Begin Source File

SOURCE=..\src\sync\TimeSync.h
# End Source File
# Begin Source File

SOURCE=..\src\sync\TimeSyncFilter.h
# End Source File
# Begin Source File

SOURCE=..\src\sync\TimeSyncTrack.h
# End Source File
# End Group
# Begin Group "Header ChannelEstimation"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\chanest\ChanEstTime.h
# End Source File
# Begin Source File

SOURCE=..\src\chanest\ChannelEstimation.h
# End Source File
# Begin Source File

SOURCE=..\src\chanest\IdealChannelEstimation.h
# End Source File
# Begin Source File

SOURCE=..\src\chanest\TimeLinear.h
# End Source File
# Begin Source File

SOURCE=..\src\chanest\TimeWiener.h
# End Source File
# End Group
# Begin Group "Header Matlib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\matlib\Matlib.h
# End Source File
# Begin Source File

SOURCE=..\src\matlib\MatlibSigProToolbox.h
# End Source File
# Begin Source File

SOURCE=..\src\matlib\MatlibStdToolbox.h
# End Source File
# End Group
# Begin Group "Header OFDMCellMapping"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\ofdmcellmapping\CellMappingTable.h
# End Source File
# Begin Source File

SOURCE=..\src\ofdmcellmapping\OFDMCellMapping.h
# End Source File
# End Group
# Begin Group "Header datadecoding"

# PROP Default_Filter ".h"
# Begin Group "Header epg"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\datadecoding\epg\EPG.h
# End Source File
# Begin Source File

SOURCE=..\src\datadecoding\epg\epgdec.h
# End Source File
# Begin Source File

SOURCE=..\src\datadecoding\epg\epgutil.h
# End Source File
# End Group
# Begin Group "Header journaline"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\datadecoding\journaline\cpplog.h
# End Source File
# Begin Source File

SOURCE=..\src\datadecoding\journaline\crc_8_16.h
# End Source File
# Begin Source File

SOURCE=..\src\datadecoding\journaline\dabdatagroupdecoder.h
# End Source File
# Begin Source File

SOURCE=..\src\datadecoding\journaline\dabdgdec_impl.h
# End Source File
# Begin Source File

SOURCE=..\src\datadecoding\journaline\log.h
# End Source File
# Begin Source File

SOURCE=..\src\datadecoding\journaline\newsobject.h
# End Source File
# Begin Source File

SOURCE=..\src\datadecoding\journaline\newssvcdec.h
# End Source File
# Begin Source File

SOURCE=..\src\datadecoding\journaline\newssvcdec_impl.h
# End Source File
# Begin Source File

SOURCE=..\src\datadecoding\journaline\NML.h
# End Source File
# Begin Source File

SOURCE=..\src\datadecoding\journaline\Splitter.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\src\sound\audiofilein.h
# End Source File
# Begin Source File

SOURCE=..\src\datadecoding\DABMOT.h
# End Source File
# Begin Source File

SOURCE=..\src\datadecoding\DataDecoder.h
# End Source File
# Begin Source File

SOURCE=..\src\datadecoding\DataEncoder.h
# End Source File
# Begin Source File

SOURCE=..\src\datadecoding\Experiment.h
# End Source File
# Begin Source File

SOURCE=..\src\datadecoding\Journaline.h
# End Source File
# Begin Source File

SOURCE=..\src\datadecoding\MOTSlideShow.h
# End Source File
# Begin Source File

SOURCE=..\src\Scheduler.h
# End Source File
# End Group
# Begin Group "Header Utilities"

# PROP Default_Filter ".h"
# Begin Source File

SOURCE=..\src\util\AudioFile.h
# End Source File
# Begin Source File

SOURCE=..\src\util\Buffer.h
# End Source File
# Begin Source File

SOURCE=..\src\util\CRC.h
# End Source File
# Begin Source File

SOURCE=..\src\util\LogPrint.h
# End Source File
# Begin Source File

SOURCE=..\src\util\Modul.h
# End Source File
# Begin Source File

SOURCE=..\src\util\Reassemble.h
# End Source File
# Begin Source File

SOURCE=..\src\util\Settings.h
# End Source File
# Begin Source File

SOURCE=..\src\util\Utilities.h
# End Source File
# Begin Source File

SOURCE=..\src\util\Vector.h
# End Source File
# End Group
# Begin Group "Header MDI"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\MDI\AFPacketGenerator.h
# End Source File
# Begin Source File

SOURCE=..\src\sourcedecoders\AudioSourceEncoder.h
# End Source File
# Begin Source File

SOURCE=..\src\MDI\MDIDecode.h
# End Source File
# Begin Source File

SOURCE=..\src\MDI\MDIDefinitions.h
# End Source File
# Begin Source File

SOURCE=..\src\MDI\MDIInBuffer.h
# End Source File
# Begin Source File

SOURCE=..\src\MDI\MDIRSCI.h
# End Source File
# Begin Source File

SOURCE=..\src\MDI\MDITagItemDecoders.h
# End Source File
# Begin Source File

SOURCE=..\src\MDI\MDITagItems.h
# End Source File
# Begin Source File

SOURCE=..\src\MDI\PacketInOut.h
# End Source File
# Begin Source File

SOURCE=..\src\MDI\PacketSinkFile.h
# End Source File
# Begin Source File

SOURCE=..\src\MDI\PacketSocketNull.h
# End Source File
# Begin Source File

SOURCE=..\src\MDI\PacketSocketQT.h
# End Source File
# Begin Source File

SOURCE=..\src\MDI\PacketSourceFile.h
# End Source File
# Begin Source File

SOURCE=..\src\MDI\Pft.h
# End Source File
# Begin Source File

SOURCE=..\src\MDI\RCITagItems.h
# End Source File
# Begin Source File

SOURCE=..\src\MDI\RSCITagItemDecoders.h
# End Source File
# Begin Source File

SOURCE=..\src\MDI\RSISubscriber.h
# End Source File
# Begin Source File

SOURCE=..\src\MDI\TagItemDecoder.h
# End Source File
# Begin Source File

SOURCE=..\src\MDI\TagPacketDecoder.h
# End Source File
# Begin Source File

SOURCE=..\src\MDI\TagPacketDecoderMDI.h
# End Source File
# Begin Source File

SOURCE=..\src\MDI\TagPacketDecoderRSCIControl.h
# End Source File
# Begin Source File

SOURCE=..\src\MDI\TagPacketGenerator.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\src\AMDemodulation.h
# End Source File
# Begin Source File

SOURCE=..\src\AMSSDemodulation.h
# End Source File
# Begin Source File

SOURCE=..\src\sourcedecoders\AudioSourceDecoder.h
# End Source File
# Begin Source File

SOURCE=..\src\interleaver\BlockInterleaver.h
# End Source File
# Begin Source File

SOURCE=..\src\drmchannel\ChannelSimulation.h
# End Source File
# Begin Source File

SOURCE=..\src\DataIO.h
# End Source File
# Begin Source File

SOURCE=..\src\DrmReceiver.h
# End Source File
# Begin Source File

SOURCE=..\src\DRMSignalIO.h
# End Source File
# Begin Source File

SOURCE=..\src\DrmSimulation.h
# End Source File
# Begin Source File

SOURCE=..\src\DrmTransmitter.h
# End Source File
# Begin Source File

SOURCE=..\src\GlobalDefinitions.h
# End Source File
# Begin Source File

SOURCE=..\src\InputResample.h
# End Source File
# Begin Source File

SOURCE=..\src\IQInputFilter.h
# End Source File
# Begin Source File

SOURCE=..\src\MSCMultiplexer.h
# End Source File
# Begin Source File

SOURCE=..\src\OFDM.h
# End Source File
# Begin Source File

SOURCE=..\src\util\Pacer.h
# End Source File
# Begin Source File

SOURCE=..\src\Parameter.h
# End Source File
# Begin Source File

SOURCE=..\src\PlotManager.h
# End Source File
# Begin Source File

SOURCE=..\src\ReceptLog.h
# End Source File
# Begin Source File

SOURCE=..\src\resample\Resample.h
# End Source File
# Begin Source File

SOURCE=..\src\resample\ResampleFilter.h
# End Source File
# Begin Source File

SOURCE=..\src\selectioninterface.h
# End Source File
# Begin Source File

SOURCE=..\src\ServiceInformation.h
# End Source File
# Begin Source File

SOURCE=..\src\sound.h
# End Source File
# Begin Source File

SOURCE=..\src\windows\Sound.h
# End Source File
# Begin Source File

SOURCE=..\src\soundinterface.h
# End Source File
# Begin Source File

SOURCE=..\src\sound\soundnull.h
# End Source File
# Begin Source File

SOURCE=..\src\interleaver\SymbolInterleaver.h
# End Source File
# Begin Source File

SOURCE=..\src\TextMessage.h
# End Source File
# Begin Source File

SOURCE=..\src\Version.h
# End Source File
# End Group
# Begin Group "Ressourcendateien"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE="..\src\GUI-QT\res\MainIcon.ico"
# End Source File
# End Group
# Begin Source File

SOURCE="..\src\GUI-QT\AboutDlgbase.ui"
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\FDRM.rc
# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\fdrmdialogbase.ui"
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\fmdialogbase.ui"
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\README
# End Source File
# Begin Source File

SOURCE="..\src\GUI-QT\systemevalDlgbase.ui"
# PROP Exclude_From_Build 1
# End Source File
# End Target
# End Project
