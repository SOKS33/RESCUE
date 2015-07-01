#!/bin/bash


echo "<html><body>" >> $res/index.html



echo "<font size=\"5\"><b>Simulation parameters :</b></font><br><br>" >> $res/index.html
echo "<b>Link parameters :</b><br>" >> $res/index.html
echo "&emsp;Link rates : ${ratelist[*]} <br>" >> $res/index.html


#JS
echo "<script language=\"javascript\">" >> $res/index.html
echo "function toggle(id) {" >> $res/index.html
echo "  var tog = \"toggle\";" >> $res/index.html
echo "  var ele = document.getElementById(tog.concat(id));" >> $res/index.html
echo "  var text = document.getElementById(id);" >> $res/index.html
echo "  if(ele.style.display == \"block\") {" >> $res/index.html
echo "    ele.style.display = \"none\";" >> $res/index.html
echo "    text.innerHTML = \"Show \" + id + \" stats\";" >> $res/index.html
echo "  } else {" >> $res/index.html
echo "    ele.style.display = \"block\";" >> $res/index.html
echo "    text.innerHTML = \"Hide \" + id + \" stats\"; }"  >> $res/index.html
echo "}"  >> $res/index.html
echo "</script>" >> $res/index.html

#CSS
echo "<style media=\"screen\" type=\"text/css\"> " >> $res/index.html

echo "td, th { " >> $res/index.html
echo "  text-align : center;" >> $res/index.html
echo "  width : 50px;" >> $res/index.html
echo "  height : 25px;" >> $res/index.html
echo "
}" >> $res/index.html

echo ".myButton {" >> $res/index.html
echo "	-moz-box-shadow:inset 0px 1px 3px 0px #91b8b3;" >> $res/index.html
echo "	-webkit-box-shadow:inset 0px 1px 3px 0px #91b8b3;" >> $res/index.html
echo "	box-shadow:inset 0px 1px 3px 0px #91b8b3;" >> $res/index.html
echo "	background:-webkit-gradient(linear, left top, left bottom, color-stop(0.05, #768d87), color-stop(1, #6c7c7c));" >> $res/index.html
echo "	background:-moz-linear-gradient(top, #768d87 5%, #6c7c7c 100%);" >> $res/index.html
echo "	background:-webkit-linear-gradient(top, #768d87 5%, #6c7c7c 100%);" >> $res/index.html
echo "	background:-o-linear-gradient(top, #768d87 5%, #6c7c7c 100%);" >> $res/index.html
echo "	background:-ms-linear-gradient(top, #768d87 5%, #6c7c7c 100%);" >> $res/index.html
echo "	background:linear-gradient(to bottom, #768d87 5%, #6c7c7c 100%);" >> $res/index.html
echo "	filter:progid:DXImageTransform.Microsoft.gradient(startColorstr='#768d87', endColorstr='#6c7c7c',GradientType=0);" >> $res/index.html
echo "	background-color:#768d87;" >> $res/index.html
echo "	-moz-border-radius:5px;" >> $res/index.html
echo "	-webkit-border-radius:5px;" >> $res/index.html
echo "	border-radius:5px;" >> $res/index.html
echo "	border:1px solid #566963;" >> $res/index.html
echo "	display:inline-block;" >> $res/index.html
echo "	cursor:pointer;" >> $res/index.html
echo "	color:#ffffff;" >> $res/index.html
echo "	font-family:arial;" >> $res/index.html
echo "	font-size:15px;" >> $res/index.html
echo "	font-weight:bold;" >> $res/index.html
echo "	padding:11px 23px;" >> $res/index.html
echo "	text-decoration:none;" >> $res/index.html
echo "	text-shadow:0px -1px 0px #2b665e;" >> $res/index.html
echo "  width: 150px;" >> $res/index.html
echo "  text-align: center;" >> $res/index.html
echo " }" >> $res/index.html

echo " .myButton:hover {" >> $res/index.html
echo "	background:-webkit-gradient(linear, left top, left bottom, color-stop(0.05, #6c7c7c), color-stop(1, #768d87));" >> $res/index.html
echo "	background:-moz-linear-gradient(top, #6c7c7c 5%, #768d87 100%);" >> $res/index.html
echo "	background:-webkit-linear-gradient(top, #6c7c7c 5%, #768d87 100%);" >> $res/index.html
echo "	background:-o-linear-gradient(top, #6c7c7c 5%, #768d87 100%);" >> $res/index.html
echo "	background:-ms-linear-gradient(top, #6c7c7c 5%, #768d87 100%);" >> $res/index.html
echo "	background:linear-gradient(to bottom, #6c7c7c 5%, #768d87 100%);" >> $res/index.html
echo "	filter:progid:DXImageTransform.Microsoft.gradient(startColorstr='#6c7c7c', endColorstr='#768d87',GradientType=0);" >> $res/index.html
echo "	background-color:#6c7c7c;" >> $res/index.html
echo " }" >> $res/index.html

echo ".myButton:active {" >> $res/index.html
echo "	position:relative;" >> $res/index.html
echo "	top:1px;" >> $res/index.html
echo "}" >> $res/index.html

echo "</style>" >> $res/index.html
