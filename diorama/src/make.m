function make(varargin)

if (nargin == 0)
    compflags = '';
elseif ((nargin == 1) && (ischar(varargin{1})))
    compflags = varargin{1};
else
    error(['Too many arguments for ', pwd , filesep,mfilename, '.m']);
end

d = dir;
N = size(d,1);

for k = [3:N],
   if ( d(k).isdir )
      cd( d(k).name );
      if ( exist('make.m') == 2 )
         make(varargin{:});
      end
      cd('..');
   end
end
