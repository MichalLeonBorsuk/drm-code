function RunAudioError(testCaseNum, RxName, SNR, recordingPath)

RxAudioFile = [recordingPath '/RxAudio_' RxName '_' testCaseNum '_' SNR '.wav'];
fprintf(1,'File: %s\n',RxAudioFile);
RxAudioFileClean = ['Z:/RxTestingAudio/RxAudioClean_' RxName '_' testCaseNum '.wav'];


AudioFER = MeasureAudioErrors(RxAudioFile, RxAudioFileClean);

fid = fopen (['../results/' RxName '_results.txt'], 'a');
fprintf(fid, 'Case: %s\tSNR: %s\t FER: %d\n', testCaseNum, SNR, AudioFER);
fclose(fid);