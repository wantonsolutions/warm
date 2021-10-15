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





#throughput_64_qp=[44287,47938,50053,51806,49811,52664,52956,54340,55329,55879,56628,56844,57501,57204,58273,58132,58169,58635,58774,58991,59233,59250,58862,58699,58888,58868,55634,]
#throughput_48_qp=[28453,38993,43822,47070,50210,52068,51422,52197,53343,52555,55054,56262,55969,56708,57440,57920,58102,58373,57886,58968,58589,59178,58836,58680,58623,59214,56917,58770,58813,58919,57772,58451,]


throughput_64_qp=[73367,87765,84889,83716,79745,75581,70851,67267,62262,58167,]
throughput_32_qp=[73077,88437,87206,83664,79931,75669,71193,66289,59985,55515,]
throughput_16_qp=[73006,88759,85412,84294,78804,74245,70676,65778,58637,54895,]
throughput_8_qp=[73383,87469,86981,83270,78877,74850,70747,64369,60660,54584,]
throughput_4_qp=[73177,87673,87980,83313,78891,74532,69688,64365,58332,56155,]
throughput_2_qp=[73737,87441,87536,84465,80148,75266,70171,65414,59394,55768,]
throughput_1_qp=[73696,87315,88614,84476,80729,75618,69534,63885,61142,56525,]

threads=[2,4,8,16,24,32,40,48,56,64,]

#http://mkweb.bcgsc.ca/colorblind/img/colorblindness.palettes.v11.pdf
#color_128="#2271B2"
color_64="#3DB7E9"
color_32="#F748A5"
color_16="#359B73"
color_8="#D55E00"
color_4="#E69F00"
color_2="#F0E442"
color_1="#000000"

#throughput_64_qp=div_thousand(throughput_64_qp)
#throughput_48_qp=div_thousand(throughput_48_qp)
#throughput_24_qp=div_thousand(throughput_24_qp)
throughput_64_qp=div_thousand(throughput_64_qp)
throughput_32_qp=div_thousand(throughput_32_qp)
throughput_16_qp=div_thousand(throughput_16_qp)
throughput_8_qp=div_thousand(throughput_8_qp)
throughput_4_qp=div_thousand(throughput_4_qp)
throughput_2_qp=div_thousand(throughput_2_qp)
throughput_1_qp=div_thousand(throughput_1_qp)

fig, ax = plt.subplots()
#ax.plot(threads_64_qp,throughput_64_qp, marker='x', label="64", color=color_64) #, width, label=treatment_label, color='tab:blue', hatch='\\')
#ax.plot(threads_48_qp,throughput_48_qp, marker='x', label="48", color=color_48) #, width, label=treatment_label, color='tab:blue', hatch='\\')
#ax.plot(threads_24_qp,throughput_24_qp, marker='x', label="24", color=color_24) #, width, label=treatment_label, color='tab:blue', hatch='\\')
ax.plot(threads,throughput_64_qp, marker='x', label="64", color=color_64) #, width, label=treatment_label, color='tab:blue', hatch='\\')
ax.plot(threads,throughput_32_qp, marker='x', label="32", color=color_32) #, width, label=treatment_label, color='tab:blue', hatch='\\')
ax.plot(threads,throughput_16_qp, marker='x', label="16", color=color_16) #, width, label=treatment_label, color='tab:blue', hatch='\\')
ax.plot(threads,throughput_8_qp, marker='x', label="8", color=color_8) #, width, label=treatment_label, color='tab:blue', hatch='\\')
ax.plot(threads,throughput_4_qp, marker='x', label="4", color=color_4) #, width, label=treatment_label, color='tab:blue', hatch='\\')
ax.plot(threads,throughput_2_qp, marker='x', label="2",color=color_2) #, width, label=treatment_label, color='tab:blue', hatch='\\')
ax.plot(threads,throughput_1_qp, marker='x', label="1",color=color_1) #, width, label=treatment_label, color='tab:blue', hatch='\\')

# Add some text for labels, title and custom x-axis tick labels, etc.
ax.set_ylabel('KOP/s')
ax.set_title('Memory Side Queue Pairs (id mapping) [burst=4]')
ticks = range(0,65)
ticks = [x for x in ticks if not x % 4]
ax.set_xticks(ticks)
ax.set_xlabel("Client Threads")
ax.legend()
fig.tight_layout()
save(plt)
#plt.show()