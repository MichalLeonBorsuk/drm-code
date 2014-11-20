function make(varargin)

if (nargin == 0)
    compflags = '';
elseif ((nargin == 1) && (ischar(varargin{1})))
    compflags = varargin{1};
else
    error(['Too many arguments for ', pwd , filesep,mfilename, '.m']);
end


if (isequal(computer,'GLNX86')||isequal(computer,'LNX86')||isequal(computer,'x86_64-pc-linux-gnu'))
    
    clear bits2bytes.mexglx;
    clear bytes2bits.mexglx;
    clear clockex.mexglx
    clear hist2D_equidist.mexglx
    
	if( exist('bits2bytes.c') )
	fprintf(1,'bits2bytes... ');
	eval(['mex ', compflags, ' bits2bytes.c -D_LINUX_']);
	fprintf(1,'ok\n');
	end

	if( exist('bytes2bits.c') )
	fprintf(1,'bytes2bits... ');
	eval(['mex ',compflags, ' bytes2bits.c -D_LINUX_']);
	fprintf(1,'ok\n');
	end
	if( exist('clockex.c') )
	fprintf(1,'clockex... ');
	eval(['mex ', compflags, ' clockex.c -D_LINUX_']);
	fprintf(1,'ok\n');
	end
    
    if( exist('hist2D_equidist.c') )
	fprintf(1,'hist2D_equidist... ');
	eval(['mex ', compflags,' hist2D_equidist.c -D_LINUX_']);
	fprintf(1,'ok\n');
	end

elseif (isequal(computer,'PCWIN'))

    clear bits2bytes.mexw32;
    clear bytes2bits.mexw32;
    clear clockex.mexw32
    clear hist2D_equidist.mexw32
    clear bits2bytes.dll;
    clear bytes2bits.dll;
    clear clockex.dll
    clear hist2D_equidist.dll

    
    if( exist('bits2bytes.c') )
	fprintf(1,'bits2bytes... ');
	eval(['mex ', compflags, ' bits2bytes.c']);
	fprintf(1,'ok\n');
	end

	if( exist('bytes2bits.c') )
	fprintf(1,'bytes2bits... ');
	eval(['mex ', compflags, ' bytes2bits.c']);
	fprintf(1,'ok\n');
	end
   
	if( exist('clockex.c') )
	fprintf(1,'clockex... ');
	eval(['mex ', compflags, ' clockex.c']);
	fprintf(1,'ok\n');
	end
    
    if( exist('hist2D_equidist.c') )
	fprintf(1,'hist2D_equidist... ');
	eval(['mex ', compflags, ' hist2D_equidist.c']);
	fprintf(1,'ok\n');
	end
end
