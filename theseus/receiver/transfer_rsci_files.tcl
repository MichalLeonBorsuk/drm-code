source settings.tcl

set ext ".rs*"
set typedir "RSCI"
#set ext ".IQ*"
#set typedir "IQ"

set shortFileLength 75

# Main program

while 1 {
  if ![catch {set filelist [lsort [glob [file join $sourcedir "*$ext"]]]}] {

    # remove the latest file
    set filelist [lrange $filelist 0 [expr [llength $filelist] - 2]]

    foreach file $filelist {
      # Parse file name into YMD
      regexp {.*_(....)-(..)-(..)_} $file match year month day

      puts "considering $file"
      # crop the first N frames    
      if {[file extension $file]==".gz"} {
        puts "file $file already zipped"
        # it's already zipped
        set gzFileName $file
      } else {
        if [regexp {.*_short.*} $file] {
          puts "file $file already headed"
          # It's already been headed
          set headFileName $file
        } else {
          set headFileName "[file root $file]_short[file extension $file]"
          puts "tclsh dcp_head.tcl $file $headFileName $shortFileLength"
          set errorFlag [catch {exec tclsh dcp_head.tcl $file $headFileName $shortFileLength} response]

          if {!$errorFlag && [file exists $headFileName]} {
            # move the original file to the "organised" directory 
            set subdirname [file join $destdir $typedir "${year}-${month}-${day}"]
           if ![file exists $subdirname] {
             file mkdir $subdirname
            }
            file rename $file $subdirname
          }
        }

        # puts "$file $year $month $day"
        # ZIP it
        set gzCommand "gzip --force $headFileName"
        puts "$gzCommand"
        set errorFlag [catch {exec $gzCommand} response ]
        puts "returned $errorFlag $response"
        set gzFileName "$headFileName.gz"
      }

      if [file exists $gzFileName] {
 
        # transfer it
        set remoteDir "/www/rsci/$RX_NAME/${year}-${month}-${day}"
        set remoteFileName "$remoteDir/[file tail $gzFileName]"
        MakeRemoteDir $remoteDir
	PutFile $gzFileName $remoteFileName
 
      }
    }
  }

  set waitVar 0
  puts "Waiting..."
  after 5000 {set waitVar 1}
  vwait waitVar
}
