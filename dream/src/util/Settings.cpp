/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2007
 *
 * Author(s):
 *	Volker Fischer, Tomi Manninen, Stephane Fillod, Robert Kesterson,
 *	Andrea Russo, Andrew Murphy
 *
 * Description:
 *
 * 10/01/2007
 *  - parameters for rsci by Andrew Murphy
 * 07/27/2004
 *  - included stlini routines written by Robert Kesterson
 * 04/15/2004 Tomi Manninen, Stephane Fillod
 *  - Hamlib
 * 10/03/2003 Tomi Manninen / OH2BNS
 *  - Initial (basic) code for command line argument parsing (argv)
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#include "Settings.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdlib>
using namespace std;

/* Implementation *************************************************************/
void
CSettings::Load(int argc, char **argv)
{
	/* First load settings from init-file and then parse command line arguments.
	   The command line arguments overwrite settings in init-file! */
	LoadIni(DREAM_INIT_FILE_NAME);
	ParseArguments(argc, argv);
}

void
CSettings::Save()
{
	/* Just write settings in init-file */
	SaveIni(DREAM_INIT_FILE_NAME);
}

string
CSettings::Get(const string & section, const string & key, const string & def) const
{
	return GetIniSetting(section, key, def);
}

void
CSettings::Put(const string & section, const string & key, const string& value)
{
	PutIniSetting(section, key, value);
}

bool
CSettings::Get(const string & section, const string & key, const bool def) const
{
	return GetIniSetting(section, key, def?"1":"0")=="1";
}

void CSettings::Put(const string& section, const string& key, const bool value)
{
	PutIniSetting(section, key, value?"1":"0");
}

int
CSettings::Get(const string & section, const string & key, const int def) const
{
	const string strGetIni = GetIniSetting(section, key);

	/* Check if it is a valid parameter */
	if (strGetIni.empty())
		return def;

	stringstream s(strGetIni);
	int iValue;
	s >> iValue;
	return iValue;
}

void
CSettings::Put(const string & section, const string & key, const int value)
{
	stringstream s;
	s << value;
	PutIniSetting(section, key, s.str());
}

_REAL
CSettings::Get(const string & section, const string & key, const _REAL def) const
{
	string s = GetIniSetting(section, key, "");
	if(s != "")
	{
		stringstream ss(s);
		_REAL rValue;
		ss >> rValue;
		return rValue;
	}
	return def;
}

void
CSettings::Put(const string & section, const string & key, const _REAL value)
{
	stringstream s;
	s << setiosflags(ios::left);
	s << setw(11);
	s << setiosflags(ios::fixed);
	s << setprecision(7);
	s << value;
	PutIniSetting(section, key, s.str());
}

void
CSettings::Get(const string& section, CWinGeom& value) const
{
	value.iXPos = Get(section, "x", 0);
	value.iYPos = Get(section, "y", 0);
	value.iWSize = Get(section, "width", 0);
	value.iHSize = Get(section, "height", 0);
}

void
CSettings::Put(const string& section, const CWinGeom& value)
{
	stringstream s;
	s << value.iXPos;
	PutIniSetting(section, "x", s.str());
	s.str("");
	s << value.iYPos;
	PutIniSetting(section, "y", s.str());
	s.str("");
	s << value.iWSize;
	PutIniSetting(section, "width", s.str());
	s.str("");
	s << value.iHSize;
	PutIniSetting(section, "height", s.str());
	s.str("");
}

int
CSettings::IsReceiver(const char *argv0)
{
#ifdef EXECUTABLE_NAME
	/* Give the possibility to launch directly dream transmitter
	   with a symbolic link to the executable, a 't' need to be 
	   appended to the symbolic link name */
# define _xstr(s) _str(s)
# define _str(s) #s
# ifndef _WIN32
	const int pathseparator = '/';
# else
	const int pathseparator = '\\';
# endif
	const char *str = strrchr(argv0, pathseparator);
	return strcmp(str ? str+1 : argv0, _xstr(EXECUTABLE_NAME) "t") != 0;
#else
	(void)argv0;
	return TRUE;
#endif
}

