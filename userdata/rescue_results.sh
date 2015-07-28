 #!/bin/bash

 ####################
 ## INITIALIZATION ##
 ####################

 source common.sh

 res="$scriptDir/results/2015-07-10-14-29-05"
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
 rm -f $res/index.html
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

 eval explist=($(cat $res/explist.config))
 echo -e "${yellow}[INFO] Exponent list : ${bold}${explist[*]}${normal}"

 eval snrlist=($(cat $res/snrlist.config))
 echo -e "${yellow}[INFO] SNR list : ${bold}${snrlist[*]}${normal}"

 ################
 ##### HTML #####
 ################

 echo -e "${blue}Creating html files${default}"
 source createHTML.sh


 #DATARATE SIMULATIONS


 #HERE
 for prefix in "wifi" "rescue" ; do
     i=0
     for exp in ${explist[@]} ; do
         snr=${snrlist[$i]}
         for r in ${ratelist[@]} ; do
             if [[ -f $res/$prefix-$exp-$r-all ]] ; then
                 #Datarate stats
                 avg=$(cat $res/$prefix-$exp-$r-all | awk '{s1+=$1; s2+=$2; s3+=$3; s4+=$4; s5+=$5; s6+=$6; s7+=$7} END {print s1/NR " " s2/NR " " s3/NR " " s4/NR " " s5/NR " " s6/NR " " s7/NR}')
                 error=$(echo "$avg" | cut -d" " -f1)
                 delay=$(echo "$avg" | cut -d" " -f2)
                 jitter=$(echo "$avg" | cut -d" " -f3)
                 throughput=$(echo "$avg" | cut -d" " -f4)
                 sendrecv=$(echo "$avg" | cut -d" " -f5,6)
                 enqueueratio=$(echo "$avg" | cut -d" " -f7)
                 echo "$r $error" >> $res/$prefix-exp$exp-error.plot
                 echo "$r $delay" >> $res/$prefix-exp$exp-delay.plot
                 echo "$r $jitter" >> $res/$prefix-exp$exp-jitter.plot
                 echo "$r $throughput" >> $res/$prefix-exp$exp-throughput.plot
                 echo "$r $sendrecv" >> $res/$prefix-exp$exp-sendrecv.plot
                 echo "$r $enqueueratio" >> $res/$prefix-exp$exp-ratio.plot

                 #SNR stats
                 echo "$snr $error" >> $res/$prefix-r$r-error.plot
                 echo "$snr $delay" >> $res/$prefix-r$r-delay.plot
                 echo "$snr $jitter" >> $res/$prefix-r$r-jitter.plot
                 echo "$snr $throughput" >> $res/$prefix-r$r-throughput.plot
                 echo "$snr $sendrecv" >> $res/$prefix-r$r-sendrecv.plot
                 echo "$snr $enqueueratio" >> $res/$prefix-r$r-ratio.plot
             else
                 echo "${bred}$res/$prefix-$exp-$r-all does not exist ... INGORING ${default}"
             fi
         done
         ((i++))
     done
 done

 echo "<br>" >> $res/index.html
 echo "<a class=\"myButton\" id=\"Datarate\" align=\"center\" onclick=\"toggle(this.id);\">Show Datarate stats</a>" >> $res/index.html
 echo "<div id=\"toggleDatarate\" style=\"display: none\">" >> $res/index.html
 echo "<br><hr>" >> $res/index.html
 echo "<h2>DATARATE STATS</h2><br>" >> $res/index.html

 i=0
 for exp in ${explist[@]} ; do
     snr=${snrlist[$i]}
     $plot <<EOF
set terminal svg enhanced
set size 1,1
set style fill empty
set key inside left top vertical Right noreverse enhanced autotitles box linetype -1 linewidth 1.000 opaque
set title "FLR with various datarate for both wifi and rescue (SNR=$snr db)"
set ylabel "FLR (in %)"
set xtics nomirror
set xlabel 'Datarate (in Mbps)'
set style line 5  lc rgb "dark-green" lt 1 lw 2
set style line 15  lc rgb "red" lt 1 lw 2
set output '$res/index-exp$exp-error.svg'
plot '$res/wifi-exp$exp-error.plot' u 1:2:xtic(1) title 'wifi' w l ls 5 \
, '$res/rescue-exp$exp-error.plot' u 1:2:xtic(1) title 'rescue' w l ls 15
EOF
     echo "<img src='./index-exp$exp-error.svg'/>" >> $res/index.html
     $plot <<EOF
