function string = jourtest(filename)

nml_fid=fopen(filename,'rb');
nml_data=uint8(fread(nml_fid,inf,'uint8'));
fclose(nml_fid);

string = journaline_decode(nml_data);