void
CSettings::FileArg(const string& str)
{
	// Identify the type of file
	string ext;
	size_t p = str.rfind('.');
	if (p != string::npos)
		ext = str.substr(p + 1);
	if (ext.substr(0,2) == "RS" || ext.substr(0,2) == "rs" || ext.substr(0,4) == "pcap")
	{
		// it's an RSI or MDI input file
		Put("command", "rsiin", str);
	}
	else
	{
		// its an I/Q or I/F file
		Put("command", "fileio", str);
	}
}
/* Command line argument parser ***********************************************/
void
CSettings::ParseArguments(int argc, char **argv)
{
	_BOOLEAN bIsReceiver;
	_REAL rArgument;
	string strArgument;
	int rsioutnum = 0;
	int rciinnum = 0;

	bIsReceiver = IsReceiver(argv[0]);

	/* QT docu: argv()[0] is the program name, argv()[1] is the first
	   argument and argv()[argc()-1] is the last argument.
	   Start with first argument, therefore "i = 1" */
	if (bIsReceiver)
	{
		for (int i = 1; i < argc; i++)
		{
			/* DRM transmitter mode flag ---------------------------------------- */
			if (GetFlagArgument(argc, argv, i, "-t", "--transmitter") == TRUE)
			{
				bIsReceiver = FALSE;
				break;
			}
		}
	}

	const char* ReceiverTransmitter = bIsReceiver ? "Receiver" : "Transmitter";
	Put("command", "mode", bIsReceiver ? string("receive") : string("transmit"));

	for (int i = 1; i < argc; i++)
	{
		/* DRM transmitter mode flag ---------------------------------------- */
		if (GetFlagArgument(argc, argv, i, "-t", "--transmitter") == TRUE)
			continue;

		/* Sample rate ------------------------------------------------------ */
		if (GetNumericArgument(argc, argv, i, "-R", "--samplerate",
							   -1e9, +1e9, rArgument) == TRUE)
		{
			Put(ReceiverTransmitter, "samplerateaud", int (rArgument));
			Put(ReceiverTransmitter, "sampleratesig", int (rArgument));
			continue;
		}

		/* Audio sample rate ------------------------------------------------ */
		if (GetNumericArgument(argc, argv, i, "--audsrate", "--audsrate",
							   -1e9, +1e9, rArgument) == TRUE)
		{
			Put(ReceiverTransmitter, "samplerateaud", int (rArgument));
			continue;
		}

		/* Signal sample rate ------------------------------------------------ */
		if (GetNumericArgument(argc, argv, i, "--sigsrate", "--sigsrate",
							   -1e9, +1e9, rArgument) == TRUE)
		{
			Put(ReceiverTransmitter, "sampleratesig", int (rArgument));
			continue;
		}

		/* Sound In device -------------------------------------------------- */
		if (GetStringArgument(argc, argv, i, "-I", "--snddevin",
							  strArgument) == TRUE)
		{
			Put(ReceiverTransmitter, "snddevin", strArgument);
			continue;
		}

		/* Sound Out device ------------------------------------------------- */
		if (GetStringArgument(argc, argv, i, "-O", "--snddevout",
							  strArgument) == TRUE)
		{
			Put(ReceiverTransmitter, "snddevout", strArgument);
			continue;
		}

		/* Flip spectrum flag ----------------------------------------------- */
		if (GetNumericArgument(argc, argv, i, "-p", "--flipspectrum",
							   0, 1, rArgument) == TRUE)
		{
			Put("Receiver", "flipspectrum", int (rArgument));
			continue;
		}

		/* Mute audio flag -------------------------------------------------- */
		if (GetNumericArgument(argc, argv, i, "-m", "--muteaudio",
							   0, 1, rArgument) == TRUE)
		{
			Put("Receiver", "muteaudio", int (rArgument));
			continue;
		}

		/* Bandpass filter flag --------------------------------------------- */
		if (GetNumericArgument(argc, argv, i, "-F", "--filter",
							   0, 1, rArgument) == TRUE)
		{
			Put("Receiver", "filter", int (rArgument));
			continue;
		}

		/* Modified metrics flag -------------------------------------------- */
		if (GetNumericArgument(argc, argv, i, "-D", "--modmetric",
							   0, 1, rArgument) == TRUE)
		{
			Put("Receiver", "modmetric", int (rArgument));
			continue;
		}

		/* Do not use sound card, read from file ---------------------------- */
		if (GetStringArgument(argc, argv, i, "-f", "--fileio",
							  strArgument) == TRUE)
		{
			FileArg(strArgument);
			continue;
		}

		/* Write output data to file as WAV --------------------------------- */
		if (GetStringArgument(argc, argv, i, "-w", "--writewav",
							  strArgument) == TRUE)
		{
			Put("command", "writewav", strArgument);
			continue;
		}

		/* Number of iterations for MLC setting ----------------------------- */
		if (GetNumericArgument(argc, argv, i, "-i", "--mlciter", 0,
							   MAX_NUM_MLC_IT, rArgument) == TRUE)
		{
			Put("Receiver", "mlciter", int (rArgument));
			continue;
		}

		/* Sample rate offset start value ----------------------------------- */
		if (GetNumericArgument(argc, argv, i, "-s", "--sampleoff",
							   MIN_SAM_OFFS_INI, MAX_SAM_OFFS_INI,
							   rArgument) == TRUE)
		{
			Put("Receiver", "sampleoff", rArgument);
			continue;
		}

		/* Frequency acquisition search window size ------------------------- */
		if (GetNumericArgument(argc, argv, i, "-S", "--fracwinsize", 0,
							   MAX_FREQ_AQC_SE_WIN_SZ, rArgument) == TRUE)
		{
			Put("command", "fracwinsize", rArgument);
			continue;
		}

		/* Frequency acquisition search window center ----------------------- */
		if (GetNumericArgument(argc, argv, i, "-E", "--fracwincent", 0,
							   MAX_FREQ_AQC_SE_WIN_CT, rArgument) == TRUE)
		{
			Put("command", "fracwincent", rArgument);
			continue;
		}

		/* Input channel selection ------------------------------------------ */
		if (GetNumericArgument(argc, argv, i, "-c", "--inchansel", 0,
							   MAX_VAL_IN_CHAN_SEL, rArgument) == TRUE)
		{
			Put("Receiver", "inchansel", (int) rArgument);
			continue;
		}

		/* Output channel selection ----------------------------------------- */
		if (GetNumericArgument(argc, argv, i, "-u", "--outchansel", 0,
							   MAX_VAL_OUT_CHAN_SEL, rArgument) == TRUE)
		{
			Put("Receiver", "outchansel", (int) rArgument);
			continue;
		}

		/* Wanted RF Frequency   -------------------------------------------- */
		if (GetNumericArgument(argc, argv, i, "-r", "--frequency", 0,
							   MAX_RF_FREQ, rArgument) == TRUE)
		{
			Put("Receiver", "frequency", (int) rArgument);
			continue;
		}

		/* if 0 then only measure PSD when RSCI in use otherwise always measure it ---- */
		if (GetNumericArgument(argc, argv, i, "--enablepsd", "--enablepsd", 0, 1,
							   rArgument) == TRUE)
		{
			Put("Receiver", "measurepsdalways", (int) rArgument);
			continue;
		}

#ifdef _WIN32
		/* Enable/Disable process priority flag ----------------------------- */
		if (GetNumericArgument(argc, argv, i, "-P", "--processpriority", 0, 1,
							   rArgument) == TRUE)
		{
			Put("command", "processpriority", (int) rArgument);
			continue;
		}
#endif

		/* Enable/Disable epg decoding -------------------------------------- */
		if (GetNumericArgument(argc, argv, i, "-e", "--decodeepg", 0,
							   1, rArgument) == TRUE)
		{
			Put("EPG", "decodeepg", (int) rArgument);
			continue;
		}

#ifdef USE_QT_GUI /* QThread needed for log file timing */

		/* log enable flag  ------------------------------------------------- */
		if (GetNumericArgument(argc, argv, i, "-g", "--enablelog", 0, 1,
							   rArgument) == TRUE)
		{
			Put("Logfile", "enablelog", (int) rArgument);
			continue;
		}

		/* log file delay value  -------------------------------------------- */
		if (GetNumericArgument(argc, argv, i, "-l", "--logdelay", 0,
							   MAX_SEC_LOG_FI_START, rArgument) == TRUE)
		{
			Put("Logfile", "delay", (int) rArgument);
			continue;
		}

		/* read DRMlog.ini style schedule file ------------------------------ */
		if (GetStringArgument(argc, argv, i, "-L", "--schedule", strArgument) == TRUE)
		{
			Put("command", "schedule", strArgument);
			continue;
		}

		/* Latitude string for log file ------------------------------------- */
		if (GetStringArgument(argc, argv, i, "-a", "--latitude",
							  strArgument) == TRUE)
		{
			Put("Logfile", "latitude", strArgument);
			continue;
		}

		/* Longitude string for log file ------------------------------------ */
		if (GetStringArgument(argc, argv, i, "-o", "--longitude",
							  strArgument) == TRUE)
		{
			Put("Logfile", "longitude", strArgument);
			continue;
		}


		/* Plot Style main plot --------------------------------------------- */
		if (GetNumericArgument(argc, argv, i, "-y", "--sysevplotstyle", 0,
							   MAX_COLOR_SCHEMES_VAL, rArgument) == TRUE)
		{
			Put("System Evaluation Dialog", "plotstyle", (int) rArgument);
			continue;
		}


#endif
		/* MDI out address -------------------------------------------------- */
		if (GetStringArgument(argc, argv, i, "--mdiout", "--mdiout",
							  strArgument) == TRUE)
		{
			cerr <<
				"content server mode not implemented yet, perhaps you wanted rsiout"
				<< endl;
			continue;
		}

		/* MDI in address --------------------------------------------------- */
		if (GetStringArgument(argc, argv, i, "--mdiin", "--mdiin",
							  strArgument) == TRUE)
		{
			cerr <<
				"modulator mode not implemented yet, perhaps you wanted rsiin"
				<< endl;
			continue;
		}

		/* RSCI status output profile --------------------------------------- */
		if (GetStringArgument(argc, argv, i, "--rsioutprofile", "--rsioutprofile",
							  strArgument) == TRUE)
		{
			for (int i = 0; i < rsioutnum; i++)
			{
				stringstream s;
				s << "rsioutprofile" << i;
				if(ini["command"].count(s.str()) == 0)
					Put("command", s.str(), strArgument);
			}
			continue;
		}

		/* RSCI status out address ------------------------------------------ */
		if (GetStringArgument(argc, argv, i, "--rsiout", "--rsiout",
							  strArgument) == TRUE)
		{
			stringstream s;
			s << "rsiout" << rsioutnum;
			Put("command", s.str(), strArgument);
			rsioutnum++;
			continue;
		}

		/* RSCI status in address ------------------------------------------- */
		if (GetStringArgument(argc, argv, i, "--rsiin", "--rsiin",
							  strArgument) == TRUE)
		{
			Put("command", "rsiin", strArgument);
			continue;
		}

		/* RSCI control out address ----------------------------------------- */
		if (GetStringArgument(argc, argv, i, "--rciout", "--rciout",
							  strArgument) == TRUE)
		{
			Put("command", "rciout", strArgument);
			continue;
		}

		/* OPH: RSCI control in address ------------------------------------- */
		if (GetStringArgument(argc, argv, i, "--rciin", "--rciin",
							  strArgument) == TRUE)
		{
			stringstream s;
			s << "rciin" << rciinnum;
			Put("command", s.str(), strArgument);
			rciinnum++;
			continue;
		}

		if (GetStringArgument (argc, argv, i,
				"--rsirecordprofile", "--rsirecordprofile", strArgument) == TRUE)
		{
			Put("command", "rsirecordprofile", strArgument);
			continue;
		}

		if (GetStringArgument (argc, argv, i,
				"--rsirecordtype", "--rsirecordtype", strArgument) == TRUE)
		{
			Put("command", "rsirecordtype", strArgument);
			continue;
		}

		if (GetNumericArgument (argc, argv, i,
				"--recordiq", "--recordiq", 0, 1, rArgument) == TRUE)
		{
			Put("command", "recordiq", int (rArgument));
			continue;
		}

#ifdef HAVE_LIBHAMLIB
		/* Hamlib config string --------------------------------------------- */
		if (GetStringArgument(argc, argv, i, "-C", "--hamlib-config",
							  strArgument) == TRUE)
		{
			Put("Hamlib", "hamlib-config", strArgument);
			continue;
		}

		/* Hamlib Model ID -------------------------------------------------- */
		if (GetNumericArgument(argc, argv, i, "-M", "--hamlib-model", 0,
							   MAX_ID_HAMLIB, rArgument) == TRUE)
		{
			Put("Hamlib", "hamlib-model", int (rArgument));
			continue;
		}

		/* Enable s-meter flag ---------------------------------------------- */
		if (GetNumericArgument(argc, argv, i, "-T", "--ensmeter",
							   0, 1, rArgument) == TRUE)
		{
			Put("Hamlib", "ensmeter", int (rArgument));
			continue;
		}
#endif

		/* Help (usage) flag ------------------------------------------------ */
		if ((!strcmp(argv[i], "--help")) ||
			(!strcmp(argv[i], "-h")) || (!strcmp(argv[i], "-?")))
		{
			Put("command", "mode", "help");
			continue;
		}

		/* not an option ---------------------------------------------------- */
		if(argv[i][0] != '-')
		{
			FileArg(string(argv[i]));
			continue;
		}

		/* Unknown option --------------------------------------------------- */
		cerr << argv[0] << ": ";
		cerr << "Unknown option '" << argv[i] << "' -- use '--help' for help"
			<< endl;

		exit(1);
	}
}

