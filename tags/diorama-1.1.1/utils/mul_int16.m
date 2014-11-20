function y = mul_int16( x, A_int16 )
%MUL_INT16 multiplies vector x with matrix A_int16 of type 'int16'
%
%   y = transpose( x*double(A_int16)/(2^15) )
%   The integer values of A_int16 represent a fixed-comma value in the range [-1..(1-2^-15)]

%only called, if corresponding mex-function does not exist!
y = transpose( x*double(A_int16)/(2^15) );
