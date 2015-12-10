#!/bin/bash

scriptDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
name="$(date +%Y-%m-%d-%H-%M-%S)"
res="$scriptDir/results/$name"
pushd "$scriptDir/.." > /dev/null
prog="$(pwd)"
popd > /dev/null

source common.sh

trap "echo && echo STOP EVERYTHING && exit -12" SIGINT SIGTERM

#Colors for fun

echo
echo -e "${green}${bold}***************************************"
echo -e "***************************************"
echo -e "********** Simulation script **********"
echo -e "***************************************"
echo -e "***************************************${default}"
echo

tabs 4
function printopt {
    if [ "$#" -ne 1 ] ;then
	    echo "Usage: ... $#"
	    exit 1
    else
	    echo -e "${bpurple}\"--$1\"$<{default}${bold}"
    fi
}
draw=0
nocolors=0
real=0
simfile="toy5"
for param in "$@" ; do
    case $param in
        "--help" )
            echo -e "${bold}Help :"
            echo -e "\t 1 - The script can run four types of simulation, described below. You can use any combination, or even use $(printopt full) to start them all."
            echo -e "\t\t a - $(printopt normal) to make RTT and PER vary."
            echo -e "\t\t b - $(printopt chunk) to make cell size and cell number (in a chunk) vary."
            echo -e "\t\t c - $(printopt withper) to make PER and PER packet interval vary."
            echo -e "\t\t d - $(printopt timers) to make RTT and sender's timeout vary."
            echo
            echo -e "\t 2 - This script can run two kind of sets. Choose one of the following :"
            echo -e "\t\t a - $(printopt real) to run the large set of tests"
            echo -e "\t\t b - $(printopt test) to run the small set of tests"
            echo
            echo -e "\t 3 - Depending on the parameters, this script can be very long to execute. To shorten its execution time, you can decrease its niceness (increased CPU priority) by typing ${purple}\"sudo nice --20 $0\"${default}${bold}."
            echo
            echo -e "\t 4 - To plot simulation stats, you must use $(printopt draw)."
            echo
            echo -e "\t 5 - To run simulations with feedback channel losses, use $(printopt ackloss)."
            echo
            echo -e "\t 6 - To use the ACK reliability feature, use the $(printopt ncf) option. $(printopt acksave) can be used to re-send the exact same ACK without re-computation."
            echo
            echo -e "\t 7 - The events file can be used to provide topology changes. The following options are available :"
            echo -e "\t\t a - $(printopt eventrtt) to change some RTT during simulations"
            echo -e "\t\t b - $(printopt eventper) to change some PER during simulations"
            echo
            echo -e "\t 8 - To use the simulation with 24 receivers, use the $(printopt multi) option. Random PER can be set on each link using $(printopt randomper)."
            echo
            echo -e "\t 9 - The $(printopt variable) option can be used to send packets of variable payload size at a variable rate (new figures drawn)."
            echo
            echo -e "\t 10 - To use the PER deduction feature, use the $(printopt deduction) option."
            echo
            echo -e "\t 11 - To run simulations with larger files, use $(printopt large) or $(printopt huge)."
            echo
            echo -e "\t 12 - To set the color option off (if you cannot see this message in bold with option names in purple, you should), use $(printopt nocolors)."
            echo
            echo -e "\t Example : For a chunk+normal results on the large set of simulation using 24 receivers with feedback channel losses and ACK reliability, type : ${purple}\"$0 --draw --real --multi --ackloss --ncf --normal --chunk\".${default}"
            exit 0 ;;
        #Set of tests
        '--real' )
            real=1;;
        '--test' )
            real=0;;
        '--draw' )
            draw=1;;
        '--nocolors' )
            nocolors=1;;
        '--toy5' )
            simfile="toy5";;
        '--toy6' )
            simfile="toy6-rescue";;
        * )
            echo -e "${red}Unknown parameter ${bold}$param${default}"
            echo -e "${yellow}Run ${bold}\"$0 --help\"${normal} to get information on the avalaible options${default}"
            echo
            exit -1 ;;
    esac;
done;
echo -e "${yellow}Type $0 --help to get information about the different options available.${default}"

if [[ $nocolors == 1 ]] ; then
    default=''
    bold=''
    normal=''
    red=''
    green=''
    yellow=''
    blue=''
    purple=''
fi;


if [[ $real == 1 ]] ; then
    nbruns=5
    txlist=(20.0 19.0 18.0 17.0 16.5 16.2 16.1 16.0)
    ratelist=(0.1 0.5 1 1.5 2 2.5 3 4 5 6)

else
    nbruns=1
    txlist=(20.0 18.0 16.0)
    ratelist=(3 6)
fi


mkdir -p $res

prefixlist=("Simple" "Distributed")

#Save chosen parameters

