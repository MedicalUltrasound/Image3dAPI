from __future__ import print_function
import fileinput
import sys
from xml.dom import minidom


def UpdateAutopkg (filename, version, url):
    '''Set the minor version in an autopkg file'''
    
    for line in fileinput.input(filename, inplace=True):
        if '<<VERSION>>' in line:
            print(line.replace('<<VERSION>>', version))
        elif '<<SOURCE-URL>>' in line:
            print(line.replace('<<SOURCE-URL>>', url))
        else:
            print(line, end='')


if __name__ == "__main__":
    filename = sys.argv[1]
    version  = sys.argv[2]
    url      = sys.argv[3]
    
    print('Setting version and URL of '+filename+' to '+version+' and '+url)
    UpdateAutopkg(filename, version, url)
