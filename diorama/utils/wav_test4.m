
% get transfer function of soundcard-out-in-loop
% you have to switch your soundcard in a closed loop (mixer-setting "sum")

Fs = 48000;	%samplingrate
Ch = 1;	%channels

measuretime_ms = 50;
guardtime_ms = 5;
mean_play_buffer_ms = 500;

N_points = 50;
f_min = 200;
f_max = 24000;
frequency_scale_log = 0;
sin_amplitude = 0.9;




EPSILON = 10^(-10);

N = floor( measuretime_ms/1000*Fs  /2+1)*2;
Ng = floor( guardtime_ms/1000*Fs  /2+1)*2;

if (frequency_scale_log == 1)
	f_grid = f_min*exp( [0:(N_points-1)]/N_points*log(f_max/f_min) );
else
   f_grid = f_min + [0:(N_points-1)]/N_points*(f_max-f_min);
end

f = zeros(size(f_grid));
H = zeros(size(f));

Omega = 2*pi*floor( N*f_grid/Fs )/N;
f = Omega*Fs/(2*pi);
N_block = N+Ng;
k = [0:(N_block-1)];

%stop running threads
clear mex

N_intro = floor(Fs/4);
x_intro = randn(1,N_intro);	%noise sequence
x_intro_int16 = int16(x_intro*2^12);

wavplayex( x_intro_int16, Fs); %start playing thread
wavrecordex(1, Fs, Ch); %start recording thread

% start feeding data to playing-thread
N_f_delay = floor(mean_play_buffer_ms/measuretime_ms)+1; %let playing be always at least 500ms ahead
Phi = 0;
for ( f_index = [1:N_f_delay] )   
   x = sin_amplitude*sin( Omega(f_index)*k + Phi );
   Phi = rem( Omega(f_index)*(k(end)+1) + Phi, 2*pi );
   level = wavplayex(x,Fs);
   if ( level==0 ) warning('wavplay buffer underun'); end
end

%estimate play-record delay
N_intro_window = 200;
wavrecordex(N_intro/2, Fs, Ch);	%wait, so we are in the mid of the noise sequence
x_intro_window = wavrecordex(N_intro_window, Fs, Ch);	%get noise sequence window
x_ccf = filter( fliplr(x_intro_window), 1, x_intro ); %cross correlation
[a,b] = max(abs(x_ccf));
delay = N_intro - (b-N_intro_window) -   N_intro_window    +      floor(Ng/2);

if ((delay<0) | (delay>N_intro))
   fprintf(1,'unable to estimate play-record delay\n');
   return;
end

%adjust recording stream
wavrecordex(delay, Fs, Ch);	%throw away samples


%measurement loop
Phi2 = 0;
for ( f_index = [1:length(f)] )
   
   if (f_index+N_f_delay<=length(f))
	   x = sin_amplitude*sin( Omega(f_index+N_f_delay)*k  + Phi );
   	Phi = rem( Omega(f_index+N_f_delay)*(k(end)+1) + Phi, 2*pi );
      level = wavplayex(x,Fs);
      if ( level==0 ) warning('wavplay buffer underun'); end
   end
   
   y = wavrecordex(N_block, Fs, Ch);
    
   x_complex = exp( j*Omega(f_index)*(k+Ng/2) + j*Phi2 );
   Phi2 = rem( Omega(f_index)*(k(end)+1) + Phi2, 2*pi );
   
   H(f_index) = x_complex(1:N)*transpose(y(1:N))/N;   
   
end


figure(1);
subplot(2,1,1);
if (frequency_scale_log == 1)
   semilogx(f/1000,20*log10(abs(H) + EPSILON ),'.-');
else
   plot(f/1000,20*log10(abs(H) + EPSILON ),'.-');
end

axis([0,f_max/1000,-50,0]);
grid on;
title('mono out, mono in');
xlabel('frequency (kHz)');
ylabel('power (dB)');

subplot(2,1,2);
phase_rotator = exp( -j*angle(H(1)) );
if (frequency_scale_log == 1)
   semilogx(f/1000,angle(H*phase_rotator)/pi,'.-');
else
   plot(f/1000,angle(H*phase_rotator)/pi,'.-');
end

axis([0,f_max/1000,-1.1,1.1]);
grid on;
title('mono out, mono in');
xlabel('frequency (kHz)');
ylabel('phase (pi)');
