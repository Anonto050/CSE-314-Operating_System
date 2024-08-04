#!/bin/bash

mkdir -p output_dir


folder="input_dir"


for file in $(find $folder -type f -name "*.*")
do
    
    name=$(basename "$file" | cut -d'.' -f1)
    number=${#name}
    echo "$number"

    #see if folder already exist with this name
    flag=0

    for dir in output_dir/*
    do
       extension=$(basename "$dir")
       echo "$extension"
       echo "$number"
       if [ "$extension" == "number" ];then
          flag=1
       fi
       #ls -S
    done

    if [ $flag -eq 1 ];then
         cp -r $file output_dir/$number
    else
         mkdir -p output_dir/$number
    cp -r $file output_dir/$number

    fi

   

done

for dir in output_dir/*
    
    do
       cd "$dir"
       echo "$dir"
       ls -S | sort
    done