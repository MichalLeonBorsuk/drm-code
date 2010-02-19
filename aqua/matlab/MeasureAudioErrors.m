function AudioFER = MeasureAudioErrors(RxAudioFile, RxAudioFileClean)

blockSize = 960000; % samples (at 48kHz) to process each time

ReferenceAudioFile = '..\AudioFiles\ktestchirp40ms50h5kmsquare2.wav';

fileSizeSamples=wavread(RxAudioFile,'size');

numBlocks = floor(fileSizeSamples(1)/blockSize);

%40ms square chirp  
refAudio=wavread(ReferenceAudioFile);
refAudioSize = size(refAudio,1);

% Measure correlation peaks on clean channel to act as reference
cleanRxAudio = wavread(RxAudioFileClean);
size(cleanRxAudio)
size(refAudio)
yxcorr=xcorr(cleanRxAudio,refAudio);
xcorrout=abs(yxcorr);   %correlation result
maxvalue = max(xcorrout); % Max value of correlation for clean signal - used to determine threshold
maxvalueb=0.7*maxvalue; % Threshold
figure(1);plot(xcorrout); 
 
for iii=1:numBlocks
    fprintf(1, 'Processing block %d of %d\n', iii, numBlocks);
    s=blockSize*(iii-1)+1;
    t=blockSize*iii;
      
   %aaa=wavread('C:\K\drmwork\RxTesting\wavfiles\test3adb29.wav',[s t]);
    audioBlock=wavread(RxAudioFile,[s t]);  
 
 


%%%%%%%%%%%%%Actual tests samples %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


% Cross correlation tests with signals

   yxcorr=xcorr(audioBlock,refAudio);

   % xcorr result is 2*blockSize-1 ranging from -(blockSize-1) to (blockSize-1). 
   % So element i is lag i-blockSize
   % We want 0 to blockSize-refAudioSize-1 which is elements blockSize to 2*blockSize-refAudioSize-1
   xcorrout=abs(yxcorr(blockSize:2*blockSize-refAudioSize-1));   %correlation result
   figure(2);plot (xcorrout);

%Algorithm

countpeak(iii)=0;
        
isize(iii)=length(xcorrout);

j=0;

a=find(xcorrout >= maxvalueb);

countpeak(iii) = length(a);

count(iii) = sum(diff(a)>=1900);

countpeak(iii)
count(iii)

fprintf(1,'iii=%d countpeak=%d count=%d\n',iii,countpeak(iii),count(iii));

correctnumb(iii)=isize(iii)/refAudioSize - 1; % Subtract 1 because the first peak will always be missed
ERP(iii)=(correctnumb(iii)-count(iii));
PER(iii)=ERP(iii)/correctnumb(iii);

end

ERPsum= sum (ERP);

AudioFER=ERPsum/sum(correctnumb)
AUDIOQ= 100- (100*AudioFER);

% figure (1)
% plot(xcorrout);
% figure (2)
% plot(yxcorr);
% figure (3)
% plot(ysq40msclean);

