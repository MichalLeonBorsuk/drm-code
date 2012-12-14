/******************************************************************************\
 *
 * Copyright (c) 2012
 *
 * Author(s):
 *	David Flamand
 *
 * Description:
 *	Dynamic Link Library Loader
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

#ifdef _WIN32
# include "windows.h"
# define LOADLIB(a) (void*)LoadLibraryA(a)
# define GETPROC(a, b) (void*)GetProcAddress((HMODULE)a, b)
# define FREELIB(a, b) FreeLibrary((HMODULE)a)
#else
# define LOADLIB(a) dlopen(a, RTLD_LOCAL | RTLD_NOW)
# define GETPROC(a, b) dlsym(a, b)
# define FREELIB(a) dlclose(a)
#endif

typedef struct LIBFUNC
{
	const char *pcFunctionName;
	void **ppvFunctionAddress;
	void *pvDummyFunctionAddress;
} LIBFUNC;

class CLibraryLoader
{
public:
	static void* Load(const char** LibraryNames, LIBFUNC* LibFunc)
	{
		void* hLib = NULL;
		for (int l = 0; LibraryNames[l]; l++)
		{
			hLib = LOADLIB(LibraryNames[l]);
			if (hLib != NULL)
			{
				int f;
				for (f = 0; LibFunc[f].pcFunctionName; f++)
				{
					void *pvFunc = GETPROC(hLib, LibFunc[f].pcFunctionName);
					if (!pvFunc)
						break;
					*LibFunc[f].ppvFunctionAddress = pvFunc;
				}
				if (!LibFunc[f].pcFunctionName)
					break;
				FREELIB(hLib);
				hLib = NULL;
			}
		}
		if (hLib == NULL)
		{
			for (int f = 0; LibFunc[f].pcFunctionName; f++)
				*LibFunc[f].ppvFunctionAddress = LibFunc[f].pvDummyFunctionAddress;
		}
		return hLib;
	}
	static void Free(void* hLib)
	{
		if (hLib != NULL)
			FREELIB(hLib);
	}
};
