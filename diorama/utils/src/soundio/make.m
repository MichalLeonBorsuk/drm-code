function make(varargin)

if (nargin == 0)
    compflags = '';
elseif ((nargin == 1) && (ischar(varargin{1})))
    compflags = varargin{1};
else
    error(['Too many arguments for ', pwd , filesep,mfilename, '.m']);
end

if (isequal(computer,'GLNX86')||isequal(computer,'LNX86')||isequal(computer,'x86_64-pc-linux-gnu'))
	% compile, if its not already done
    
    clear wavio_linux.mex;
    clear sleep.mex;
    
	if (exist('wavio_linux.c') && exist('src_filter.c') & exist('src_filter.h') && exist('src_filter_table.h'))
	fprintf(1,'wavio_linux... ');
	eval(['mex ', compflags, ' wavio_linux.c src_filter.c -D_LINUX_']);
	fprintf(1,'ok\n');
	end

	if( exist('sleep.c') )
	fprintf(1,'sleep... ');
	eval(['mex ', compflags, ' sleep.c -D_LINUX_']);
	fprintf(1,'ok\n');
	end
elseif (isequal(computer,'PCWIN'))
    
    clear wavplay_directx.mexw32;
    clear wavrecord_directx.mexw32;
    clear sleep.mexw32;
    clear wavplay_directx.dll;
    clear wavrecord_directx.dll;
    clear sleep.dll;
    
	% compile, if its not already done
	if (exist('wavplay_directx.c') & exist('src_filter.c') && exist('src_filter.h') && exist('src_filter_table.h'))
	fprintf(1,'wavplay_directx... ');
    eval(['mex ', compflags, ' wavplay_directx.c src_filter.c dynamicbuffer.c user32.lib dsound.lib winmm.lib MSVCRT.lib']);
	fprintf(1,'ok\n');
	end

	if (exist('wavrecord_directx.c') && exist('src_filter.c') && exist('src_filter.h') && exist('src_filter_table.h'))
	fprintf(1,'wavrecord_directx... ');
	eval(['mex ', compflags, ' wavrecord_directx.c src_filter.c user32.lib dsound.lib winmm.lib libcmt.lib']);
	fprintf(1,'ok\n');
	end

	if( exist('sleep.c') )
	fprintf(1,'sleep... ');
	eval(['mex ', compflags, ' sleep.c']);
	fprintf(1,'ok\n');
	end
end

