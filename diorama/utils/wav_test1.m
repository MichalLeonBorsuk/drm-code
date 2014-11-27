
% test continous recording and playing

Fs = 48000;	%samplingrate
Ch = 1;	%channels
playtime_seconds = 30*1;

N_filterorder = 4;
Wn = 0.5;
[B,A] = BUTTER(N_filterorder,Wn);
Zi = zeros(max(length(A),length(B))-1,Ch);

blocksize = 5000;	%in samples
N_blocks = floor(playtime_seconds*Fs/blocksize);

%stop running threads
clear mex

%reset/start recording thread
x = wavrecordex(blocksize, Fs, Ch );
x = x.*[1:size(x,2)]/size(x,2); % volume fade in
sleep(500);	%let the recording go ahead some time

tic;
for(k=1:N_blocks)
   
   [x,Zi] = filter(B,A,x,Zi);
   
   [bufferlevel] = wavplayex(x,Fs);
   if ((k>1) && (bufferlevel==0)) warning('wavplayex: bufferlevel underrun'); end
   
   x = wavrecordex(blocksize, Fs, Ch );
   
   drawnow;
end

x = x.*[size(x,2):-1:1]/size(x,2); % volume fade out
wavplayex(x,Fs);




   
   
   
 
