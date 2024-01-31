#!/bin/sh

file_path=$1
content=$2

if [ -z "$file_path" ]
then
    echo "file_path is empty."
    exit 1
fi

if [ -z "$content" ]
then
    echo "content is empty."
    exit 1
fi

file_dir="$(dirname "${file_path}")"
mkdir -p $file_dir
echo $content > $file_path
