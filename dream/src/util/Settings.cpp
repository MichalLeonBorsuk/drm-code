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
#include <limits>
#include <cstdlib>
#include <cstring>

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

void CSettings::Clear()
{
    ini.clear();
}

void CSettings::Clear(const string& section)
{
    INIFile::iterator s = ini.find(section);
    if(s!=ini.end())
	ini.erase(s);
}

void CSettings::Clear(const string& section, const string& key)
{
    INIFile::iterator s = ini.find(section);
    if(s!=ini.end())
    {
	INISection::iterator k = s->second.find(key);
	if(k!=s->second.end())
	    s->second.erase(k);
    }
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

/* Command line argument parser ***********************************************/
void
CSettings::ParseArguments(int argc, char **argv)
{
	_REAL rArgument;
	string strArgument;
	int mdioutnum = 0;
	int rsioutnum = 0;
	int rciinnum = 0;

	/* QT docu: argv()[0] is the program name, argv()[1] is the first
	   argument and argv()[argc()-1] is the last argument.
	   Start with first argument, therefore "i = 1" */
	for (int i = 1; i < argc; i++)
	{
		/* Mode ----------------------------------------------------------------- */
		if (GetStringArgument(argc, argv, i, "--mode", "--mode", strArgument) == true)
		{
			Put("0", "mode", strArgument);
			continue;
		}

		/* Modulation ----------------------------------------------------------- */
		if (GetStringArgument(argc, argv, i, "--modn", "--modn", strArgument) == true)
		{
			Put("Receiver", "modulation", strArgument);
			continue;
		}

		/* Flip spectrum flag ----------------------------------------------- */
		if (GetFlagArgument(argc, argv, i, "-p", "--flipspectrum") == true)
		{
			Put("Receiver", "flipspectrum", 1);
			continue;
		}

		/* Mute audio flag -------------------------------------------------- */
		if (GetFlagArgument(argc, argv, i, "-m", "--muteaudio") == true)
		{
			Put("Receiver", "muteaudio", 1);
			continue;
		}

		/* Bandpass filter flag --------------------------------------------- */
		if (GetFlagArgument(argc, argv, i, "-F", "--filter") == true)
		{
			Put("Receiver", "filter", 1);
			continue;
		}

		/* Modified metrics flag -------------------------------------------- */
		if (GetFlagArgument(argc, argv, i, "-D", "--modmetric") == true)
		{
			Put("Input-DRM", "modmetric", 1);
			continue;
		}

		/* Sound In device -------------------------------------------------- */
		if (GetNumericArgument(argc, argv, i, "-I", "--snddevin", -1,
							   MAX_NUM_SND_DEV, rArgument) == true)
		{
			Put("Receiver", "snddevin", int (rArgument));
			continue;
		}

		/* Sound Out device ------------------------------------------------- */
		if (GetNumericArgument(argc, argv, i, "-O", "--snddevout", -1,
							   MAX_NUM_SND_DEV, rArgument) == true)
		{
			Put("Receiver", "snddevout", int (rArgument));
			continue;
		}

		/* Do not use sound card, read from file ---------------------------- */
		if (GetStringArgument(argc, argv, i, "-f", "--fileio",
							  strArgument) == true)
		{
			Put("command", "fileio", strArgument);
			continue;
		}

		/* Write output data to file as WAV --------------------------------- */
		if (GetStringArgument(argc, argv, i, "-w", "--writewav",
							  strArgument) == true)
		{
			Put("Receiver", "writewav", strArgument);
			continue;
		}

		/* Number of iterations for MLC setting ----------------------------- */
		if (GetNumericArgument(argc, argv, i, "-i", "--mlciter", 0,
							   MAX_NUM_MLC_IT, rArgument) == true)
		{
			Put("Input-DRM", "mlciter", int (rArgument));
			continue;
		}

		/* Sample rate offset start value ----------------------------------- */
		if (GetNumericArgument(argc, argv, i, "-s", "--sampleoff",
							   MIN_SAM_OFFS_INI, MAX_SAM_OFFS_INI,
							   rArgument) == true)
		{
			Put("Receiver", "sampleoff", rArgument);
			continue;
		}

		/* Frequency acquisition search window size ------------------------- */
		if (GetNumericArgument(argc, argv, i, "-S", "--fracwinsize", 0,
							   MAX_FREQ_AQC_SE_WIN_SI, rArgument) == true)
		{
			Put("command", "fracwinsize", rArgument);
			continue;
		}

		/* Frequency acquisition search window center ----------------------- */
		if (GetNumericArgument(argc, argv, i, "-E", "--fracwincent", 0,
							   MAX_FREQ_AQC_SE_WIN_CEN, rArgument) == true)
		{
			Put("command", "fracwincent", rArgument);
			continue;
		}

		/* Input channel selection ------------------------------------------ */
		if (GetNumericArgument(argc, argv, i, "-c", "--inchansel", 0,
							   MAX_VAL_IN_CHAN_SEL, rArgument) == true)
		{
			Put("command", "inchansel", (int) rArgument);
			continue;
		}

		/* Output channel selection ----------------------------------------- */
		if (GetNumericArgument(argc, argv, i, "-u", "--outchansel", 0,
							   MAX_VAL_OUT_CHAN_SEL, rArgument) == true)
		{
			Put("Receiver", "outchansel", (int) rArgument);
			continue;
		}

		/* Wanted RF Frequency   ------------------------------------------- */
		if (GetNumericArgument(argc, argv, i, "-r", "--frequency", 0,
							   MAX_RF_FREQ, rArgument) == true)
		{
			Put("Receiver", "frequency", (int) rArgument);
			continue;
		}

		/* Enable/Disable process priority flag */
		if (GetNumericArgument
			(argc, argv, i, "-P", "--processpriority", 0, 1, rArgument) == true)
		{
			Put("GUI", "processpriority", (int) rArgument);
			continue;
		}

		/* enable/disable epg decoding ----------------------------------------------- */
		if (GetNumericArgument(argc, argv, i, "-e", "--decodeepg", 0,
							   1, rArgument) == true)
		{
			Put("EPG", "decodeepg", (int) rArgument);
			continue;
		}

#ifdef QT_GUI_LIB
		/* log enable flag  ---------------------------------------------- */
		if (GetNumericArgument(argc, argv, i, "-g", "--enablelog", 0, 1,
							   rArgument) == true)
		{
			Put("Logfile", "enablelog", (int) rArgument);
			continue;
		}

		/* log file delay value  ---------------------------------------------- */
		if (GetNumericArgument(argc, argv, i, "-l", "--logdelay", 0,
							   MAX_SEC_LOG_FI_START, rArgument) == true)
		{
			Put("Logfile", "delay", (int) rArgument);
			continue;
		}
#endif
		/* Latitude string for log file ------------------------------------- */
		if (GetStringArgument(argc, argv, i, "-a", "--latitude", strArgument)
			== true)
		{
			Put("GPS", "latitude", strArgument);
			continue;
		}

		/* Longitude string for log file ------------------------------------ */
		if (GetStringArgument(argc, argv, i, "-o", "--longitude", strArgument)
			== true)
		{
			Put("GPS", "longitude", strArgument);
			continue;
		}

#ifdef QT_GUI_LIB
		/* Main plot colours ------------------------------------------- */
		if (GetNumericArgument(argc, argv, i, "-y", "--sysevplotstyle", 0,
							   MAX_COLOR_SCHEMES_VAL, rArgument) == true)
		{
			Put("GUI System Evaluation", "plotstyle", (int) rArgument);
			continue;
		}
#endif

		/* MDI out address -------------------------------------------------- */
		if (GetStringArgument(argc, argv, i, "--mdiout", "--mdiout",
							  strArgument) == true)
		{
			stringstream s;
			s << "mdiout" << mdioutnum;
			Put("transmitter", s.str(), strArgument);
			mdioutnum++;
			continue;
		}

		/* MDI in address -------------------------------------------------- */
		if (GetStringArgument(argc, argv, i, "--mdiin", "--mdiin",
							  strArgument) == true)
		{
			Put("transmitter", "mdiin", strArgument);
			continue;
		}

		/* RSCI status output profile */
		if (GetStringArgument (argc, argv, i, "--rsioutprofile", "--rsioutprofile", strArgument) == true)
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

		/* RSCI status out address -------------------------------------------------- */
		if (GetStringArgument(argc, argv, i, "--rsiout", "--rsiout",
							  strArgument) == true)
		{
			stringstream s;
			s << "rsiout" << rsioutnum;
			Put("command", s.str(), strArgument);
			rsioutnum++;
			continue;
		}

		/* RSCI status in address -------------------------------------------------- */
		if (GetStringArgument(argc, argv, i, "--rsiin", "--rsiin",
							  strArgument) == true)
		{
			Put("command", "rsiin", strArgument);
			continue;
		}

		/* RSCI control out address */
		if (GetStringArgument(argc, argv, i, "--rciout", "--rciout",
							  strArgument) == true)
		{
			Put("command", "rciout", strArgument);
			continue;
		}

		/* OPH: RSCI control in address */
		if (GetStringArgument(argc, argv, i, "--rciin", "--rciin", strArgument) == true)
		{
			stringstream s;
			s << "rciin" << rciinnum;
			Put("command", s.str(), strArgument);
			rciinnum++;
			continue;
		}

		if (GetNumericArgument (argc, argv, i,
				"--recordiq", "--recordiq", 0, 1, rArgument) == true)
		{
			Put("Receiver", "writeiq", int (rArgument));
			continue;
		}

		/* Hamlib config string --------------------------------------------- */
		// command line only - will be converted and saved in Dream.ini by CHamlib
		if (GetStringArgument(argc, argv, i, "-C", "--hamlib-config",
							  strArgument) == true)
		{
			Put("command", "hamlib-config", strArgument);
			continue;
		}

		/* Hamlib Model ID -------------------------------------------------- */
		if (GetNumericArgument(argc, argv, i, "-M", "--hamlib-model", 0,
							   MAX_ID_HAMLIB, rArgument) == true)
		{
			Put("command", "hamlib-model", (int)rArgument);
			continue;
		}

		/* Enable s-meter flag ---------------------------------------------- */
		if (GetFlagArgument(argc, argv, i, "-T", "--ensmeter") == true)
		{
			Put("Hamlib", "smeter", (int)rArgument);
			continue;
		}

		/* allow -group:key <str> type arguments */
		char g[200],k[200];
		if(sscanf(argv[i], "-%[^:]:%s", g,k)==2)
		{
			Put(g, k, string(argv[++i]));
			continue;
		}

		/* Help (usage) flag ------------------------------------------------ */
		if ((!strcmp(argv[i], "--help")) ||
			(!strcmp(argv[i], "-h")) || (!strcmp(argv[i], "-?")))
		{
			Put("command", "help", 1);
			continue;
		}

		/* not an option --------------------------------------------------- */
		if(argv[i][0] != '-')
		{
			strArgument = argv[i];
			size_t p = strArgument.rfind('.');
			if (p == string::npos)
			{
				Put("command", "fileio", strArgument);
			}
			else
			{
				string strInFileExt = strArgument.substr(p + 1);
				if (strInFileExt.substr(0,2) == "RS" || strInFileExt.substr(0,2) == "rs" || strInFileExt == "pcap")
					Put("command", "rsiin", strArgument);
				else
					Put("command", "fileio", strArgument);
			}
			continue;
		}
		/* Unknown option --------------------------------------------------- */
		Put("command", "error", string(argv[i]));
		Put("command", "help", 1);
	}
}

string
CSettings::UsageArguments(char **argv)
{
// TODO: Internationalisation

	return
		"Usage: " + string(argv[0]) + " [option] [argument]\n"
		"Recognized options:\n"
		"  --mode                      operating mode: TX, MOD, ENC, RX (default)\n"
		"  --modn                      modulation type DRM (default), AM, USB, LSB, CW, NBFM, WBFM\n"
		"  -p, --flipspectrum          flip input spectrum\n"
		"  -i <n>, --mlciter <n>       number of MLC iterations (allowed range: 0...4 default: 1)\n"
		"  -s <r>, --sampleoff <r>     sample rate offset initial value [Hz] (allowed range: -200.0...200.0)\n"
		"  -m, --muteaudio             mute audio output\n"
		"  -f <s>, --fileio <s>        disable sound card, use file <s> instead\n"
		"  -w <s>, --writewav <s>      write output to wave file\n"
		"  -S <r>, --fracwinsize <r>   freq. acqu. search window size [Hz]\n"
		"  -E <r>, --fracwincent <r>   freq. acqu. search window center [Hz]\n"
		"  -F, --filter                apply bandpass filter\n"
		"  -D, --modmetric             enable modified metrics\n"
		"  -c <n>, --inchansel <n>     input channel selection\n"
		"                              0: left channel;   1: right channel;   2: mix both channels (default)\n"
		"                              3: I / Q input positive;   4: I / Q input negative\n"
		"                              5: I / Q input positive (0 Hz IF);   6: I / Q input negative (0 Hz IF)\n"
		"  -u <n>, --outchansel <n>    output channel selection\n"
		"                              0: L -> L, R -> R (default);   1: L -> L, R muted;   2: L muted, R -> R\n"
		"                              3: mix -> L, R muted;   4: L muted, mix -> R\n"
		"  -e <n>, --decodeepg <n>     enable/disable epg decoding (0: off; 1: on)\n"
#ifdef QT_GUI_LIB
		"  -g <n>, --enablelog <n>     enable/disable logging (0: no logging; 1: logging\n"
		"  -r <n>, --frequency <n>     set frequency [kHz] for log file\n"
		"  -l <n>, --logdelay <n>      delay start of logging by <n> seconds, allowed range: 0...3600)\n"
		"  -y <n>, --sysevplotstyle <n> set style for main plot\n"
		"                              0: blue-white (default);   1: green-black;   2: black-grey\n"
#endif
		"  --mdiout <s>                MDI out address format [IP#:]IP#:port (for Content Server)\n"
		"  --mdiin  <s>                MDI in address (for modulator) [[IP#:]IP:]port\n"
		"  --rsioutprofile <s>         MDI/RSCI output profile: A|B|C|D|Q|M\n"
		"  --rsiout <s>                MDI/RSCI output address format file or [IP#:]IP#:port (prefix address with 'p' to enable the simple PFT)\n"
		"  --rsiin <s>                 RSCI/MDI status input address format [[IP#:]IP#:]port\n"
		"  --rciout <s>                RSCI Control output format IP#:port\n"
		"  --rciin <n>                 RSCI Control input listening port\n"
		"  -I <n>, --snddevin <n>      set sound in device\n"
		"  -O <n>, --snddevout <n>     set sound out device\n"
#ifdef HAVE_LIBHAMLIB
		"  -M <n>, --hamlib-model <n>  set Hamlib radio model ID\n"
		"  -C <s>, --hamlib-config <s> set Hamlib config parameter\n"
#endif
		"  -T, --ensmeter              enable S-Meter\n"
#ifdef WIN32
		"  -P, --processpriority <n>   enable/disable high priority for working thread\n"
#endif
		" -group:item <s>             any config file parameter\n"
		"  -h, -?, --help             this help text\n"
		"\n"
		"Example: " + string(argv[0]) + " -p --sampleoff -0.23 -i 2 -EPG:decodeepg 1"
#ifdef QT_GUI_LIB
		" -r 6140 --rsiout 127.0.0.1:3002"
#endif
		"\n";
}

bool
CSettings::GetFlagArgument(int, char **argv, int &i,
						   string strShortOpt, string strLongOpt)
{
	if ((!strShortOpt.compare(argv[i])) || (!strLongOpt.compare(argv[i])))
		return true;
	else
		return false;
}

bool
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

		return true;
	}
	else
		return false;
}

bool
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

		return true;
	}
	else
		return false;
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
	//const_cast<CMutex*>(&Mutex)->Lock();
	INIFile::const_iterator iSection = ini.find(section);
	if (iSection != ini.end())
	{
		INISection::const_iterator apair = iSection->second.find(key);
		if (apair != iSection->second.end())
			result = apair->second;
	}
	//const_cast<CMutex*>(&Mutex)->Unlock();
	return result;
}

void
CIniFile::PutIniSetting(const string& section, const string& key, const string& value)
{
	//Mutex.Lock();

	/* null key is ok and empty value is ok but empty both is not useful */
	if(key != "" || value != "")
		ini[section][key]=value;

	//Mutex.Unlock();
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
			PutIniSetting(section, key, value);
		}
	}
}

void
CIniFile::SaveIni(const char *filename)
{
	bool bFirstSection = true;	/* Init flag */

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
				if (bFirstSection == true)
				{
					/* Don't put a newline at the beginning of the first section */
					file << "[" << section->first << "]" << std::endl;

					/* Reset flag */
					bFirstSection = false;
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
#ifdef WIN32
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