set terminal svg enhanced
set size 1,1
set style fill empty
set key inside left top vertical Right noreverse enhanced autotitles box linetype -1 linewidth 1.000 opaque
set title "Delay with various datarate for both wifi and rescue  (SNR=$snr db)"
set ylabel "Delay (in ms)"
set xtics nomirror
set xlabel 'Datarate (in Mbps)'
set style line 5  lc rgb "dark-green" lt 1 lw 2
set style line 15  lc rgb "red" lt 1 lw 2
set output '$res/index-exp$exp-delay.svg'
plot '$res/wifi-exp$exp-delay.plot' u 1:2:xtic(1) title 'wifi' w l ls 5 \
, '$res/rescue-exp$exp-delay.plot' u 1:2:xtic(1) title 'rescue' w l ls 15
EOF
     echo "<img src='./index-exp$exp-delay.svg'/>" >> $res/index.html

     $plot <<EOF
set terminal svg enhanced
set size 1,1
set style fill empty
set key inside left top vertical Right noreverse enhanced autotitles box linetype -1 linewidth 1.000 opaque
set title "Jitter with various datarate for both wifi and rescue (SNR=$snr db)"
set ylabel "Jitter (in ms)"
set xtics nomirror
set xlabel 'Datarate (in Mbps)'
set style line 5  lc rgb "dark-green" lt 1 lw 2
set style line 15  lc rgb "red" lt 1 lw 2
set output '$res/index-exp$exp-jitter.svg'
plot '$res/wifi-exp$exp-jitter.plot' u 1:2:xtic(1) title 'wifi' w l ls 5 \
, '$res/rescue-exp$exp-jitter.plot' u 1:2:xtic(1) title 'rescue' w l ls 15
EOF
     echo "<img src='./index-exp$exp-jitter.svg'/>" >> $res/index.html

     $plot <<EOF
set terminal svg enhanced
set size 1,1
set style fill empty
set key inside left top vertical Right noreverse enhanced autotitles box linetype -1 linewidth 1.000 opaque
set title "Throughput with various datarate for both wifi and rescue (SNR=$snr db)"
set ylabel "Throughput (in Mbps)"
set xtics nomirror
set xlabel 'Datarate (in Mbps)'
set style line 5  lc rgb "dark-green" lt 1 lw 2
set style line 15  lc rgb "red" lt 1 lw 2
set output '$res/index-exp$exp-throughput.svg'
plot '$res/wifi-exp$exp-throughput.plot' u 1:2:xtic(1) title 'wifi' w l ls 5 \
, '$res/rescue-exp$exp-throughput.plot' u 1:2:xtic(1) title 'rescue' w l ls 15
EOF
     echo "<img src='./index-exp$exp-throughput.svg'/>" >> $res/index.html

     $plot <<EOF
set terminal svg enhanced
set size 1,1
set style fill empty
set key inside left top vertical Right noreverse enhanced autotitles box linetype -1 linewidth 1.000 opaque
set title "Send & Recv with various datarate for both wifi and rescue (SNR=$snr db)"
set ylabel "Send & Recv (in pkts)"
set xtics nomirror
set xlabel 'Datarate (in Mbps)'
set style line 5  lc rgb "green" lt 1 lw 2
set style line 15  lc rgb "dark-green" lt 1 lw 2
set style line 10  lc rgb "red" lt 1 lw 2
set style line 20  lc rgb "dark-red" lt 1 lw 2
set output '$res/index-exp$exp-sendrecv.svg'
plot '$res/wifi-exp$exp-sendrecv.plot' u 1:2:xtic(1) title 'wifi/send' w l ls 10 \
,  '$res/rescue-exp$exp-sendrecv.plot' u 1:3:xtic(1) title 'wifi/recv' w l ls 20 \
,  '$res/rescue-exp$exp-sendrecv.plot' u 1:2:xtic(1) title 'rescue/send' w l ls 5 \
, '$res/rescue-exp$exp-sendrecv.plot' u 1:3:xtic(1) title 'rescue/recv' w l ls 15
EOF
     echo "<img src='./index-exp$exp-sendrecv.svg'/>" >> $res/index.html

      $plot <<EOF
