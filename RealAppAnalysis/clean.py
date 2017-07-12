from datautil import parseCmdLnArgs, _flagged, log
from datamapper import mapDataInDirectory
import sys
import os

def main(argv):
    opts, args = parseCmdLnArgs(argv, "k", ["keep-only"])
    log(args, tag = "args")
    keepOnly = _flagged(opts, "-k", "--keep-only")
    cleaner = cleanerGenerator(args[1:], keepOnly)
    
    mapDataInDirectory(args[0], cleaner, isData = lambda x: True)

def cleanerGenerator(args, keepOnly):
    def extOf(filepath): return filepath[0].split(".")[-1]
    def notIn(ext, exts):
        for e in exts:
            log(e + " " + ext)
            log(ext not in e, tag = "Not in")
            if ext not in e: 
                return True
        return False
   
    def cleaner(filepath):
        file_type = extOf(filepath)
        log(args, tag = "args")
        if notIn(file_type, args) == keepOnly:
            log(filepath[0], tag = "Deleted")
            os.remove(filepath[0])
            
    return cleaner

if __name__ == "__main__":
    main(sys.argv[1:])