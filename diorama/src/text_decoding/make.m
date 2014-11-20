function make(varargin)

if (nargin == 0)
    compflags = '';
elseif ((nargin == 1) && (ischar(varargin{1})))
    compflags = varargin{1};
else
    error(['Too many arguments for ', pwd , filesep,mfilename, '.m']);
end

if (isequal(computer,'PCWIN'))
	clear deflate_uncompress.dll
	clear deflate_uncompress.mexw32
	clear journaline_decode.dll
	clear journaline_decode.mexw32

	fprintf(1,'zlib... ');
	if (~exist('zlib1.dll') || ~exist('zdll.lib'))
	  cd ('journaline_20040318\zlib-1.2.1');
	  
	  [dos_status, dos_result] = dos('nmake /help');
	  if (dos_status ~= 0)
	      error ([pwd, filesep,mfilename, '.m: Cannot find nmake on your search path']);
	  end
	  
	  [dos_status, dos_result] = dos('nmake -f win32\Makefile.msc zlib1.dll');
	  [dos_status, dos_result] = dos ('copy zlib1.dll ..\..\');
	  [dos_status, dos_result] = dos ('copy zdll.lib ..\..\');
	  cd ('..\..'); 
	end
	fprintf(1,'ok\n');
	fprintf(1,'deflate_uncompress... ');
	eval(['mex ', compflags, ' deflate_uncompress.c -Ijournaline_20040318\zlib-1.2.1 zdll.lib']);
	fprintf(1,'ok\n');
	fprintf(1,'journaline_decode... ');             
	eval(['mex ', compflags, ' journaline_decode.cpp -DWIN32 -DFHG_USEFLATDIRSTRUCT -Ijournaline_20040318 -Ijournaline_20040318\zlib-1.2.1 journaline_20040318\zlib-1.2.1\zdll.lib journaline_20040318\dabdgdec_impl.c journaline_20040318\newsobject.cpp journaline_20040318\newssvcdec_impl.cpp journaline_20040318\crc_8_16.c journaline_20040318\NML.cpp journaline_20040318\Splitter.cpp journaline_20040318\log.c']);
	fprintf(1,'ok\n');

elseif (isequal(computer,'GLNX86')||isequal(computer,'LNX86')||isequal(computer,'x86_64-pc-linux-gnu'))
	clear deflate_uncompress.mex
	clear journaline_decode.mex
	fprintf(1,'deflate_uncompress... ');
	eval(['mex ', compflags, ' deflate_uncompress.c -lz -D_LINUX_']);
	fprintf(1,'ok\n');
	fprintf(1,'journaline_decode... ');
	[ unix_status, unix_result] = unix('make');
	eval(['mex ', compflags, ' journaline_decode.cpp  crc_8_16.o dabdgdec_impl.o log.o newsobject.o newssvcdec_impl.o NML.o Splitter.o -lstdc++ -lz -D_LINUX_']);
	fprintf(1,'ok\n');

end
