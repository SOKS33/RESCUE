 #!/bin/bash

 ####################
 ## INITIALIZATION ##
 ####################

 source common.sh

 res="$scriptDir/results/2015-01-22-14-00-45"
 echo
 echo -e "${bgreen}*************************************"
 echo -e "*************************************"
 echo -e "********** Plotting script **********"
 echo -e "*************************************"
 echo -e "*************************************${default}"

 #################
 # SCRIPT PARAMS #
 #################
 echo
 if [ $# == 2 ] && [ -d "$1" ] ; then # called by rescue.sh
     res=$1
     dummy=$2
     echo -e "${yellow}[INFO] Running $0 script on directory $res. ${default}"

 else # standalone
     if [ $# == 1 ] ; then
         res=$scriptDir/results/$1
     fi
     if [[ -d $res ]] ; then
         echo -e "${yellow}[INFO] Drawing simulation results at $res${default}"
         echo
     else
         echo -e "${red}Please provide an existing directory.${default}"
         exit -1
     fi
 fi

 ##################
 #### CLEANING ####
 ##################

 echo -e "${yellow}[INFO] Removing old files (${bold}*.svg${normal}, ${bold}*.png${normal}, ${bold}*.plot${normal} and ${bold}index*${normal})${default}"
 rm -f $res/*.svg
 rm -f $res/*.png
 rm -f $res/index*
 rm -f $res/*.plot

 ####################
 #### PARAMETERS ####
 ####################

 nbruns=$(grep 'nbruns' $res/info | getlastfield ":" | awk '{ printf("%d ",$0) }')
 echo -e "${yellow}[INFO] Parameters taken out from simulation script : ${default}"
 echo -e "${yellow}[INFO] Runs per simulation : ${bold}$nbruns${normal} ${default}"

 eval ratelist=($(cat $res/ratelist.config))
 echo -e "${yellow}[INFO] Rate list : ${bold}${ratelist[*]}${normal}"


 ################
 ##### HTML #####
 ################

 echo -e "${blue}Creating html files${default}"
 source createHTML.sh


 #DATARATE SIMULATIONS
 echo "<br>" >> $res/index.html
 echo "<a class=\"myButton\" id=\"Datarate\" align=\"center\" onclick=\"toggle(this.id);\">Show Datarate stats</a>" >> $res/index.html
 echo "<div id=\"toggleDatarate\" style=\"display: none\">" >> $res/index.html
 echo "<br><hr>" >> $res/index.html
 echo "<h2>DATARATE STATS</h2><br>" >> $res/index.html

 #HERE
 for prefix in "wifi" "rescue" ; do
     for r in ${ratelist[@]} ; do
         if [[ -f $res/$prefix-$r-all ]] ; then
             avg=$(cat $res/$prefix-$r-all | awk '{s1+=$1; s2+=$2; s3+=$3; s4+=$4; s5+=$5; s6+=$6} END {print s1/NR " " s2/NR " " s3/NR " " s4/NR " " s5/NR " " s6/NR}')
             error=$(echo "$avg" | cut -d" " -f1)
             delay=$(echo "$avg" | cut -d" " -f2)
             jitter=$(echo "$avg" | cut -d" " -f3)
             throughput=$(echo "$avg" | cut -d" " -f4)
             sendrecv=$(echo "$avg" | cut -d" " -f5,6)
             echo "$r $error" >> $res/$prefix-error.plot
             echo "$r $delay" >> $res/$prefix-delay.plot
             echo "$r $jitter" >> $res/$prefix-jitter.plot
             echo "$r $throughput" >> $res/$prefix-throughput.plot
             echo "$r $sendrecv" >> $res/$prefix-sendrecv.plot
         else
             echo "${bred}$res/$prefix-$r-all does not exist ... INGORING ${default}"
         fi
     done
 done

 $plot <<EOF
set terminal svg enhanced
set size 1,1
set style fill empty
set key inside left top vertical Right noreverse enhanced autotitles box linetype -1 linewidth 1.000 opaque
set title "Error rate with various datarate for both wifi and rescue"
set ylabel "Error rate (in %)"
set xtics nomirror
set xlabel 'Datarate (in Mbps)'
set style line 5  lc rgb "dark-green" lt 1 lw 2
set style line 15  lc rgb "red" lt 1 lw 2
set output '$res/index-error.svg'
plot '$res/wifi-error.plot' u 1:2:xtic(1) title 'wifi' w l ls 5 \
, '$res/rescue-error.plot' u 1:2:xtic(1) title 'rescue' w l ls 15
EOF
 echo "<img src='./index-error.svg'/> <br><br>" >> $res/index.html
  $plot <<EOF
set terminal svg enhanced
set size 1,1
set style fill empty
set key inside left top vertical Right noreverse enhanced autotitles box linetype -1 linewidth 1.000 opaque
set title "Delay with various datarate for both wifi and rescue"
set ylabel "Delay (in ms)"
set xtics nomirror
set xlabel 'Datarate (in Mbps)'
set style line 5  lc rgb "dark-green" lt 1 lw 2
set style line 15  lc rgb "red" lt 1 lw 2
set output '$res/index-delay.svg'
plot '$res/wifi-delay.plot' u 1:2:xtic(1) title 'wifi' w l ls 5 \
, '$res/rescue-delay.plot' u 1:2:xtic(1) title 'rescue' w l ls 15
EOF
echo "<img src='./index-delay.svg'/> <br><br>" >> $res/index.html

 $plot <<EOF
set terminal svg enhanced
set size 1,1
set style fill empty
set key inside left top vertical Right noreverse enhanced autotitles box linetype -1 linewidth 1.000 opaque
set title "Jitter with various datarate for both wifi and rescue"
set ylabel "Jitter (in ms)"
set xtics nomirror
set xlabel 'Datarate (in Mbps)'
set style line 5  lc rgb "dark-green" lt 1 lw 2
set style line 15  lc rgb "red" lt 1 lw 2
set output '$res/index-jitter.svg'
plot '$res/wifi-jitter.plot' u 1:2:xtic(1) title 'wifi' w l ls 5 \
, '$res/rescue-jitter.plot' u 1:2:xtic(1) title 'rescue' w l ls 15
EOF
echo "<img src='./index-jitter.svg'/> <br><br>" >> $res/index.html

 $plot <<EOF
set terminal svg enhanced
set size 1,1
set style fill empty
set key inside left top vertical Right noreverse enhanced autotitles box linetype -1 linewidth 1.000 opaque
set title "Throughput with various datarate for both wifi and rescue"
set ylabel "Throughput (in Mbps)"
set xtics nomirror
set xlabel 'Datarate (in Mbps)'
set style line 5  lc rgb "dark-green" lt 1 lw 2
set style line 15  lc rgb "red" lt 1 lw 2
set output '$res/index-throughput.svg'
plot '$res/wifi-throughput.plot' u 1:2:xtic(1) title 'wifi' w l ls 5 \
, '$res/rescue-throughput.plot' u 1:2:xtic(1) title 'rescue' w l ls 15
EOF
 echo "<img src='./index-throughput.svg'/> <br><br>" >> $res/index.html

  $plot <<EOF
set terminal svg enhanced
set size 1,1
set style fill empty
set key inside left top vertical Right noreverse enhanced autotitles box linetype -1 linewidth 1.000 opaque
set title "Send & Recv with various datarate for both wifi and rescue"
set ylabel "Send & Recv (in pkts)"
set xtics nomirror
set xlabel 'Datarate (in Mbps)'
set style line 5  lc rgb "dark-green" lt 1 lw 2
set style line 15  lc rgb "red" lt 1 lw 2
set output '$res/index-sendrecv.svg'
plot '$res/wifi-sendrecv.plot' u 1:2:xtic(1) title 'wifi/send' w l ls 5 \
,  '$res/rescue-sendrecv.plot' u 1:3:xtic(1) title 'wifi/recv' w l ls 15 \
,  '$res/rescue-sendrecv.plot' u 1:2:xtic(1) title 'rescue/send' w l ls 5 \
, '$res/rescue-sendrecv.plot' u 1:3:xtic(1) title 'rescue/recv' w l ls 15
EOF
 echo "<img src='./index-sendrecv.svg'/> <br><br>" >> $res/index.html

 echo "</div>" >> $res/index.html

 # End of script
 echo "</body></html>" >> $res/index.html
 echo
 echo -e "${green}End of drawing script.${default}"
