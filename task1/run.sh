#!/bin/sh

make
sudo insmod hello.ko
sudo rmmod hello
dmesg | tail
