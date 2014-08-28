#include <stdio.h>
#include <cv.h>
#include <opencv/cxcore.h>
#include <opencv/highgui.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>


const int _TRESHOLD = 35;
const int _FFMPEG_CMND_SLICES = 6;
const std::string ffmpeg_cmnd_1 = "ffmpeg -i ";
const std::string ffmpeg_cmnd_2 = " -ss ";
const std::string ffmpeg_cmnd_3 = " -to ";
const std::string ffmpeg_cmnd_4 = " -vcodec libx264 -c:a copy part";
const std::string ffmpeg_cmnd_5 = ".mp4 -force_key_frames ";
const std::string ffmpeg_cmnd_6 = " -force_key_frames ";
const std::string ffmpeg_arr[_FFMPEG_CMND_SLICES] ={ffmpeg_cmnd_1,ffmpeg_cmnd_2,ffmpeg_cmnd_3,ffmpeg_cmnd_4,ffmpeg_cmnd_5,ffmpeg_cmnd_6};

// Returns string command for ffmpeg
//smth like : ffmpeg -i in.mp4 -ss 00:00:03 -to 00:00:09 -c:v copy -c:a copy part1.mp4
std::string ffmpegCutCommand(std::string argv, const std::string * ffmpg_array, const std::string file_cut_start, const std::string file_cut_end, int partNum){
    std::string numStr;
    std::stringstream o;
    o << partNum;
    numStr = o.str();
    std::string out =ffmpg_array[0]+ argv + ffmpg_array[1]+ file_cut_start+ffmpg_array[2]+file_cut_end+ffmpg_array[3]+numStr+ffmpg_array[4]+ file_cut_start + ffmpg_array[5] + file_cut_end;
    return out;
}