const char *
CSettings::UsageArguments()
{
	/* The text below must be translatable */
	return
		"Usage: $EXECNAME [option [argument]]\n"
		"\n"
		"Recognized options:\n"
		"  -t, --transmitter            DRM transmitter mode\n"
		"  -p <b>, --flipspectrum <b>   flip input spectrum (0: off; 1: on)\n"
		"  -i <n>, --mlciter <n>        number of MLC iterations (allowed range: 0...4 default: 1)\n"
		"  -s <r>, --sampleoff <r>      sample rate offset initial value [Hz] (allowed range: -200.0...200.0)\n"
		"  -m <b>, --muteaudio <b>      mute audio output (0: off; 1: on)\n"
		"  -f <s>, --fileio <s>         disable sound card, use file <s> instead\n"
		"  -w <s>, --writewav <s>       write output to wave file\n"
		"  -S <r>, --fracwinsize <r>    freq. acqu. search window size [Hz] (-1.0: sample rate / 2 (default))\n"
		"  -E <r>, --fracwincent <r>    freq. acqu. search window center [Hz] (-1.0: sample rate / 4 (default))\n"
		"  -F <b>, --filter <b>         apply bandpass filter (0: off; 1: on)\n"
		"  -D <b>, --modmetric <b>      enable modified metrics (0: off; 1: on)\n"
		"  -c <n>, --inchansel <n>      input channel selection\n"
		"                               0: left channel;                     1: right channel;\n"
		"                               2: mix both channels (default);      3: subtract right from left;\n"
		"                               4: I / Q input positive;             5: I / Q input negative;\n"
		"                               6: I / Q input positive (0 Hz IF);   7: I / Q input negative (0 Hz IF)\n"
		"  -u <n>, --outchansel <n>     output channel selection\n"
		"                               0: L -> L, R -> R (default);   1: L -> L, R muted;   2: L muted, R -> R\n"
		"                               3: mix -> L, R muted;          4: L muted, mix -> R\n"
		"  -e <n>, --decodeepg <b>      enable/disable epg decoding (0: off; 1: on)\n"
#ifdef USE_QT_GUI
		"  -g <n>, --enablelog <b>      enable/disable logging (0: no logging; 1: logging\n"
		"  -r <n>, --frequency <n>      set frequency [kHz] for log file\n"
		"  -l <n>, --logdelay <n>       delay start of logging by <n> seconds, allowed range: 0...3600)\n"
		"  -L <s>, --schedule <s>       read DRMlog.ini style schedule file and obey it\n"
		"  -y <n>, --sysevplotstyle <n> set style for main plot\n"
		"                               0: blue-white (default);   1: green-black;   2: black-grey\n"
#endif
		"  --enablepsd <b>              if 0 then only measure PSD when RSCI in use otherwise always measure it\n"
		"  --mdiout <s>                 MDI out address format [IP#:]IP#:port (for Content Server)\n"
		"  --mdiin  <s>                 MDI in address (for modulator) [[IP#:]IP:]port\n"
		"  --rsioutprofile <s>          MDI/RSCI output profile: A|B|C|D|Q|M\n"
		"  --rsiout <s>                 MDI/RSCI output address format [IP#:]IP#:port (prefix address with 'p' to enable the simple PFT)\n"
		"  --rsiin <s>                  MDI/RSCI input address format [[IP#:]IP#:]port\n"
		"  --rciout <s>                 RSCI Control output format IP#:port\n"
		"  --rciin <s>                  RSCI Control input address number format [IP#:]port\n"
		"  --rsirecordprofile <s>       RSCI recording profile: A|B|C|D|Q|M\n"
		"  --rsirecordtype <s>          RSCI recording file type: raw|ff|pcap\n"
		"  --recordiq <b>               enable/disable recording an I/Q file\n"
		"  -R <n>, --samplerate <n>     set audio and signal sound card sample rate [Hz]\n"
		"  --audsrate <n>               set audio sound card sample rate [Hz] (allowed range: 8000...192000)\n"
		"  --sigsrate <n>               set signal sound card sample rate [Hz] (allowed values: 24000, 48000, 96000, 192000)\n"
		"  -I <s>, --snddevin <s>       set sound in device\n"
		"  -O <s>, --snddevout <s>      set sound out device\n"
#ifdef HAVE_LIBHAMLIB
		"  -M <n>, --hamlib-model <n>   set Hamlib radio model ID\n"
		"  -C <s>, --hamlib-config <s>  set Hamlib config parameter\n"
		"  -T <b>, --ensmeter <b>       enable S-Meter (0: off; 1: on)\n"
#endif
#ifdef _WIN32
		"  -P, --processpriority <b>    enable/disable high priority for working thread\n"
#endif
		"  -h, -?, --help               this help text\n"
		"\n"
		"Example: $EXECNAME -p --sampleoff -0.23 -i 2"
#ifdef USE_QT_GUI
		" -r 6140 --rsiout 127.0.0.1:3002"
#endif
		"";
}

