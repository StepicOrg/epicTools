__author__ = 'mehanig'
import subprocess as sp
from optparse import OptionParser
import sys
from os import listdir
from os.path import isfile, join


PATH = "."
FFMPEG_BIN = "ffmpeg"
OUTPUT_FILE = "volume.txt"
NORMALIZE = "-15"
EXT = ".mp4"


#Command generator for volume detection
def command_volume(FILE):
    command_out = [FFMPEG_BIN,
            '-y',
            '-analyzeduration', '500000000',
            '-threads', '8',
            '-i', FILE,
            '-af', 'volumedetect',
            '-f', 'null',
            '-']
    return command_out


#command generator for volume normalization
def command_to_normalize(input_file, vol):
        file_name = input_file.split("/")[-1]
        (file_name, extention) = (file_name.split(".")[0],file_name.split(".")[-1])
        #extention = input_file.split(".")[-1]
        command_out = [FFMPEG_BIN,
            '-threads', '8',
            '-i', input_file,
            '-af', 'volume='+str(vol)+'dB',
            '-c:v', 'copy',
            file_name+"norm_"+str(int(vol)) + "." + extention]
        return command_out


#get info about file volume
def get_sound_info(input_file):
    pipe = sp.Popen(command_volume(input_file), stdout=sp.PIPE, stdin=sp.PIPE,  stderr=sp.PIPE, bufsize=10**8)
    infos = pipe.stderr.read()
    info_start = infos.decode('utf-8').find('mean_volume:')
    mean_vol = (infos[info_start:]).decode('utf-8').split("\n")[0].split(" ")[1]
    info_start = infos.decode('utf-8').find('max_volume:')
    max_vol = (infos[info_start:]).decode('utf-8').split("\n")[0].split(" ")[1]
    return float(mean_vol), float(max_vol), input_file


#runs normalization for one file
def normalize_volume(input_file, new_mean_vol):
    file_mean = get_sound_info(input_file)[0]
    if file_mean < new_mean_vol:
        print(input_file + " normalized to mean volume " + str(new_mean_vol))
        db_to_add = abs(new_mean_vol - file_mean)
        sp.Popen(command_to_normalize(input_file, db_to_add), stdout=sp.PIPE, stdin=sp.PIPE,  stderr=sp.PIPE, bufsize=10**8)


#Command for normalising files
if __name__ == "__main__":

    parser = OptionParser()
    parser.add_option("-p", "--path", dest="PATH", default=PATH,
                      help="Set folder path to analyze. Default = Current folder")
    parser.add_option("-e", "--ext", dest="EXT", default=EXT,
                      help="Set input files extention. Default = " + EXT)
    parser.add_option("-o", "--output", dest="OUTPUT_FILE", default=OUTPUT_FILE,
                      help="Set output log file. Default = " + OUTPUT_FILE)
    parser.add_option("-n", "--normalize", dest="normalize", default=False,
                      help=(" Set mean value to all files in folder, 'auto' for "+NORMALIZE+"dB"))

    #parse args
    (options, sys.argv[1:]) = parser.parse_args(sys.argv[1:])
    if options.PATH[-1] != "/":
        options.PATH += "/"
    #clear file
    output_file = open(options.PATH + options.OUTPUT_FILE, "w")
    #List all files
    onlyfiles = [f for f in listdir(options.PATH) if isfile(join(options.PATH, f)) and f.endswith(options.EXT)]

    if not options.normalize:
        for curr_file in onlyfiles:
            output_file = open(options.PATH + options.OUTPUT_FILE, "a")
            info = (get_sound_info(options.PATH + curr_file))
            print(info)
            output_file.write(str(info) + "\n")
            output_file.close()

    else:
        if options.normalize == "auto":
            options.normalize = NORMALIZE
        for curr_file in onlyfiles:
                normalize_volume(options.PATH + curr_file,float(options.normalize))

