#!/bin/bash
cd user_program
find . | cpio -o -H newc > ../initramfs.cpio
cd ..