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
    
    clear crc8.mex;
    clear crc16.mex;
    clear crc16_bytewise.mex;
    
	if (exist('crc8.c'))
	fprintf(1,'crc8... ');
	eval(['mex ', compflags, ' crc8.c -D_LINUX_']);
	fprintf(1,'ok\n');
	end

	if (exist('crc16.c'))
	fprintf(1,'crc16... ');
	eval(['mex ', compflags, ' crc16.c -D_LINUX_']);
	fprintf(1,'ok\n');
	end

	if (exist('crc16_bytewise.c'))
	fprintf(1,'crc16_bytewise... ');
	eval(['mex ', compflags, ' crc16_bytewise.c -D_LINUX_']);
	fprintf(1,'ok\n');
	end
elseif (isequal(computer,'PCWIN'))
	% compile, if its not already done

    clear crc8.mexw32;
    clear crc16.mexw32;
    clear crc16_bytewise.mexw32;
    clear crc8.dll;
    clear crc16.dll;
    clear crc16_bytewise.dll;

    if (exist('crc8.c'))
	fprintf(1,'crc8... ');
	eval(['mex ', compflags, ' crc8.c']);
	fprintf(1,'ok\n');
	end

	if (exist('crc16.c'))
	fprintf(1,'crc16... ');
	eval(['mex ', compflags, ' crc16.c']);
	fprintf(1,'ok\n');
	end

	if (exist('crc16_bytewise.c'))
	fprintf(1,'crc16_bytewise... ');
	eval(['mex ', compflags, ' crc16_bytewise.c']);
	fprintf(1,'ok\n');
	end
end
