
if( (exist('zlib.h')))
   if (isequal(computer,'PCWIN'))
      fprintf(1,'zlib... ');
      if (~exist('zlib1.dll') | ~exist('zdll.lib'))
          cd ('journaline_20040318\zlib-1.2.1');
          [dos_status, dos_result] = dos('nmake -f win32\Makefile.msc zlib1.dll');
          [dos_status, dos_result] = dos ('copy zlib1.dll ..\..\');
          [dos_status, dos_result] = dos ('copy zdll.lib ..\..\');
          cd ('..\..'); 
      end
      fprintf(1,'ok\n');
      fprintf(1,'deflate_uncompress... ');
      mex deflate_uncompress.c -Ijournaline_20040318\zlib-1.2.1 zdll.lib
      fprintf(1,'ok\n');
      fprintf(1,'journaline_decode... ');  
      [dos_status, dos_result] = dos('nmake -f Makefile.msc dll');
      mex journaline_decode.c journaline_decode_dll.lib
      fprintf(1,'ok\n');
      
   elseif (isequal(computer,'GLNX86')|isequal(computer,'LNX86'))
      if (unix('/sbin/ldconfig -p | grep ''libz\.[a\|so]'' > /dev/null'))
         fprintf(1,'zlib... ');
         cd ('journaline_20040318/zlib-1.2.1');
         [unix_status, unix_result] = unix('make libz.a');
         cd ('../..');
         fprintf(1,'ok\n');
      end
      fprintf(1,'deflate_uncompress... ');
      mex deflate_uncompress.c -Ljournaline_20040318/zlib-1.2.1 -lz -D_LINUX_
      fprintf(1,'ok\n');
      fprintf(1,'journaline_decode... ');
      [unix_status, unix_result] = unix('make dll')
      mex journaline_decode.c -L./ -ljournaline_decode_dll -lz -lstdc++ -D_LINUX_  
      fprintf(1,'ok\n');
      
   end
end
