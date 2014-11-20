
d = dir;
N = size(d,1);

for k = [3:N],
   if ( d(k).isdir )
      cd( d(k).name );
      if ( exist('make.m') == 2 )
        if (isequal(computer,'PCWIN'))
           delete *.dll
        elseif (isequal(computer,'GLNX86')|isequal(computer,'LNX86'))
            delete *.mexglx
            delete *.o
        end
      end
      cd('..');
   end
end
