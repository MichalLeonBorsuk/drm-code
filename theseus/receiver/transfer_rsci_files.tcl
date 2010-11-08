source settings.tcl

set ext ".rs*"
set typedir "RSCI"
#set ext ".IQ*"
#set typedir "IQ"

set sourcedir "F:/datafiles/$RX_NAME"
set destdir "F:/datafiles/$RX_NAME/organised"
set gzCmdName [file join $ROOT_DIR gzip]  
set pscpCmdName "C:/Program Files/putty/pscp.exe"
set puttyCmdName "C:/Program Files/putty/putty.exe"
set plinkCmdName "C:/Program Files/putty/plink.exe"
set serverUser drm
set serverPassword 5au5age5
set shortFileLength 75

proc MakeRemoteDir {dirName} {
  global SERVER_ADDRESS
  global serverUser
  global serverPassword
  global plinkCmdName

  set tty [open "putty_commands.scr" w]
  puts $tty "mkdir $dirName"
  close $tty

  puts "$plinkCmdName -ssh ${serverUser}@$SERVER_ADDRESS -pw $serverPassword mkdir $dirName"
  set errorFlag [catch {exec $plinkCmdName -ssh ${serverUser}@$SERVER_ADDRESS -pw $serverPassword mkdir $dirName} response]
  if {$errorFlag} {
    puts "Making directory failed with $response"
  }
}


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
        set gzCommand "$gzCmdName --force $headFileName"
        puts "$gzCommand"
        set errorFlag [catch {exec $gzCmdName --force $headFileName} response ]
        puts "returned $errorFlag $response"
        set gzFileName "$headFileName.gz"
      }

      if [file exists $gzFileName] {
 
        # transfer it
        set remoteFileName "mayflower/www/rsci/$RX_NAME/${year}-${month}-${day}/[file tail $gzFileName]"
        MakeRemoteDir mayflower/www/rsci/$RX_NAME
        MakeRemoteDir mayflower/www/rsci/$RX_NAME/${year}-${month}-${day}
       
        set pscpCommand "$pscpCmdName -pw 5au5age5 $gzFileName drm@132.185.142.42:$remoteFileName"
        puts $pscpCommand
        set errorFlag [catch {exec $pscpCmdName -pw 5au5age5 $gzFileName drm@132.185.142.42:$remoteFileName} response]
        puts "returned $errorFlag $response"
        if {!$errorFlag} {
          catch {file delete $gzFileName}
        }
 
      }
    }
  }

  set waitVar 0
  puts "Waiting..."
  after 5000 {set waitVar 1}
  vwait waitVar
}
