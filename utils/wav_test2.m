
% test quality of soundcard and the modi 44.1 kHz and 48 kHz
% you have to switch your soundcard in a closed loop (mixer-setting "sum")

Ch = 1;	%channels
N_fft = 2^16;
N_delay = N_fft/8;
x_sequence = [0,1/sqrt(2),1,1/sqrt(2),0,-1/sqrt(2),-1,-1/sqrt(2)]*0.5;   
x = reshape( x_sequence'*ones(1,N_fft/length(x_sequence)), 1, N_fft );


%stop running threads
clear mex

Fs = 48000;	%samplingrate
wavplayex(x,Fs);
wavplayex(x,Fs);
y48 = wavrecordex(N_delay, Fs, Ch );
y48 = wavrecordex(N_fft, Fs, Ch );
if( max( abs(y48) )>0.9 ) warning('peak recorded sound value >0.9!'); end

Y48 = fft(y48);
figure(48000);
plot( 1e6*[-50:50]/(N_fft/8), 20*log10( abs(Y48(N_fft/8+1+[-50:50])) ), '.-' );
title('Powerspectrum - samplingrate: 48 kHz');
xlabel('in/out freq. offset (ppm)');
ylabel('powerspectrum (dB)'); 
figure(48001);
plot( [1:N_fft]/N_fft*Fs, 20*log10( abs(Y48) ), '.-' );
title('Powerspectrum - samplingrate: 48 kHz');
xlabel('frequency (Hz)');
ylabel('powerspectrum (dB)'); 
figure(48002);
plot( [1:N_fft]/Fs, y48, '-' );
axis( [0,N_fft/Fs,-1,1] );
title('recorded signal - samplingrate: 48 kHz');
xlabel('time (s)');
ylabel('amplitude (-1..1)'); 


%stop running threads
clear mex

Fs = 44100;
wavplayex(x,Fs);
wavplayex(x,Fs);
y44 = wavrecordex(N_delay, Fs, Ch );
y44 = wavrecordex(N_fft, Fs, Ch );
if( max( abs(y44) )>0.9 ) warning('peak recorded sound value >0.9!'); end

Y44 = fft(y44);
figure(44100);
plot( 1e6*[-50:50]/(N_fft/8), 20*log10( abs(Y44(N_fft/8+1+[-50:50])) ), '.-' );
title('Samplingrate: 44.1 kHz');
xlabel('in/out freq. offset (ppm)');
ylabel('powerspectrum (dB)'); 
figure(44101);
plot( [1:N_fft]/N_fft*Fs, 20*log10( abs(Y44) ), '.-' );
title('Powerspectrum - samplingrate: 44.1 kHz');
xlabel('frequency (Hz)');
ylabel('powerspectrum (dB)'); 
figure(44102);
plot( [1:N_fft]/Fs, y44, '-' );
axis( [0,N_fft/Fs,-1,1] );
title('recorded signal - samplingrate: 44.1 kHz');
xlabel('time (s)');
ylabel('amplitude (-1..1)'); 


