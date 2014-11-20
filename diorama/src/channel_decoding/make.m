function make(varargin)

if (nargin == 0)
    compflags = '';
elseif ((nargin == 1) && (ischar(varargin{1})))
    compflags = varargin{1};
else
    error(['Too many arguments for ', pwd , filesep,mfilename, '.m']);
end

if (isequal(computer,'GLNX86')||isequal(computer,'LNX86')||isequal(computer,'x86_64-pc-linux-gnu'))
    
    clear msd_hard.mexglx;
    
   if( (exist('viterbi_decode.c')) && (exist('msd_hard.c')) )
      fprintf(1,'msd_hard... ');
      
      eval(['mex ', compflags , ' msd_hard.c viterbi_decode.c -D_LINUX_']);
      
      fprintf(1,'ok\n');
   end
   
elseif (isequal(computer,'PCWIN'))
    
    clear msd_hard.dll;
    clear msd_hard.mexw32;

   if( (exist('viterbi_decode.c')) && (exist('msd_hard.c')) )
      fprintf(1,'msd_hard... ');
      eval(['mex ',compflags, ' msd_hard.c viterbi_decode.c']);
      fprintf(1,'ok\n');
   end
   
end
