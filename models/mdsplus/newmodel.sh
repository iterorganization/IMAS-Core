#!/bin/sh

usage()
{
	echo "Script used to copy MDSplus model files in new user location,"
	echo "and set the relative env variable to use them."
	echo ""
	echo "Usage: . newmodel <path> <tree_name>"
	echo ""
	echo "  <path>:      Full path where to copy model files"
	echo "  <tree_name>: name of the tree, also use in env var"
	echo ""
	echo "IT IS MANDATORY TO CALL THIS SCRIPT THIS A DOT . OR source"
	echo "TO ENSURE THAT ENV VAR ARE PROPERLY SETTED!"
}

if [ $# -lt 2 ] || [ $# -gt 2 ]
then
	usage
else
	if [ -d $1 ]
	then
		name=$2_model
		cp -rp ids_model.characteristics $1/$name.characteristics
		cp -rp ids_model.datafile $1/$name.datafile
		cp -rp ids_model.tree $1/$name.tree
		
		var=$2_path
		export $var=$1/
		
		echo "$var exported, use it in your MDSplus backend"
	else
		echo "$1 is not a directory!"
	fi
fi
