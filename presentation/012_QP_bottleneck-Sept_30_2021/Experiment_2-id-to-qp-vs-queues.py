import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np
from common_library import *

from svgpathtools import svg2paths
from svgpath2mpl import parse_path


skull_path, skull_attributes = svg2paths('/home/ena/Pictures/matplotlib_custom_markers/Skull_and_Crossbones.svg')
skull_marker = parse_path(skull_attributes[0]['d'])
skull_marker.vertices -= skull_marker.vertices.mean(axis=0)
skull_marker = skull_marker.transformed(mpl.transforms.Affine2D().rotate_deg(180))
skull_marker = skull_marker.transformed(mpl.transforms.Affine2D().scale(-1,1))


def death_marker(ax, threads, throughput, color):
    ax.plot(threads[len(threads)-1:],throughput[len(throughput)-1:], marker=skull_marker, markersize=20, color=color)





throughput_64_qp=[44287,47938,50053,51806,49811,52664,52956,54340,55329,55879,56628,56844,57501,57204,58273,58132,58169,58635,58774,58991,59233,59250,58862,58699,58888,58868,55634,]
threads_64_qp=[   6,    8,    10,   12,   14,   16,   18,   20,   22,   24,   26,   28,   30,   32,   34,   36,   38,   40,   42,   44,   46,   48,   50,   58,   60,   62,   64,]


throughput_48_qp=[28453,38993,43822,47070,50210,52068,51422,52197,53343,52555,55054,56262,55969,56708,57440,57920,58102,58373,57886,58968,58589,59178,58836,58680,58623,59214,56917,58770,58813,58919,57772,58451,]
threads_48_qp=[   2,    4,    6,    8,    10,   12,   14,   16,   18,   20,   22,   24,   26,   28,   30,   32,   34,   36,   38,   40,   42,   44,   46,   48,   50,   52,   54,   56,   58,   60,   62,   64,]


throughput_24_qp=[27975,38917,44284,47618,50482,51171,51348,51577,53537,53585,55384,55685,]
threads_24_qp=[2,4,6,8,10,12,14,16,18,20,22,24,]

throughput_12_qp=[28506,38907,44356,47874,49532,52363,]
threads_12_qp=[2,4,6,8,10,12,]

throughput_6_qp=[28507,39165,44190,]
threads_6_qp=[2,4,6,]

throughput_3_qp=[28505,39094,]
threads_3_qp=[2,4,]

throughput_2_qp=[28369,38908,]
threads_2_qp=[2,4,]

throughput_1_qp=[28486,]
threads_1_qp=[2,]

#http://mkweb.bcgsc.ca/colorblind/img/colorblindness.palettes.v11.pdf
color_64="#2271B2"
color_48="#3DB7E9"
color_24="#F748A5"
color_12="#359B73"
color_6="#D55E00"
color_3="#E69F00"
color_2="#F0E442"
color_1="#000000"

throughput_64_qp=div_thousand(throughput_64_qp)
throughput_48_qp=div_thousand(throughput_48_qp)
throughput_24_qp=div_thousand(throughput_24_qp)
throughput_12_qp=div_thousand(throughput_12_qp)
throughput_6_qp=div_thousand(throughput_6_qp)
throughput_3_qp=div_thousand(throughput_3_qp)
throughput_2_qp=div_thousand(throughput_2_qp)
throughput_1_qp=div_thousand(throughput_1_qp)

fig, ax = plt.subplots()
ax.plot(threads_64_qp,throughput_64_qp, marker='x', label="64", color=color_64) #, width, label=treatment_label, color='tab:blue', hatch='\\')
ax.plot(threads_48_qp,throughput_48_qp, marker='x', label="48", color=color_48) #, width, label=treatment_label, color='tab:blue', hatch='\\')
ax.plot(threads_24_qp,throughput_24_qp, marker='x', label="24", color=color_24) #, width, label=treatment_label, color='tab:blue', hatch='\\')
ax.plot(threads_12_qp,throughput_12_qp, marker='x', label="12", color=color_12) #, width, label=treatment_label, color='tab:blue', hatch='\\')
ax.plot(threads_6_qp,throughput_6_qp, marker='x', label="6", color=color_6) #, width, label=treatment_label, color='tab:blue', hatch='\\')
ax.plot(threads_3_qp,throughput_3_qp, marker='x', label="3", color=color_3) #, width, label=treatment_label, color='tab:blue', hatch='\\')
ax.plot(threads_2_qp,throughput_2_qp, marker='x', label="2",color=color_2) #, width, label=treatment_label, color='tab:blue', hatch='\\')
ax.plot(threads_1_qp,throughput_1_qp, marker='x', label="1",color=color_1) #, width, label=treatment_label, color='tab:blue', hatch='\\')

death_marker(ax,threads_24_qp,throughput_24_qp, color_24)
death_marker(ax,threads_12_qp,throughput_12_qp, color_12)
death_marker(ax,threads_6_qp,throughput_6_qp, color_6)
death_marker(ax,threads_3_qp,throughput_3_qp, color_3)
death_marker(ax,threads_2_qp,throughput_2_qp, color_2)
death_marker(ax,threads_1_qp,throughput_1_qp, color_1)
#ax.plot(threads_12_qp[len(threads_12_qp)-1:],throughput_12_qp[len(threads_12_qp)-1:], marker="X")

# Add some text for labels, title and custom x-axis tick labels, etc.
ax.set_ylabel('KOP/s')
ax.set_title('Memory Side Queue Pairs (id mapping)')
ticks = range(0,65)
ticks = [x for x in ticks if not x % 4]
ax.set_xticks(ticks)
ax.set_xlabel("Client Threads")
ax.legend()


fig.tight_layout()
save(plt)
#plt.show()