set terminal svg enhanced
set size 1,1
set style fill empty
set key inside left top vertical Right noreverse enhanced autotitles box linetype -1 linewidth 1.000 opaque
set title "Efficiency ratio with various datarate for both wifi and rescue (SNR=$snr db)"
set ylabel "Efficiency ratio"
set xtics nomirror
set xlabel 'Datarate (in Mbps)'
set style line 5  lc rgb "dark-green" lt 1 lw 2
set style line 15  lc rgb "red" lt 1 lw 2
set output '$res/index-exp$exp-ratio.svg'
plot '$res/wifi-exp$exp-ratio.plot' u 1:2:xtic(1) title 'wifi' w l ls 5 \
, '$res/rescue-exp$exp-ratio.plot' u 1:2:xtic(1) title 'rescue' w l ls 15
EOF
     echo "<img src='./index-exp$exp-ratio.svg'/>" >> $res/index.html

     echo "<br>" >> $res/index.html
     ((i++))
 done

 echo "</div>" >> $res/index.html













 #SNR
 echo "<br>" >> $res/index.html
 echo "<a class=\"myButton\" id=\"SNR\" align=\"center\" onclick=\"toggle(this.id);\">Show SNR stats</a>" >> $res/index.html
 echo "<div id=\"toggleSNR\" style=\"display: none\">" >> $res/index.html
 echo "<br><hr>" >> $res/index.html
 echo "<h2>SNR STATS</h2><br>" >> $res/index.html
 for r in ${ratelist[@]} ; do
     $plot <<EOF
set terminal svg enhanced fsize 9
set size 1,1
set style fill empty
set key inside left top vertical Right noreverse enhanced autotitles box linetype -1 linewidth 1.000 opaque
set title "FLR with various SNR for both wifi and rescue (Datarate=$r Mbps)"
set ylabel "FLR (in %)"
set xtics nomirror rotate by -45
set logscale x
set xrange [*:*]
set xlabel 'SNR (in db)'
set style line 5  lc rgb "dark-green" lt 1 lw 2 pt 1 ps 1.3
set style line 15  lc rgb "red" lt 1 lw 2 pt 2 ps 1.3
set output '$res/index-r$r-error.svg'
plot '$res/wifi-r$r-error.plot' u (-\$1):2:xtic(1) title 'wifi' w lp  ls 5 \
, '$res/rescue-r$r-error.plot' u (-\$1):2:xtic(1) title 'rescue' w lp ls 15
EOF
     echo "<img src='./index-r$r-error.svg'/>" >> $res/index.html
     $plot <<EOF
set terminal svg enhanced fsize 9
set size 1,1
set style fill empty
set key inside left top vertical Right noreverse enhanced autotitles box linetype -1 linewidth 1.000 opaque
set title "Delay with various SNR for both wifi and rescue (Datarate=$r Mbps)"
set ylabel "Delay (in ms)"
set xtics nomirror rotate by -45
set xlabel 'SNR (in db)'
set logscale x
set xrange [*:*]
set style line 5  lc rgb "dark-green" lt 1 lw 2 pt 1 ps 1.3
set style line 15  lc rgb "red" lt 1 lw 2 pt 2 ps 1.3
set output '$res/index-r$r-delay.svg'
plot '$res/wifi-r$r-delay.plot' u (-\$1):2:xtic(1) title 'wifi' w lp ls 5 \
, '$res/rescue-r$r-delay.plot' u (-\$1):2:xtic(1) title 'rescue' w lp ls 15
EOF
     echo "<img src='./index-r$r-delay.svg'/>" >> $res/index.html

     $plot <<EOF
set terminal svg enhanced fsize 9
set size 1,1
set style fill empty
set key inside left top vertical Right noreverse enhanced autotitles box linetype -1 linewidth 1.000 opaque
set title "Jitter with various SNR for both wifi and rescue (Datarate=$r Mbps)"
set ylabel "Jitter (in ms)"
set xtics nomirror rotate by -45
set xlabel 'SNR (in db)'
set logscale x
set xrange [*:*]
set style line 5  lc rgb "dark-green" lt 1 lw 2  pt 1 ps 1.3
set style line 15  lc rgb "red" lt 1 lw 2  pt 2 ps 1.3
set output '$res/index-r$r-jitter.svg'
plot '$res/wifi-r$r-jitter.plot' u (-\$1):2:xtic(1) title 'wifi' w lp ls 5 \
, '$res/rescue-r$r-jitter.plot' u (-\$1):2:xtic(1) title 'rescue' w lp ls 15
EOF
     echo "<img src='./index-r$r-jitter.svg'/>" >> $res/index.html

     $plot <<EOF
