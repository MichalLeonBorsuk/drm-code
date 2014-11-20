function make(varargin)

if (nargin == 0)
    compflags = '';
elseif ((nargin == 1) && (ischar(varargin{1})))
    compflags = varargin{1};
else
    error(['Too many arguments for ', pwd , filesep,mfilename, '.m']);
end

if( exist('drm_aacdecode.c') )
   fprintf(1,'drm_aacdecode... ');
   if (isequal(computer,'PCWIN'))

      clear drm_aacdecode.dll;
      clear drm_aacdecode.mexw32;
      
      if (~exist('libfaad2.dll'))
          cd ('faad2\libfaad');
          
          [dos_status, dos_result] = dos('nmake /help')
          if (dos_status ~= 0)
              error ([pwd, filesep,mfilename, '.m: Cannot find nmake on your search path']);
          end
          
          [dos_status, dos_result] = dos('nmake /f libfaad2_dll.mak CFG="libfaad2_dll - Win32 Release"');
          [dos_status, dos_result] = dos ('copy ReleaseDLL\libfaad2.dll ..\..\');
          [dos_status, dos_result] = dos ('copy ReleaseDLL\libfaad2.lib ..\..\');
          cd ('..\..'); 
      end
      eval(['mex ', compflags, ' drm_aacdecode.c libfaad2.lib']);
   elseif (isequal(computer,'GLNX86')||isequal(computer,'LNX86')||isequal(computer,'x86_64-pc-linux-gnu'))
       
      clear drm_aacdecode.mex;
       
      eval(['mex ', compflags, ' drm_aacdecode.c -lfaad_drm -D_LINUX_']); 
   end
   fprintf(1,'ok\n');
end
