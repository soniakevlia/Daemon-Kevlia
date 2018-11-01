#!/Daemon-master-Kevlia/bash


mkdir ./folder2
touch ./folder2/total.log

mkdir ./folder1

./daemon start

cp -a ./files/. ./folder1

sleep 5

cp -a ./files/. ./folder1

sleep 5

cp -a ./files/. ./folder1

sleep 5;

./daemon stop


