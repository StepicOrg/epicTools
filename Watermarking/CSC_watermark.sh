#!/bin/sh
for f in *.mp4 ; 
do ffmpeg -i $f -i watermarklogo.png -filter_complex "overlay=(main_w-overlay_w-10):(main_h-overlay_h-10)" -codec:a copy ${f%.mp4}wtrmrk.mp4; done