_BOOLEAN
CSettings::GetFlagArgument(int, char **argv, int &i,
						   string strShortOpt, string strLongOpt)
{
	if ((!strShortOpt.compare(argv[i])) || (!strLongOpt.compare(argv[i])))
		return TRUE;
	else
		return FALSE;
}

_BOOLEAN
CSettings::GetStringArgument(int argc, char **argv, int &i,
							 string strShortOpt, string strLongOpt,
							 string & strArg)
{
	if ((!strShortOpt.compare(argv[i])) || (!strLongOpt.compare(argv[i])))
	{
		if (++i >= argc)
		{
			cerr << argv[0] << ": ";
			cerr << "'" << strLongOpt << "' needs a string argument" << endl;
			exit(1);
		}

		strArg = argv[i];

		return TRUE;
	}
	else
		return FALSE;
}

_BOOLEAN
CSettings::GetNumericArgument(int argc, char **argv, int &i,
							  string strShortOpt, string strLongOpt,
							  _REAL rRangeStart, _REAL rRangeStop,
							  _REAL & rValue)
{
	if ((!strShortOpt.compare(argv[i])) || (!strLongOpt.compare(argv[i])))
	{
		if (++i >= argc)
		{
			cerr << argv[0] << ": ";
			cerr << "'" << strLongOpt << "' needs a numeric argument between "
				<< rRangeStart << " and " << rRangeStop << endl;
			exit(1);
		}

		char *p;
		rValue = strtod(argv[i], &p);
		if (*p || rValue < rRangeStart || rValue > rRangeStop)
		{
			cerr << argv[0] << ": ";
			cerr << "'" << strLongOpt << "' needs a numeric argument between "
				<< rRangeStart << " and " << rRangeStop << endl;
			exit(1);
		}

		return TRUE;
	}
	else
		return FALSE;
}