//Converts frame number into string code
std::string frameToTime(const int &frameNum, const int &fps){
    char digits[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
    std::string out;
    out = "00:00:00.00";
    //int tmp = frameNum %fps;
    //int frm = 0;
    //(tmp == 0) ? (frm = 0) : (frm  = (100*fps)/(frameNum % fps));
    int frm = 0;
    if (frameNum != 0) {
        frm = (100* (frameNum%fps))/fps;
    }
    std::cout<<"FRM = "<<frm<<" FRAME = "<< frameNum << "\n";
    int min = (frameNum / fps) / 60;
    int sec = (frameNum / fps) % 60;
    out[3] = digits[min/10];
    out[4] = digits[min%10];
    out[6] = digits[sec/10];
    out[7] = digits[sec%10];
    out[9] = digits[frm/10];
    out[10] = digits[frm%10];
    return out;
}

//Creates txt file with cuts
void generateJoinList(std::vector<std::string> &cuts){
    std::ofstream joinList;
    joinList.open ("joinlist.txt");
    if 	(joinList.is_open()){
        unsigned long size = cuts.size();
        size = (size+1)/ 2;
        std::cout<<"SIZE = "<<size;
        for (int i = 0; i < size; i++){
            joinList<<"file 'part"<<i<<".mp4'\n";
        }
    } else {
        std::cout<<"ERROR WHILE OPENING FILE";
        return;
    }
    joinList.close();
}

int main(int argc,  char** argv)
{
    //argv[1] should contain file name with extention
    if (argc < 2){
        std::cout<<"No filename to analyse found.\n";
        return -3;
    }
    
    //Contains all timecodes
    std::vector<std::string> cuts;
    
    //file part.
    //Do we really need it?? Not sure
    std::ofstream cut_log;
    cut_log.open ("cut_log.txt");
    
    if 	(cut_log.is_open()){
        int key = 0;
    
        CvCapture * capture = cvCaptureFromFile(argv[1]);
        cv::VideoCapture cap(argv[1]);
        cv::Mat image_mat;
        cap.read(image_mat);
        CvMat oldMat = image_mat;
        //initialization of scal
        CvScalar scal = cvGet2D( &oldMat,0,0);
        IplImage* frame = cvQueryFrame( capture );
        IplImage* currframe = cvCreateImage(cvGetSize(frame),IPL_DEPTH_8U,3);
        IplImage* destframe = cvCreateImage(cvGetSize(frame),IPL_DEPTH_8U,3);
    
        if ( !capture )
        
        {
            fprintf( stderr, "Cannot open file!\n" );
            return 1;
        }
        if ( !cap.isOpened() )  // if not success, exit program
        {
            std::cout << "Cannot open the video file" << std::endl;
            return -1;
        }
    
    
        int fps = ( int )cvGetCaptureProperty( capture, CV_CAP_PROP_FPS );
        std::cout << fps << " <<FPS\n";
        //generate matrix of framesize
        CvMat *mat = cvCreateMat(destframe->height,destframe->width,CV_32FC3 );
    
        int frameNum = 0;
        int prev_cutFrameNum = 0;
        long int summ = 0;
        std::string frameTime;
        bool cut_Flag = true;
        //system(console_ffmpeg_command);
        while( key != 'x' )
        {
            frame = cvQueryFrame( capture );
            if (!frame) break;
            currframe = cvCloneImage( frame );
            frame = cvQueryFrame( capture );
            if (!frame) break;

            cvSub(frame,currframe,destframe);
            cvConvert( destframe, mat);
            summ = 0;
            
            //Remove hardcoded HD defenition
            for(int i=0;i<1920;i++)
                    {
                        for(int j=0;j<1080;j++)
                        {
                            scal = cvGet2D( mat,j,i);
                            if (scal.val[0] > _TRESHOLD) {
                                summ += scal.val[0];
                                //std::cout<<i<<" "<<j<<" "<<scal.val[0]<<"\n";
                            }
                        }
                    }
        
            //If motion detected, set marker to stop
            if (summ != 0) {
                if (!cut_Flag){
                    if (!(frameNum - prev_cutFrameNum < fps)){
                        cut_Flag = true;
                        frameTime = frameToTime(frameNum, fps);
                        std::cout<<"x00 "<<frameTime<<" \t#Include from this frame: MOTION DETECTED \n";
                        cut_log<<"x00 "<<frameTime<<" \t#Include from this frame: MOTION DETECTED \n";
                        cuts.push_back(frameTime);
                        prev_cutFrameNum = frameNum;
                    }
                }
            }
        
            //If there is no motion in frame
            if (summ == 0 && cut_Flag){
                if (!(frameNum - prev_cutFrameNum < fps) || frameNum == 0){
                    cut_Flag = false;
                    frameTime = frameToTime(frameNum, fps);
                    std::cout<<"x01 "<<frameTime<<" \t#Cut here: NO MOTION FROM THIS FRAME \n";
                    cut_log<<"x01 "<<frameTime<<" \t#Cut here: NO MOTION FROM THIS FRAME \n";
                    cuts.push_back(frameTime);
                    prev_cutFrameNum = frameNum;
                }
            }
        
            if(key==27 )break;
            //cvShowImage( "dest",destframe);
            key = cvWaitKey( 1000 / fps );
            
            //We increment frames by two because it's faster. May be not so accurate, but works fine for now.
            frameNum += 2;
            //cvReleaseImage(&frame);
            cvReleaseImage(&currframe);
        }
        cvDestroyWindow( "dest" );
        cvReleaseCapture( &capture );
        cut_log.close();
        
        //Start cutting procces
        std::string command = "";
        if (cuts[0] == "00:00:00.00"){
            for(int i = 0; i+1 < cuts.size();i += 2){
                command = ffmpegCutCommand(argv[1],ffmpeg_arr,cuts[i+1],cuts[i+2],(i+1)/2);
                system(&command[0]);
            }
            
        ///This part not tested yet
        } else {
            command = ffmpegCutCommand(argv[1],ffmpeg_arr,"00:00:00.00",cuts[0],0);
            system(&command[0]);
            for(int i = 1; i+1 < cuts.size();i += 2){
                command = ffmpegCutCommand(argv[1],ffmpeg_arr,cuts[i],cuts[i+1],(i+1)/2);
                system(&command[0]);
            }
        }
        
        ////Add here ffmpeg commands for concatenating filese
        ////example: ffmpeg -f concat -i joinlist.txt -c copy joinedfile.mp4
        ////joinlist.txt should look like:
        ////
        ////file 'part1.mp4'
        ////file 'part2.mp4'
        
        generateJoinList(cuts);
        system("ffmpeg -f concat -i joinlist.txt -c copy joinedfile.mp4");
        
        
        return 0;
    } else {
        std::cout << "Unable to open file";
        return -2;
    }
}