for (( e=0; e < ${#prefixlist[@]}; e++ )) ; do
    echo ${prefixlist[$e]} >> $res/prefixlist.config
done;

for (( e=0; e < ${#txlist[@]}; e++ )) ; do
    echo ${txlist[$e]} >> $res/txlist.config
done;

for (( e=0; e < ${#ratelist[@]}; e++ )) ; do
    echo ${ratelist[$e]} >> $res/ratelist.config
done;

echo $simfile >> $res/simfile.config

nbsimu=$(echo ${#ratelist[@]}*${#txlist[@]}*$nbruns | bc)
echo -e "${green}Running $nbsimu simulations ($nbruns for each set of parameters) for both SimpleMAC and DistributedMAC on simulation file ${simfile}.cc${default}"

cd $prog || exit

for prefix in ${prefixlist[@]} ; do
    simu=1
    #expoffset=0
    #for exp in ${explist[@]} ; do
    for dr in ${ratelist[@]} ; do
        for tx in ${txlist[@]} ; do
            for run in $(seq 1 $nbruns) ; do
                # tmpwd=$PWD
                # cd $prog
                ret=0
                try=0

                tmpfile=$res/$prefix-$tx-$dr-$run.tmp
                filerun=$res/$prefix-$tx-$dr-$run
                filerunall=$res/$prefix-$tx-$dr-all
                until [ $ret -eq 1 ] ; do
                    seed=$(echo "$RANDOM + $try * 10000" | bc)
                    # if [[ "$prefix" == "wifi" ]] ; then
                    #     #Skip some wifi simulations
                    #     if [[ $expoffset -gt 3 ]] ; then
                    #         echo "skip"
                    #         echo "100 0 0 0 0 0" >  $res/$prefix-$exp-$r-$run.tmp
                    #         echo "PROPER END" >>  $res/$prefix-$exp-$r-$run.tmp
                    #     else
                    #         ./waf --run "toy5wifi --exponent=3.0 --dataRate=$r --seed=$seed"  > $res/$prefix-$exp-$r-$run.tmp 2>&1
                    #     fi

                    # elif [[ "$prefix" == "rescue" ]] ; then
                    #     ./waf --run "toy5 --${prefix} --exponent=3.0  --dataRate=$r --seed=$seed"  > $res/$prefix-$exp-$r-$run.tmp 2>&1
                    # else
                    #     echo "${bred}UNKNOWN PREFIX$ ! ${default}"
                    #     exit -1
                    # fi
                    ./waf --run "${simfile} --${prefix} --simTime=2 --exponent=3.0 --TxPower=$tx --dataRate=$dr --seed=$seed"  > $tmpfile 2>&1

                    if [[ $try -eq 4 ]] ; then
                        echo "FAIL SIMU"
                        echo "100 0 0 0 0 0 0 0" >  $tmpfile
                        echo "PROPER END" >>  $tmpfile
                    fi

                    if  grep -q 'PROPER END' $tmpfile ; then
                        echo -e "${green}$simu/$nbsimu ${default}: ${bblue}$prefix${default} TxPower=$tx Datarate=$dr seed=$seed"
                        ret=1
                    else
                        echo -e "${red}$simu/$nbsimu ${default}: ${bblue}$prefix${default} TxPower=$tx Datarate=$dr ${red}seed=$seed${default}"
                    fi;
                    ((try++))
                done;
                ((simu++))
                grep 'SIM_STATS' $tmpfile | cut -d' ' -f2- > $filerun
                grep 'SIM_STATS' $tmpfile | cut -d' ' -f2- >> $filerunall
                #rm $tmpfile
            done
        done
    done
done

#Extract SNR on DistributedMAC results for each Tx
for tx in ${txlist[@]} ; do
    tmpfile=$res/${prefixlist[1]}-$tx-${ratelist[0]}-1.tmp
    if [[ -f $tmpfile ]] ; then
        grep 'SIM_STATS' $tmpfile | cut -d' ' -f9 >> $res/snrlist.config
    fi
done
cd $res || exit
rm *.tmp

cd $scriptDir || exit

if [[ ! -f $res/info ]] ; then
    echo "nbruns : $nbruns" >> $res/info
    echo "txlist : ${txlist[@]}" >> $res/info
    echo "ratelist : ${ratelist[@]}" >> $res/info
    echo "snrlist : ${snrlist[@]}" >> $res/info
fi

if [[ $draw -eq 1 ]] ; then
    ./new_rescue_results.sh $res 1
else
    echo -e "${red}You should run ${bold}new_rescue_results.sh${normal} to create graphics on computed stats. $res folder has NOT been removed. You also can pass ${bold}--draw${normal} as an option to this script to automatically draw results. ${default}"
fi

if [ "$(ps aux | grep "$0" | head -n1 | cut -d' ' -f1)" == "root" ] ; then
    user=$(who | cut -d" " -f1 | head -n1)
    chown -R $user:$user $res
    echo -e "${yellow}[INFO] Script launched as ${bold}root${normal}, $res has been chowned to ${bold}$user${normal} user and group.${default}"
fi

echo -e "${green}End of simulation script.${default}"
exit 0
