#!/usr/bin/python
import string
import random
import sys

filename = sys.argv[1]
in_f = open(filename, 'r')
out_f = open(filename + '.prob', 'w')

for line in in_f:
    if line[0] == '#':
        continue
    data = string.split(line)
    sep = '\t'
    output = string.join(data, sep) + sep + '1' + sep + str(random.random()) + '\r\n'
    out_f.write(output)
    
in_f.close()
out_f.close()

