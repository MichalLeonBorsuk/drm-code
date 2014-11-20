
Folder \faad2 is a CVS snapshot from FAAD2 decoder from 04. April 2005

plus the files

"libfaad2.def", "libfaad2_dll.dep", "libfaad2_dll.dsp", "libfaad2_dll.dsw", "libfaad2_dll.mak"


To build the Library "libfaad2.dll" under Windows for use with Diorama:
-----------------------------------------------------------------------
nmake /f libfaad2_dll.mak CFG="libfaad2_dll - Win32 Release"
