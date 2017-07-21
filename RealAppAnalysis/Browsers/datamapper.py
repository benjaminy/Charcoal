from os import listdir, getcwd, walk
from os.path import isfile, isdir, join
from json import load
from datautil import log, parseCmdLnArgs, _flagged, _flaggedRetArg
from importlib import import_module
import subprocess
import sys

def main(argv):
    opts, args = parseCmdLnArgs(argv, "hf:p:", ["help", "fileformat=", "post-process-"])
    #log(opts, tag = "opts")
    #log(args, tag = "args")
    
    if _flagged(opts, "-h", "--help"):
        _usage()
        sys.exit(2)
    
    module_name = args[0]
    script = import_module(module_name, package = None)
    dir = args[1]
     
    _isData = _defaultIsData
    
    for opt in opts:
        if "--fileformat" in opt or "-f" in opt:
            log(opt[1], tag = "eSxt")
            _isData = _isDataGenerator(opt[1])
            
    index_of_flags_for_map_script = len(opts) * 2 + 2
    results = onAllDataInDir(dir, 
                   script.main, 
                   func_args = argv[index_of_flags_for_map_script:], 
                   isData = _isData)
    
    post_process_script = _flaggedRetArg(opts, "-p", "--post-process")

    if post_process_script:
        try:
            post_process_module = import_module(post_process_script, package = None)
            post_process_module.main(results)
            
        except ModuleNotFoundError:
            print("Error: Post process module not found")
        
    return results

def _isDataGenerator(ext): return lambda filename: filename.endswith(ext)
 
def _defaultIsData(filename): return filename.endswith(".json")

def onAllDataInDir(root_dir, func, func_args = [], inputfile = True, isData = _defaultIsData):
    results = {}
    for (filepath, sub_directories, files) in walk(root_dir):
        log(filepath, tag = "Filepath")
        log(sub_directories, tag = "Subdirectories")
        log(files, tag = "Files")
        for file in files:
            if(isData(file)):
                log(file, tag = "Processing file") 
                
                #Inputs filepath as an argument to the script
                if inputfile:
                    func_args.append(join(filepath, file))
                    path_without_root = filepath[len(root_dir) + 1:] 
                    results.update({(path_without_root, file): func(func_args)})
                    func_args = func_args[:-1]
                
                #Else, loads the data and inserts that as an argument to the script
                else:
                    try:
                        with open((join(filepath, file)), 'r') as data_file: 
                            data = load(data_file)
                            func_args.append(data)
                            
                    except:
                        print("Error in processing data file: " + file)  
                        continue
                    
                    results.update({(filepath.split("\\")[-1:], file): func(func_args)})
                    func_args = func_args[:-1]
                                      
    return results

def _usage():
    def flag_log(flag, description): log(description, indent = 1, tag = "-" + flag)
    log("Maps a function over the data files found in the provided directory")
    flag_log("f, --fileformat", "Specifies the format of the input file. Default is .json")
    args =  ("<module name>" ,"<directory>", "<script flags>")
    mn, dir, sf = args
    flag_log("args", mn + dir + sf)
    desc = {mn: "The name of the module whose main method can be be passed a \n\
                 filepath or loaded data",
            dir: "The complete path of a directory that contains files of a specified format." ,
            sf: "Flags to be passed into " + mn + "."}
    
    for arg in args:
        log(desc[arg], indent = 2, tag = arg)

if __name__ == "__main__":
    main(sys.argv[1:])    