
% test continous playing

Fs = 48000;	%samplingrate
Ch = 1;	%channels
playtime_seconds = 3*1;

f_base = 10;


amplitudes = zeros(1, floor(Fs/2/f_base));
amplitudes(1+20) = 0.2;
amplitudes(1+40) = 1;
amplitudes(1+60) = 0.9;
amplitudes(1+80) = 0.8;
amplitudes(1+100) = 0.2;
amplitudes(1+120) = 0.3;

N_tones = length(amplitudes);

N_fft = floor( Fs/2/f_base/N_tones )*N_tones;

X_left = reshape( [amplitudes; zeros(N_fft/N_tones-1,N_tones)], 1, N_fft );
X = [0,X_left(2:end),0,fliplr(X_left(2:end))];
x = real( ifft(X) );
x_int16 = int16( x/max(x)*0.9*(2^15-1) );
x_start_int16 = int16( ([1:length(x)]/length(x)).*x /max(x)*0.9*(2^15-1) );
x_stop_int16 = int16( ([length(x):-1:1]/length(x)).*x /max(x)*0.9*(2^15-1) );


%stop running threads
%clear mex

wavplayex(zeros(1,floor(Fs/4)),Fs);
wavplayex(x_start_int16,Fs);

N_cycles = floor( playtime_seconds*Fs/length(x) );
for(k=1:N_cycles)
   [bufferlevel_ms, delay_diff, delay_offset] = wavplayex(x_int16, Fs);
   fprintf(1,'bufferlevel: %5.0f ms, delay_diff: %2.0f smp, delay_offset: %2.3f smp\n',bufferlevel_ms, delay_diff, delay_offset); 
   %sleep( max(0,bufferlevel_ms-3000) );
   drawnow;
end
wavplayex(x_stop_int16,Fs);

 
