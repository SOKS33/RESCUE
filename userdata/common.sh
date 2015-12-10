 #!/bin/bash

 default='\e[0m'
 bold='\e[1m'
 normal='\e[21m'
 red='\e[31m'
 green='\e[32m'
 yellow='\e[33m'
 blue='\e[94m'
 purple='\e[95m'
 cyan='\e[36m'

 bred=$bold$red
 bgreen=$bold$green
 byellow=$bold$yellow
 bblue=$bold$blue
 bpurple=$bold$purple
 bcyan=$bold$cyan

 scriptDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

 chunk=0
 version=0

 loss=1
 lossmodel=("Uniform" "Random" "Sinus" "Burst")


 umask 022

 function createButton {
      if [ $#  -ne 2 ] ; then
          echo "createButton string absolute_path_to_html_file"
          exit -23
      fi
      name=$1
      nameup=$(echo $name | awk '{print toupper($0)}')
      file=$2
      if [ ! -f $file ] ; then
          echo "$file does not exist"
          exit -43
      fi
      echo "<br>" >> $file
      echo "<a class=\"myButton\" id=\"${name}\" align=\"center\" onclick=\"toggle(this.id);\">Show ${name} stats</a>" >> $file
      echo "<div id=\"toggle${name}\" style=\"display: none\">" >> $file
      echo "<br><hr>" >> $file
      echo "<h2> $nameup STATS</h2><br>" >> $file
  }

 function getlastfield {
     if [ "$#" -ne 1 ] ;then
	     echo "Usage: ... $#"
	     exit 1
     else
	     grep -o "[^$1]*$"
     fi
 }

 # Try to find gnuplot and a user defined gnuplot if possible !
 if which gnuplot >/dev/null; then
     plot=$(which gnuplot)
     if [ -e /home/${USER}/usr/bin/gnuplot ] ; then
         plot="/home/${USER}/usr/bin/gnuplot"
         echo -e "${yellow}Will use user-provided ${bold}$($plot --version)${normal} from ${bold}$plot${default}"
     else
         echo -e "${yellow}Will use system-wide ${bold}$($plot --version)${normal} from ${bold}$plot${default}"
     fi
 else
     echo -e "${bred}gnuplot hasn't been found ! Either install it using a package manager, or install the latest version from CVS at ${bgreen}http://www.gnuplot.info/development/index.html ${default}"
 fi

