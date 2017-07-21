import os

def newpath(dirpath, inputfilename, newextension):
    _, tail = os.path.split(inputfilename)
    return dirpath + tail.split('.')[0] + newextension
