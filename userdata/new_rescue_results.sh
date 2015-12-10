#!/bin/bash

####################
## INITIALIZATION ##
####################

source common.sh

#toy5
#res="$scriptDir/results/2015-12-09-14-22-40"
#toy6-rescue
res="$scriptDir/results/2015-12-09-16-15-14"
echo
echo -e "${bgreen}*************************************"
echo -e "*************************************"
echo -e "********** Plotting script **********"
echo -e "*************************************"
echo -e "*************************************${default}"
echo
#################
# SCRIPT PARAMS #
#################

standalone=0
if [ $# == 2 ] && [ -d "$1" ] ; then # called by rescue.sh
    res=$1
    dummy=$2
    echo -e "${yellow}[INFO] Running $0 script on directory $res. ${default}"
else # standalone
    standalone=1
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

if [ $standalone == 1 ] ; then
    echo -e "${yellow}[INFO] Removing old files (${bold}*.svg${normal}, ${bold}*.png${normal}, ${bold}*.plot${normal} and ${bold}index*${normal})${default}"
    rm -f $res/*.svg
    rm -f $res/*.png
    rm -f $res/index.html
    rm -f $res/index*
    rm -f $res/*.plot
fi

####################
#### PARAMETERS ####
####################

nbruns=$(grep 'nbruns' $res/info | getlastfield ":" | awk '{ printf("%d ",$0) }')
echo -e "${yellow}[INFO] Parameters taken out from simulation script : ${default}"
echo -e "${yellow}[INFO] \t Runs per simulation : ${bold}$nbruns${normal} ${default}"

eval prefixlist=($(cat $res/prefixlist.config))
echo -e "${yellow}[INFO] \t MAC version list : ${bold}${prefixlist[*]}${normal}"

eval txlist=($(cat $res/txlist.config))
echo -e "${yellow}[INFO] \t Tx list : ${bold}${txlist[*]}${normal}"

eval snrlist=($(cat $res/snrlist.config))
echo -e "${yellow}[INFO] \t SNR list : ${bold}${snrlist[*]}${normal}"

eval ratelist=($(cat $res/ratelist.config))
echo -e "${yellow}[INFO] \t Datarate list : ${bold}${ratelist[*]}${normal}"

simfile=$(cat $res/simfile.config)
echo -e "${yellow}[INFO] \t Simulation file : ${bold}${simfile}${normal}"

minmax=$(echo "${txlist[*]}" | awk 'BEGIN {RS=" ";min=50;max=0;} {if(min>$1) min=$1; if(max<$1) max=$1;next;} END {print min " " max}')
mintx=$(echo "$minmax" | cut -d" " -f1)
mintx=$(echo "$mintx - 0.1" | bc )
maxtx=$(echo "$minmax" | cut -d" " -f2)
maxtx=$(echo "$maxtx + 0.1" | bc )
################
##### HTML #####
################

echo -e "${blue}Creating html files${default}"
source new_createHTML.sh

html=$res/index.html

echo "<font size=\"5\"><b>Simulation parameters :</b></font><br><br>" >> $html
echo "&emsp;Simulation file : ${simfile} <br>" >> $html
echo "&emsp;MAC used : ${prefixlist[*]} <br>" >> $html
echo "&emsp;Tx power : ${txlist[*]} <br>" >> $html
echo "&emsp;Datarate power : ${ratelist[*]} <br>" >> $html

for prefix in ${prefixlist[@]} ; do
    for dr in ${ratelist[@]} ; do
        snrindex=0
        for tx in ${txlist[@]} ; do
            if [[ -f $res/$prefix-$tx-$dr-all ]] ; then
                #Tx Power stats
                avg=$(cat $res/$prefix-$tx-$dr-all | awk '{s1+=$1; s2+=$2; s3+=$3; s4+=$4; s5+=$5; s6+=$6; s7+=$7} END {print s1/NR " " s2/NR " " s3/NR " " s4/NR " " s5/NR " " s6/NR " " s7/NR}')
                error=$(echo "$avg" | cut -d" " -f1)
                delay=$(echo "$avg" | cut -d" " -f2)
                jitter=$(echo "$avg" | cut -d" " -f3)
                throughput=$(echo "$avg" | cut -d" " -f4)
                sendrecv=$(echo "$avg" | cut -d" " -f5,6)
                enqueueratio=$(echo "$avg" | cut -d" " -f7)
                #echo "DR=$dr && TX=$tx avg $avg"
                #echo "DR=$dr && TX=$tx delay $delay"

                snr=$(echo "${snrlist[$snrindex]}" | LC_ALL=C xargs printf "%.*f" 1)
                #echo $snr

                #Old : s/$snr/$tx
                echo "$snr $error" >> $res/$prefix-$dr-tx-error.plot
                echo "$snr $delay" >> $res/$prefix-$dr-tx-delay.plot
                echo "$snr $jitter" >> $res/$prefix-$dr-tx-jitter.plot
                echo "$snr $throughput" >> $res/$prefix-$dr-tx-throughput.plot
                echo "$snr $sendrecv" >> $res/$prefix-$dr-tx-sendrecv.plot
                echo "$snr $enqueueratio" >> $res/$prefix-$dr-tx-ratio.plot

            else
                echo -e "${bred}$res/$prefix-$tx-all does not exist ... IGNORING ${default}"
            fi
            ((snrindex++))
        done
    done
done

minsnr=$(echo "${snrlist[-1]} - 0.1" | bc -l | LC_ALL=C xargs printf "%.*f" 1)
#echo $minsnr

createButton "TxPower" $html
#########
# ERROR #
#########
if [[ -f $res/Simple-1-tx-error.plot
            && -f $res/Simple-3-tx-error.plot
            && -f $res/Distributed-1-tx-error.plot
            && -f $res/Distributed-3-tx-error.plot ]] ; then
     $plot <<EOF
set terminal svg enhanced
set tics font ", 10"
set size 1,1
set grid y
set style fill empty
set key inside left top vertical Right noreverse enhanced autotitles box linetype -1 linewidth 1.000 opaque
set title "FLR with various SNR, 1Mbps and 3Mbps \n for both SimpleMAC and DistributedMAC"
set ylabel "FLR [%]"
set xtics nomirror rotate by -45 scale 1
set xrange [:] reverse
set xlabel 'SNR [dB]'
set style line 5 lc rgb "green" lt 1 lw 2
set style line 6 lc rgb "dark-green" lt 1 lw 2
set style line 15 lc rgb "red" lt 1 lw 2
set style line 16 lc rgb "dark-red" lt 1 lw 2
set output '$res/index-tx-error-final.svg'
plot '$res/Simple-1-tx-error.plot' u 1:2:xtic(1) title 'Simple 1Mbps' w l ls 5 \
, '$res/Simple-3-tx-error.plot' u 1:2:xtic(1) title 'Simple 3Mbps' w l ls 6 \
, '$res/Distributed-1-tx-error.plot' u 1:2:xtic(1) title 'Distributed 1Mbps' w l ls 15 \
, '$res/Distributed-3-tx-error.plot' u 1:2:xtic(1) title 'Distributed 3Mbps' w l ls 16
EOF
    echo "<img src='./index-tx-error-final.svg'/>" >> $html
fi
#All error
$plot <<EOF
set terminal svg enhanced
set tics font ", 10"
set size 1,1
set grid y
set style fill empty
set key outside center top horizontal box linetype -1 linewidth 1.000 spacing 0.6 font ",8"
set title "FLR with various SNR and Datarates \n for both SimpleMAC and DistributedMAC"
set ylabel "FLR [%]"
set xtics nomirror rotate by -45 scale 0
set xrange [:] reverse
set xlabel 'SNR [dB]'
set style line 5 lc rgb "green" lt 1 lw 2
set style line 6 lc rgb "dark-green" lt 1 lw 2
set style line 15 lc rgb "red" lt 1 lw 2
set style line 16 lc rgb "dark-red" lt 1 lw 2
set output '$res/index-tx-error.svg'

list="${ratelist[*]}
item(n) = word(list,n)

plot \
for [w = 0 : ${#ratelist[@]} - 1] \
'$res/Simple-'.item(w+1).'-tx-error.plot' using 1:2:xtic(1) title 'Simple '.item(w+1).'Mbps' w l ls (w+5) , \
for [w = 0 : ${#ratelist[@]} - 1] \
'$res/Distributed-'.item(w+1).'-tx-error.plot' u 1:2:xtic(1) title 'Distributed '.item(w+1).'Mbps' w l ls (w+15)
EOF
echo "<img src='./index-tx-error.svg'/>" >> $html

#########
# DELAY #
#########
if [[ -f $res/Simple-1-tx-delay.plot
            && -f $res/Simple-3-tx-delay.plot
            && -f $res/Distributed-1-tx-delay.plot
            && -f $res/Distributed-3-tx-delay.plot ]] ; then
     $plot <<EOF
set terminal svg enhanced
set tics font ", 10"
set size 1,1
set grid y
set style fill empty
set key at graph 0.42,0.8 vertical Right noreverse enhanced autotitles box linetype -1 linewidth 1.000 opaque
set title "Delay with various SNR, 1Mbps and 3Mbps \n for both SimpleMAC and DistributedMAC"
set ylabel "Delay [ms]"
set xtics nomirror rotate by -45
set xrange [:] reverse
set xlabel 'SNR [dB]'
set logscale y
set style line 5 lc rgb "green" lt 1 lw 2
set style line 6 lc rgb "dark-green" lt 1 lw 2
set style line 15 lc rgb "red" lt 1 lw 2
set style line 16 lc rgb "dark-red" lt 1 lw 2
set output '$res/index-tx-delay-final.svg'
plot '$res/Simple-1-tx-delay.plot' u 1:2:xtic(1) title 'Simple 1Mbps' w l ls 5 \
, '$res/Simple-3-tx-delay.plot' u 1:2:xtic(1) title 'Simple 3Mbps' w l ls 6 \
, '$res/Distributed-1-tx-delay.plot' u 1:2:xtic(1) title 'Distributed 1Mbps' w l ls 15 \
, '$res/Distributed-3-tx-delay.plot' u 1:2:xtic(1) title 'Distributed 3Mbps' w l ls 16
EOF
    echo "<img src='./index-tx-delay-final.svg'/>" >> $html
fi
#All delay
$plot <<EOF
set terminal svg enhanced
set tics font ", 10"
set size 1,1
set grid y
set style fill empty
set key outside center top horizontal box linetype -1 linewidth 1.000 spacing 0.6 font ",8"
set title "Delay with various SNR and Datarates \n for both SimpleMAC and DistributedMAC"
set ylabel "Delay [ms]"
set xtics nomirror rotate by -45 scale 0
set xrange [:] reverse
set xlabel 'SNR [dB]'
set style line 5 lc rgb "green" lt 1 lw 2
set style line 6 lc rgb "dark-green" lt 1 lw 2
set style line 15 lc rgb "red" lt 1 lw 2
set style line 16 lc rgb "dark-red" lt 1 lw 2
set output '$res/index-tx-delay.svg'

list="${ratelist[*]}
item(n) = word(list,n)

plot \
for [w = 0 : ${#ratelist[@]} - 1] \
'$res/Simple-'.item(w+1).'-tx-delay.plot' using 1:2:xtic(1) title 'Simple '.item(w+1).'Mbps' w l ls (w+5) , \
for [w = 0 : ${#ratelist[@]} - 1] \
'$res/Distributed-'.item(w+1).'-tx-delay.plot' u 1:2:xtic(1) title 'Distributed '.item(w+1).'Mbps' w l ls (w+15)
EOF
echo "<img src='./index-tx-delay.svg'/>" >> $html

##############
# THROUGHPUT #
##############
if [[ -f $res/Simple-1-tx-throughput.plot
            && -f $res/Simple-3-tx-throughput.plot
            && -f $res/Distributed-1-tx-throughput.plot
            && -f $res/Distributed-3-tx-throughput.plot ]] ; then
     $plot <<EOF
set terminal svg enhanced
set tics font ", 10"
set size 1,1
set grid y
set style fill empty
set key inside left bottom vertical Right noreverse enhanced autotitles box linetype -1 linewidth 1.000 opaque
set title "Throughput with various SNR, 1Mbps and 3Mbps \n for both SimpleMAC and DistributedMAC"
set ylabel "Throughput [Mbps]"
set xtics nomirror rotate by -45
set xrange [:] reverse
set xlabel 'SNR [dB]'
set style line 5 lc rgb "green" lt 1 lw 2
set style line 6 lc rgb "dark-green" lt 1 lw 2
set style line 15 lc rgb "red" lt 1 lw 2
set style line 16 lc rgb "dark-red" lt 1 lw 2
set output '$res/index-tx-throughput-final.svg'
plot '$res/Simple-1-tx-throughput.plot' u 1:2:xtic(1) title 'Simple 1Mbps' w l ls 5 \
, '$res/Simple-3-tx-throughput.plot' u 1:2:xtic(1) title 'Simple 3Mbps' w l ls 6 \
, '$res/Distributed-1-tx-throughput.plot' u 1:2:xtic(1) title 'Distributed 1Mbps' w l ls 15 \
, '$res/Distributed-3-tx-throughput.plot' u 1:2:xtic(1) title 'Distributed 3Mbps' w l ls 16
EOF
    echo "<img src='./index-tx-throughput-final.svg'/>" >> $html
fi
#All throughput
$plot <<EOF
set terminal svg enhanced
set tics font ", 10"
set size 1,1
set grid y
set style fill empty
set key outside center top horizontal box linetype -1 linewidth 1.000 spacing 0.6 font ",8"
set title "Throughput with various SNR and Datarates \n for both SimpleMAC and DistributedMAC"
set ylabel "Throughput [Mbps]"
set xtics nomirror rotate by -45 scale 0
set xrange [:] reverse
set xlabel 'SNR [dB]'
set style line 5 lc rgb "green" lt 1 lw 2
set style line 6 lc rgb "dark-green" lt 1 lw 2
set style line 15 lc rgb "red" lt 1 lw 2
set style line 16 lc rgb "dark-red" lt 1 lw 2
set output '$res/index-tx-throughput.svg'

list="${ratelist[*]}
item(n) = word(list,n)

plot \
for [w = 0 : ${#ratelist[@]} - 1] \
'$res/Simple-'.item(w+1).'-tx-throughput.plot' using 1:2:xtic(1) title 'Simple '.item(w+1).'Mbps' w l ls (w+5) , \
for [w = 0 : ${#ratelist[@]} - 1] \
'$res/Distributed-'.item(w+1).'-tx-throughput.plot' u 1:2:xtic(1) title 'Distributed '.item(w+1).'Mbps' w l ls (w+15)
EOF
echo "<img src='./index-tx-throughput.svg'/>" >> $html

#########
# RATIO #
#########
if [[ -f $res/Simple-1-tx-ratio.plot
            && -f $res/Simple-3-tx-ratio.plot
            && -f $res/Distributed-1-tx-ratio.plot
            && -f $res/Distributed-3-tx-ratio.plot ]] ; then
     $plot <<EOF
set terminal svg enhanced
set tics font ", 10"
set size 1,1
set grid y
set style fill empty
set key inside left bottom vertical Right noreverse enhanced autotitles box linetype -1 linewidth 1.000 opaque
set title "Efficiency ratio with various SNR, 1Mbps and 3Mbps \n for both SimpleMAC and DistributedMAC"
set ylabel "Efficiency ratio"
set xtics nomirror rotate by -45
set xrange [:] reverse
set xlabel 'SNR [dB]'
set style line 5 lc rgb "green" lt 1 lw 2
set style line 6 lc rgb "dark-green" lt 1 lw 2
set style line 15 lc rgb "red" lt 1 lw 2
set style line 16 lc rgb "dark-red" lt 1 lw 2
set output '$res/index-tx-ratio-final.svg'
plot '$res/Simple-1-tx-ratio.plot' u 1:2:xtic(1) title 'Simple 1Mbps' w l ls 5 \
, '$res/Simple-3-tx-ratio.plot' u 1:2:xtic(1) title 'Simple 3Mbps' w l ls 6 \
, '$res/Distributed-1-tx-ratio.plot' u 1:2:xtic(1) title 'Distributed 1Mbps' w l ls 15 \
, '$res/Distributed-3-tx-ratio.plot' u 1:2:xtic(1) title 'Distributed 3Mbps' w l ls 16
EOF
    echo "<img src='./index-tx-ratio-final.svg'/>" >> $html
fi
#All ratio
$plot <<EOF
set terminal svg enhanced
set tics font ", 10"
set size 1,1
set grid y
set style fill empty
set key outside center top horizontal box linetype -1 linewidth 1.000 spacing 0.6 font ",8"
set title "Efficiency ratio with various SNR and Datarates \n for both SimpleMAC and DistributedMAC"
set ylabel "Efficiency ratio"
set xtics nomirror rotate by -45 scale 0
set xrange [:] reverse
set xlabel 'SNR [dB]'
set style line 5 lc rgb "green" lt 1 lw 2
set style line 6 lc rgb "dark-green" lt 1 lw 2
set style line 15 lc rgb "red" lt 1 lw 2
set style line 16 lc rgb "dark-red" lt 1 lw 2
set output '$res/index-tx-ratio.svg'

list="${ratelist[*]}
item(n) = word(list,n)

plot \
for [w = 0 : ${#ratelist[@]} - 1] \
'$res/Simple-'.item(w+1).'-tx-ratio.plot' using 1:2:xtic(1) title 'Simple '.item(w+1).'Mbps' w l ls (w+5) , \
for [w = 0 : ${#ratelist[@]} - 1] \
'$res/Distributed-'.item(w+1).'-tx-ratio.plot' u 1:2:xtic(1) title 'Distributed '.item(w+1).'Mbps' w l ls (w+15)
EOF
echo "<img src='./index-tx-ratio.svg'/>" >> $html

echo "<br>" >> $html
echo "<hr>" >> $html



#Datarate

for dr in ${ratelist[@]} ; do
    $plot <<EOF
set terminal svg enhanced
set size 1,1
set style fill empty
set key inside left top vertical Right noreverse enhanced autotitles box linetype -1 linewidth 1.000 opaque
set title "FLR with various SNR for both \n SimpleMAC and DistributedMAC (Datarate = ${dr}Mbps)"
set ylabel "FLR [%]"
set xtics nomirror rotate by -45 scale 0
set xrange [:] reverse
set xlabel 'SNR [dB]'
set style line 5  lc rgb "dark-green" lt 1 lw 2
set style line 15  lc rgb "red" lt 1 lw 2
set output '$res/index-$dr-tx-error.svg'
plot '$res/Simple-$dr-tx-error.plot' u 1:2:xtic(1) title 'Simple' w l ls 5 \
, '$res/Distributed-$dr-tx-error.plot' u 1:2:xtic(1) title 'Distributed' w l ls 15
EOF
    echo "<img src='./index-$dr-tx-error.svg'/>" >> $html

    $plot <<EOF
set terminal svg enhanced
set size 1,1
set style fill empty
set key inside left top vertical Right noreverse enhanced autotitles box linetype -1 linewidth 1.000 opaque
set title "Delay with various SNR for both \n SimpleMAC and DistributedMAC (Datarate = ${dr}Mbps)"
set ylabel "Delay [ms]"
set xtics nomirror rotate by -45 scale 0
set xrange [:] reverse
set xlabel 'SNR [dB]'
set style line 5  lc rgb "dark-green" lt 1 lw 2
set style line 15  lc rgb "red" lt 1 lw 2
set output '$res/index-$dr-tx-delay.svg'
plot '$res/Simple-$dr-tx-delay.plot' u 1:2:xtic(1) title 'Simple' w l ls 5 \
, '$res/Distributed-$dr-tx-delay.plot' u 1:2:xtic(1) title 'Distributed' w l ls 15
EOF
    echo "<img src='./index-$dr-tx-delay.svg'/>" >> $html

    $plot <<EOF
set terminal svg enhanced
set size 1,1
set style fill empty
set key inside left top vertical Right noreverse enhanced autotitles box linetype -1 linewidth 1.000 opaque
set title "Jitter with various SNR for both \n SimpleMAC and DistributedMAC (Datarate = ${dr}Mbps)"
set ylabel "Jitter [ms]"
set xtics nomirror rotate by -45 scale 0
set xrange [:] reverse
set xlabel 'SNR [dB]'
set style line 5  lc rgb "dark-green" lt 1 lw 2
set style line 15  lc rgb "red" lt 1 lw 2
set output '$res/index-$dr-tx-jitter.svg'
plot '$res/Simple-$dr-tx-jitter.plot' u 1:2:xtic(1) title 'Simple' w l ls 5 \
, '$res/Distributed-$dr-tx-jitter.plot' u 1:2:xtic(1) title 'Distributed' w l ls 15
EOF
    echo "<img src='./index-$dr-tx-jitter.svg'/>" >> $html

    $plot <<EOF
set terminal svg enhanced
set size 1,1
set style fill empty
set key inside left top vertical Right noreverse enhanced autotitles box linetype -1 linewidth 1.000 opaque
set title "Throughput with various SNR for both \n SimpleMAC and DistributedMAC (Datarate = ${dr}Mbps)"
set ylabel "Throughput [Mbps]"
set xtics nomirror rotate by -45 scale 0
set xrange [:] reverse
set xlabel 'SNR [dB]'
set style line 5  lc rgb "dark-green" lt 1 lw 2
set style line 15  lc rgb "red" lt 1 lw 2
set output '$res/index-$dr-tx-throughput.svg'
plot '$res/Simple-$dr-tx-throughput.plot' u 1:2:xtic(1) title 'Simple' w l ls 5 \
, '$res/Distributed-$dr-tx-throughput.plot' u 1:2:xtic(1) title 'Distributed' w l ls 15
EOF
    echo "<img src='./index-$dr-tx-throughput.svg'/>" >> $html

    $plot <<EOF
set terminal svg enhanced
set size 1,1
set style fill empty
set key inside left top vertical Right noreverse enhanced autotitles box linetype -1 linewidth 1.000 opaque
set title "Send & Recv with various SNR for both \n SimpleMAC and DistributedMAC (Datarate = ${dr}Mbps)"
set ylabel "Send & Recv (in pkts)"
set xtics nomirror rotate by -45 scale 0
set xrange [:] reverse
set xlabel 'SNR [dB]'
set style line 5  lc rgb "green" lt 1 lw 2
set style line 15  lc rgb "dark-green" lt 1 lw 2
set style line 10  lc rgb "red" lt 1 lw 2
set style line 20  lc rgb "dark-red" lt 1 lw 2
set output '$res/index-$dr-tx-sendrecv.svg'
plot '$res/Simple-$dr-tx-sendrecv.plot' u 1:2:xtic(1) title 'Simple/send' w l ls 10 \
,  '$res/Distributed-$dr-tx-sendrecv.plot' u 1:3:xtic(1) title 'Simple/recv' w l ls 20 \
,  '$res/Distributed-$dr-tx-sendrecv.plot' u 1:2:xtic(1) title 'Distributed/send' w l ls 5 \
, '$res/Distributed-$dr-tx-sendrecv.plot' u 1:3:xtic(1) title 'Distributed/recv' w l ls 15
EOF
    echo "<img src='./index-$dr-tx-sendrecv.svg'/>" >> $html

    $plot <<EOF
set terminal svg enhanced
set size 1,1
set style fill empty
set key inside left top vertical Right noreverse enhanced autotitles box linetype -1 linewidth 1.000 opaque
set title "Efficiency ratio with various SNR for both \n SimpleMAC and DistributedMAC (Datarate = ${dr}Mbps)"
set ylabel "Efficiency ratio"
set xtics nomirror rotate by -45 scale 0
set xrange [:] reverse
set xlabel 'SNR [dB]'
set style line 5  lc rgb "dark-green" lt 1 lw 2
set style line 15  lc rgb "red" lt 1 lw 2
set output '$res/index-$dr-tx-ratio.svg'
plot '$res/Simple-$dr-tx-ratio.plot' u 1:2:xtic(1) title 'Simple' w l ls 5 \
, '$res/Distributed-$dr-tx-ratio.plot' u 1:2:xtic(1) title 'Distributed' w l ls 15
EOF
    echo "<img src='./index-$dr-tx-ratio.svg'/>" >> $html

    echo "<br>" >> $html
done

echo "</div>" >> $html


# End of script
echo "</body></html>" >> $html
echo
echo -e "${green}End of drawing script.${default}"
