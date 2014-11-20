function setup(varargin)

if (nargin == 0)
    compflags = '';
elseif ((nargin == 1) & (ischar(varargin{1})))
    compflags = varargin{1};
else
    error(['Too many arguments for ', pwd , filesep,mfilename, '.m']);
end


clear mex;
cd ('src');
eval('make_clean');
eval('make(varargin{:})');
eval('make_install');
cd ('..');

cd (['utils',filesep,'src']);
eval('make_clean');
eval('make(varargin{:})');
eval('make_install');
cd ('..');cd ('..');

equalization_init('c');