for f in ./*.mp4; do echo "file '$f'" >> cut_list.txt; done
mkdir Concated_video
ffmpeg -f concat -i cut_list.txt -c copy Concated_video/Concated_Video.mp4
for f in Concated_video/*.mp4 ; 
do ffmpeg -i $f  -codec:a copy -vcodec libx264 -preset slow -crf 30 ${f%.mp4}_compressed.mp4; done
rm cut_list.txt
rm Concated_video/Concated_Video.MP4