/* INI File routines using the STL ********************************************/
/* The following code was taken from "INI File Tools (STLINI)" written by
   Robert Kesterson in 1999. The original files are stlini.cpp and stlini.h.
   The homepage is http://robertk.com/source

   Copyright August 18, 1999 by Robert Kesterson */

#ifdef _MSC_VER
/* These pragmas are to quiet VC++ about the expanded template identifiers
   exceeding 255 chars. You won't be able to see those variables in a debug
   session, but the code will run normally */
#pragma warning (push)
#pragma warning (disable : 4786 4503)
#endif

string
CIniFile::GetIniSetting(const string& section,
	 const string& key, const string& defaultval) const
{
	string result(defaultval);
	const_cast<CMutex*>(&Mutex)->Lock();
	INIFile::const_iterator iSection = ini.find(section);
	if (iSection != ini.end())
	{
		INISection::const_iterator apair = iSection->second.find(key);
		if (apair != iSection->second.end())
			result = apair->second;
	}
	const_cast<CMutex*>(&Mutex)->Unlock();
	return result;
}

void
CIniFile::PutIniSetting(const string& section, const string& key, const string& value)
{
	Mutex.Lock();

	/* null key is ok and empty value is ok but empty both is not useful */
	if(key != "" || value != "")
		ini[section][key]=value;

	Mutex.Unlock();
}

