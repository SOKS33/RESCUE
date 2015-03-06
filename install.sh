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

umask 022

echo
echo -e "${green}${bold}*************************************"
echo -e "*************************************"
echo -e "******** Installation script ********"
echo -e "*************************************"
echo -e "*************************************${default}"
echo

version="3.21"
archive="ns-allinone-${version}.tar.bz2"
path="ns-allinone-${version}/ns-${version}"

if [ ! -d $path ] ; then
    rm -f $archive
    echo -e "${bold}${blue}Downloading NS-${version}${default}"
    wget --no-check-certificate https://www.nsnam.org/release/${archive}
    echo -e "${bold}${blue}Extracting NS-${version}${default}"
    tar xjf $archive
    rm -f $archive
    new=1
fi

#rm -rf $path/userdata/*

echo -e "${green}Creating $path/userdata and its subdirectories${default}"
for dir in `find userdata -type d`
do
	mkdir -p  $path/$dir
	#echo $path/$dir
done

for dir in  scratch userdata src
do
	echo -e "${green}Linking files from directory $dir to $path/${default}"
	for file in `find $dir`
	do
        #Create folder if needed
        if [ -d $file ] ; then
            echo "Creating folder $file"
            mkdir -p $path/$file
        fi
        #Link each files
	    if [[ -f $file && ! `echo $file | grep ".svn" `  && ! `echo $file | grep ".git" `  && ! `echo $file | grep "documentation" ` ]] ; then
		rm -f $path/$file
        echo "Linking file $file"
		ln -s `pwd`/$file $path/$file
	    fi
	done
done

if [ ! -d $path/build ] ; then
    echo -e "${red}Build folder does not exist. Will prompt for ns3 configuration and compilation${default}"
    echo -e "${bold}${blue}This is a new installation ! This script will configure and compile NS-3 ...(Takes 5-10 minutes)"

    read -p "Are you sure you want to continue? <y/N> " prompt
    echo -e "${default}"
    if [[ $prompt == "y" || $prompt == "Y" || $prompt == "yes" || $prompt == "Yes" ]] ; then
	    cd $path
	    #CXXFLAGS="-std=c++0x" ./waf --build-profile=debug --disable-examples --disable-tests --disable-python --enable-modules=applications,config-store,core,flow-monitor,internet,mobility,network,nix-vector-routing,olsr,point-to-point,wifi,netanim configure
	    #./waf
        CXXFLAGS="-std=c++0x" ./waf --build-profile=debug --enable-examples --disable-tests --disable-python --enable-modules=rescue,mobility,internet,propagation,applications,flow-monitor configure
        ./waf
    fi
fi

echo -e "${bold}${green}End of installation script ! ${default}"
exit 0
