
d = dir;
N = size(d,1);

for k = [3:N],
   if ( d(k).isdir )
      cd( d(k).name );
      if ( exist('make.m') == 2 )
         if (isequal(computer,'PCWIN'))
            [dos_status, dos_result] = dos('copy *.dll ..\..');
            [dos_status, dos_result] = dos('copy *.mexw32 ..\..');
        elseif (isequal(computer,'GLNX86')|isequal(computer,'LNX86'))
            [dos_status, dos_result] = unix('cp *.mexglx ../../');
        end
      end
      cd('..');
   end
end
