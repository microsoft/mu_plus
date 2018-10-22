import os
import sys
import math


if __name__ == "__main__":
    #START: -9.4247779607693793      STOP: 9.4247779607693793        STEP:0.039269908169872414
    start = 3
    stop = 3000
    step = 6
    total = (stop - start) / step
    funct = math.sqrt
    
    print("Start: ", start)
    print("Stop: ", stop)
    print("Step: ", step)
    print("Total: ", total)

    allVals = list()

    for index in range(start,stop,step):
        value = round(funct(index))
        allVals.append(value)
    print(allVals)