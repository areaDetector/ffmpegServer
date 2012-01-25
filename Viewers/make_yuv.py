#!/bin/env dls-python2.6

lines = open("colorMaps.h", "r").readlines()

go = False
Ys = []
Us = []
Vs = []
for line in lines:
    if "IronColorRGB[]" in line:
        go = True
        continue
    if go:
        data = line.replace(" ","").rstrip(",};\n").split(",")
        for i in range(len(data)/3):
            R = int(data[i*3], 16)
            G = int(data[i*3+1], 16)
            B = int(data[i*3+2], 16)
            Y = 0.299 * R + 0.587 * G + 0.114 * B
            U = -0.169 * R - 0.331 * G + 0.499 * B + 128
            V = 0.499 * R - 0.418 * G - 0.0813 * B + 128 
            Ys.append("%02x" % Y)
            Us.append("%02x" % U)
            Vs.append("%02x" % V)
        if line.strip().endswith("};"):
            break
        
f = open("newColorMaps.h", "w")


f.write("static const unsigned char IronColorY[] = {\n")
for i, y in enumerate(Ys):
    f.write("  0x%s" %y)
    if i != len(Ys)-1:
        f.write(",")
    if i % 10 == 9:
        f.write("\n")        
f.write(" };\n\n")        

f.write("static const unsigned char IronColorU[] = {\n")
for i, u in enumerate(Us):
    f.write("  0x%s" %u)
    if i != len(Us)-1:
        f.write(",")
    if i % 10 == 9:
        f.write("\n")        
f.write(" };\n\n")  

f.write("static const unsigned char IronColorV[] = {\n")
for i, v in enumerate(Vs):
    f.write("  0x%s" %v)
    if i != len(Vs)-1:
        f.write(",")
    if i % 10 == 9:
        f.write("\n")        
f.write(" };\n\n")  