void
CIniFile::LoadIni(const char *filename)
{
	string section;
	fstream file(filename, ios::in);

	while (file.good())
	{
		if(file.peek() == '[')
		{
			file.ignore(); // read '['
			getline(file, section, ']');
			file.ignore(80, '\n'); // read eol
		}
		else if(file.peek() == '\n')
		{
			file.ignore(10, '\n'); // read eol
		}
		else
		{
			string key,value;
			getline(file, key, '=');
			getline(file, value);
			int n = int(value.length())-1;
			if(n >= 0)
			{
				if(value[n] == '\r') // remove CR if file has DOS line endings
					PutIniSetting(section, key, value.substr(0,n));
				else
					PutIniSetting(section, key, value);
			}
		}
	}
}

void
CIniFile::SaveIni(const char *filename)
{
	_BOOLEAN bFirstSection = TRUE;	/* Init flag */

	std::fstream file(filename, std::ios::out);
	if (!file.good())
		return;

	/* Just iterate the hashes and values and dump them to a file */
	for(INIFile::iterator section = ini.begin(); section != ini.end(); section++)
	{
		if (section->first != "command")
		{
			if (section->first > "")
			{
				if (bFirstSection == TRUE)
				{
					/* Don't put a newline at the beginning of the first section */
					file << "[" << section->first << "]" << std::endl;

					/* Reset flag */
					bFirstSection = FALSE;
				}
				else
					file << std::endl << "[" << section-> first << "]" << std::endl;
			}

			INISection::iterator pair = section->second.begin();

			while (pair != section->second.end())
			{
				if (pair->second > "")
					file << pair->first << "=" << pair->second << std::endl;
				else
					file << pair->first << "=" << std::endl;
				pair++;
			}
		}
	}
	file.close();
}

/* Return true or false depending on whether the first string is less than the
   second */
bool
StlIniCompareStringNoCase::operator() (const string & x, const string & y)
	 const
	 {
#ifdef _WIN32
		 return (_stricmp(x.c_str(), y.c_str()) < 0) ? true : false;
#else
#ifdef strcasecmp
		 return (strcasecmp(x.c_str(), y.c_str()) < 0) ? true : false;
#else
		 unsigned nCount = 0;
		 int nResult = 0;
		 const char *p1 = x.c_str();
		 const char *p2 = y.c_str();

		 while (*p1 && *p2)
		 {
			 nResult = toupper(*p1) - toupper(*p2);
			 if (nResult != 0)
				 break;
			 p1++;
			 p2++;
			 nCount++;
		 }
		 if (nResult == 0)
		 {
			 if (*p1 && !*p2)
				 nResult = -1;
			 if (!*p1 && *p2)
				 nResult = 1;
		 }
		 if (nResult < 0)
			 return true;
		 return false;
#endif /* strcasecmp */
#endif
	 }

#ifdef _MSC_VER
#pragma warning(pop)
#endif
