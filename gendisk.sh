#!/bin/bash
cp programs/helloworld/helloworld disk/HLWORLDC.OUT

device=$(sudo losetup -Pf --show disk.img)
partition="${device}p1"
sudo mount $partition disk_mnt/
sudo cp -r disk/* disk_mnt/
sudo umount $partition

