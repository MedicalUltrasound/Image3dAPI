# Script to determine highest tag number and increment version
from __future__ import print_function
from distutils.version import StrictVersion
import subprocess
import sys


def DetermineHighestTagVersion ():
    # retrive all tags
    cmd = 'git tag'
    p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    tags = []
    for line in p.stdout.readlines():
        line = line.decode()
        line = line.strip(' /\r\n') # remove '/' & newline suffix
        tags.append(line)
    
    # identify newest tag
    newest_tag = tags[0]
    for tag in tags:
        try:
            if StrictVersion(tag) > StrictVersion(newest_tag):
                newest_tag = tag
        except:
            print('Ignoring unparsable tag '+tag, file=sys.stderr)
    
    return newest_tag


def IncrementVersionNumber (version, incr_idx):  
    # tokenize version
    tokens = version.split('.')
    
    # append zero tokens to avoid out-of-bounds access
    while incr_idx >= len(tokens):
        tokens.append('0')
    
    # increment token
    tokens[incr_idx] = str(int(tokens[incr_idx])+1)
    
    # set lower version indices to zero (eg. go from '0.9' to '1.0')
    for i in range(incr_idx+1, len(tokens)):
        tokens[i] = '0'
    
    # joint tokens into new version string
    return '.'.join(tokens)


if __name__ == "__main__":
    incr_type = sys.argv[1] # version increment type (major, minor, patch)
    
    # determine which version index to increment
    if incr_type   == 'major':  incr_idx = 0
    elif incr_type == 'minor':  incr_idx = 1
    elif incr_type == 'patch':  incr_idx = 2
    else:  raise Exception('unknown incr_type')
    
    prev_ver = DetermineHighestTagVersion()
    new_ver  = IncrementVersionNumber(prev_ver, incr_idx)
    
    # output new version number to stdout, without newline suffix
    sys.stdout.write(new_ver)
