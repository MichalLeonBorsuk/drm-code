function make(varargin)

if (nargin == 0)
    compflags = '';
elseif ((nargin == 1) & (ischar(varargin{1})))
    compflags = varargin{1};
else
    error(['Too many arguments for ', pwd , filesep,mfilename, '.m']);
end

if( exist('mul_int16.c') )
   fprintf(1,'mul_int16... ');
   if (isequal(computer,'GLNX86')|isequal(computer,'LNX86'))
    clear mul_int16.mexglx;
    
   	eval(['mex ', compflags, ' mul_int16.c -D_LINUX_']);
   else
    clear mul_int16.mexw32;
    clear mul_int16.dll;
    
   	eval(['mex ', compflags, ' mul_int16.c']);  
   end
   fprintf(1,'ok\n');
end

