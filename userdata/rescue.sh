#!/bin/bash

scriptDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
name="$(date +%Y-%m-%d-%H-%M-%S)"
res="$scriptDir/results/$name"
pushd "$scriptDir/.." > /dev/null
prog="$(pwd)"
popd > /dev/null

source common.sh

trap "echo && echo COWARD && exit -12" SIGINT SIGTERM

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
    nbruns=10
    ratelist=(0.1 0.5 1 1.5 2 2.5 3 4 5 6)
    #explist=(2 2.53 2.807 2.887 2.953 3.009)
    #snrlist=(-0.05 -0.45 -0.85 -1.25 -1.65 -2.05 -2.45)
    explist=(1.6 1.98 2.16 2.382 2.56 2.745 2.859 2.945 3.015 3.076 3.13)
    snrlist=(-0.01 -0.05 -0.10 -0.25 -0.50 -1.0 -1.5 -2.0 -2.5 -3.0 -3.5)
else
    nbruns=2
    ratelist=(2 4 8)
    explist=(1.6 2.745)
    snrlist=(-0.01 -1.0)
fi


mkdir -p $res



#Save chosen parameters
for (( e=0; e < ${#ratelist[@]}; e++ )) ; do
    echo ${ratelist[$e]} >> $res/ratelist.config
done;

for (( e=0; e < ${#explist[@]}; e++ )) ; do
    echo ${explist[$e]} >> $res/explist.config
done;

for (( e=0; e < ${#snrlist[@]}; e++ )) ; do
    echo ${snrlist[$e]} >> $res/snrlist.config
done;


nbsimu=$(echo ${#explist[@]}*${#ratelist[@]}*$nbruns | bc)
echo -e "${green}Running $nbsimu simulations ($nbruns for each set of parameters) for both WiFi and RESCUE${default}"

for prefix in "wifi" "rescue" ; do
    simu=1
    expoffset=0
    for exp in ${explist[@]} ; do
        for r in ${ratelist[@]} ; do
            tmpwd=$PWD
            cd $prog
            for run in $(seq 1 $nbruns) ; do
                tmpwd=$PWD
                cd $prog
                ret=0
                try=0
                until [ $ret -eq 1 ] ; do
                    seed=$(echo "$RANDOM + $try * 10000" | bc)
                    if [[ "$prefix" == "wifi" ]] ; then
                        #Skip some wifi simulations
                        if [[ $expoffset -gt 3 ]] ; then
                            echo "skip"
                            echo "100 0 0 0 0 0" >  $res/$prefix-$exp-$r-$run.tmp
                            echo "PROPER END" >>  $res/$prefix-$exp-$r-$run.tmp
                        else
                            ./waf --run "toy5wifi --exponent=$exp --dataRate=$r --seed=$seed"  > $res/$prefix-$exp-$r-$run.tmp 2>&1
                        fi

                    elif [[ "$prefix" == "rescue" ]] ; then
                        ./waf --run "toy5 --exponent=$exp --dataRate=$r --seed=$seed"  > $res/$prefix-$exp-$r-$run.tmp 2>&1
                    else
                        echo "${bred}UNKNOWN PREFIX$ ! ${default}"
                        exit -1
                    fi

                    if [[ $try -eq 4 ]] ; then
                        echo "FAIL SIMU"
                        echo "100 0 0 0 0 0 0" >  $res/$prefix-$exp-$r-$run.tmp
                        echo "PROPER END" >>  $res/$prefix-$exp-$r-$run.tmp
                    fi

                    if  grep -q 'PROPER END' $res/$prefix-$exp-$r-$run.tmp ; then
                        echo -e "${green}$simu/$nbsimu ${default}: ${bblue}$prefix${default} exp=$exp dataRate=$r seed=$seed"
                        ret=1
                    else
                        echo -e "${red}$simu/$nbsimu ${default}: ${bblue}$prefix${default} exp=$exp dataRate=$r ${red}seed=$seed${default}"
                    fi;
                    ((try++))
                done;
                ((simu++))
                echo "$(tail -n 2 $res/$prefix-$exp-$r-$run.tmp | head -n 1)" > $res/$prefix-$exp-$r-$run
                echo "$(tail -n 2 $res/$prefix-$exp-$r-$run.tmp | head -n 1)" >> $res/$prefix-$exp-$r-all
            done
            cd $tmpwd
            unset tmpwd
        done
        ((expoffset++))
    done
    cd $scriptDir
done

if [[ ! -f $res/info ]] ; then
    echo "nbruns : $nbruns" >> $res/info
fi

if [[ $draw -eq 1 ]] ; then
    ./rescue_results.sh $res 1
else
    echo -e "${red}You should run ${bold}rescue_results${normal} to create graphics on computed stats. $res folder has NOT been removed. You also can pass ${bold}--draw${normal} as an option to this script to automatically draw results. ${default}"
fi

if [ "$(ps aux | grep "$0" | head -n1 | cut -d' ' -f1)" == "root" ] ; then
    user=$(who | cut -d" " -f1 | head -n1)
    chown -R $user:$user $res
    echo -e "${yellow}[INFO] Script launched as ${bold}root${normal}, $res has been chowned to ${bold}$user${normal} user and group.${default}"
fi

echo -e "${green}End of simulation script.${default}"
exit 0
