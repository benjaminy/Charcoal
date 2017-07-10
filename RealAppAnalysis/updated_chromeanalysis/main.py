import profileparser as parser
import cpuprofileparser as cpuparser
import durationeventsanalyzer as duranalyzer
import profileanalyzer
import utils
import os


def main():
    for (filepath, sub_directories, files) in os.walk("profiles"):
        utils.log(filepath, tag = "Filepath")
        utils.log(sub_directories, tag = "Subdirectories")
        utils.log(files, tag = "Files")
        for file in files:
            utils.log(file, tag = "Processing file")

            profileanalyzer.analyze_profile( os.path.join(filepath, file)  )

if __name__ == '__main__':
    main()
