#
# This file is part of the 'NewsService Journaline(R) Decoder'
# 
# Copyright (c) 2003, 2004 by Fraunhofer IIS, Erlangen, Germany
# 
# --------------------------------------------------------------------
# 
# For NON-COMMERCIAL USE,
# the 'NewsService Journaline(R) Decoder' is free software;
# you can redistribute it and/or modify it under the terms of
# the GNU General Public License as published by the
# Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# The 'NewsService Journaline(R) Decoder' is distributed in the hope
# that it will be useful, but WITHOUT ANY WARRANTY;
# without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with the 'NewsService Journaline(R) Decoder';
# if not, write to the Free Software Foundation, Inc.,
# 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
# 
# 
# If you use this software in a project with user interaction, please
# provide the following text to the user in an appropriate place:
# "Features NewsService Journaline(R) decoder technology by
# Fraunhofer IIS, Erlangen, Germany.
# For more information visit http://www.iis.fhg.de/dab"
# 
# --------------------------------------------------------------------
# 
# To use the 'NewsService Journaline(R) Decoder' software for
# COMMERCIAL purposes, please contact Fraunhofer IIS for a
# commercial license (see below for contact information)!
# 
# --------------------------------------------------------------------
# 
# Contact:
#   Fraunhofer IIS, Department 'Broadcast Applications'
#   Am Wolfsmantel 33, 91058 Erlangen, Germany
#   http://www.iis.fraunhofer.de/dab
#   mailto:bc-info@iis.fraunhofer.de
# 


CC=gcc
LD=g++
MAKE=/usr/bin/make
VPATH=journaline_20040318/

BINDIR = .

OBJFILES= \
	journaline_decode_dll.o dabdgdec_impl.o newsobject.o newssvcdec_impl.o crc_8_16.o \
  NML.o Splitter.o log.o

ifeq ($(HAVE_READLINE), yes)
  DEFINES = -DLINUX -DFHG_USEFLATDIRSTRUCT -DHAVE_READLINE -D_LINUX_
else
  DEFINES = -DLINUX -DFHG_USEFLATDIRSTRUCT -D_LINUX_
endif

CFLAGS=$(DEFINES) $(INCLUDES) -Wall -pedantic -Wunused -g3 -O -Wunused -c -fPIC
CXXFLAGS=$(DEFINES) $(INCLUDES) -g3 -O -Wunused -c -fPIC\
  -W -Wall -pedantic -Wformat -Wmissing-braces -Wparentheses \
  -Wno-long-long -Wold-style-cast -Woverloaded-virtual \
  -Wreturn-type -Wuninitialized -ffor-scope

dll: obj
	ar rcs libjournaline_decode_dll.a $(OBJFILES) 
  
all: obj

tests: obj

clean:
	-rm  $(OBJFILES) *~

.PHONY: obj
obj: $(OBJFILES)



