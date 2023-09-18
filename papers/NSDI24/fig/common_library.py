import os
import sys
import matplotlib.pyplot as plt

#name of the file, full system performance
def save(plt_local):
    figure_name, ext = os.path.splitext(sys.argv[0])
    plt_local.savefig(figure_name+'.pdf')
    plt_local.savefig(figure_name+'.png')

def div_thousand (list):
    return [val /1000.0 for val in list]