set terminal svg enhanced fsize 9
set size 1,1
set style fill empty
set key inside left top vertical Right noreverse enhanced autotitles box linetype -1 linewidth 1.000 opaque
set title "Throughput with various SNR for both wifi and rescue (Datarate=$r Mbps)"
set ylabel "Throughput (in Mbps)"
set xtics nomirror rotate by -45
set xlabel 'SNR (in db)'
set logscale x
set xrange [*:*]
set style line 5  lc rgb "dark-green" lt 1 lw 2 pt 1 ps 1.3
set style line 15  lc rgb "red" lt 1 lw 2 pt 2 ps 1.3
set output '$res/index-r$r-throughput.svg'
plot '$res/wifi-r$r-throughput.plot' u (-\$1):2:xtic(1) title 'wifi' w lp ls 5 \
, '$res/rescue-r$r-throughput.plot' u (-\$1):2:xtic(1) title 'rescue' w lp ls 15
EOF
     echo "<img src='./index-r$r-throughput.svg'/>" >> $res/index.html

     $plot <<EOF
set terminal svg enhanced fsize 9
set size 1,1
set style fill empty
set key inside left top vertical Right noreverse enhanced autotitles box linetype -1 linewidth 1.000 opaque
set title "Send & Recv with various SNR for both wifi and rescue (Datarate=$r Mbps)"
set ylabel "Send & Recv (in pkts)"
set xtics nomirror rotate by -45
set xlabel 'SNR (in db)'
set logscale x
set xrange [*:*]
set style line 5  lc rgb "green" lt 1 lw 2 pt 1 ps 1.3
set style line 15  lc rgb "dark-green" lt 1 lw 2 pt 2 ps 1.3
set style line 10  lc rgb "red" lt 1 lw 2 pt 3 ps 1.3
set style line 20  lc rgb "dark-red" lt 1 lw 2 pt 4 ps 1.3
set output '$res/index-r$r-sendrecv.svg'
plot '$res/wifi-r$r-sendrecv.plot' u (-\$1):2:xtic(1) title 'wifi/send' w lp ls 10 \
,  '$res/wifi-r$r-sendrecv.plot' u (-\$1):3:xtic(1) title 'wifi/recv' w lp ls 20 \
,  '$res/rescue-r$r-sendrecv.plot' u (-\$1):2:xtic(1) title 'rescue/send' w lp ls 5 \
, '$res/rescue-r$r-sendrecv.plot' u (-\$1):3:xtic(1) title 'rescue/recv' w lp ls 15
EOF
     echo "<img src='./index-r$r-sendrecv.svg'/>" >> $res/index.html

          $plot <<EOF
set terminal svg enhanced fsize 9
set size 1,1
set style fill empty
set key inside left top vertical Right noreverse enhanced autotitles box linetype -1 linewidth 1.000 opaque
set title "Efficiency ratio with various SNR for both wifi and rescue (Datarate=$r Mbps)"
set ylabel "Efficiency ratio"
set xtics nomirror rotate by -45
set xlabel 'SNR (in db)'
set logscale x
set xrange [*:*]
set style line 5  lc rgb "dark-green" lt 1 lw 2 pt 1 ps 1.3
set style line 15  lc rgb "red" lt 1 lw 2 pt 2 ps 1.3
set output '$res/index-r$r-ratio.svg'
plot '$res/wifi-r$r-ratio.plot' u (-\$1):2:xtic(1) title 'wifi' w lp ls 5 \
, '$res/rescue-r$r-ratio.plot' u (-\$1):2:xtic(1) title 'rescue' w lp ls 15
EOF
          echo "<img src='./index-r$r-ratio.svg'/>" >> $res/index.html
     echo "<br>" >> $res/index.html
 done

 echo "</div>" >> $res/index.html


 # End of script
 echo "</body></html>" >> $res/index.html
 echo
 echo -e "${green}End of drawing script.${default}"
