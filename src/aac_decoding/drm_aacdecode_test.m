
clear all;
load PrepAudio.mat

no_of_audioframes = length(PrepAudioFrame);

y = [];
for (k=1:no_of_audioframes),
   y = [y;drm_aacdecode(uint8(PrepAudioFrame{k}),24000,4)];
end


wavplay(y,48000);
