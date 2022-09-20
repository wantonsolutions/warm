import matplotlib
import matplotlib.pyplot as plt
import numpy as np

def div_mil(a):
    return[x/1000000.0 for x in a]

def mul4(a):
    return[x*4 for x in a]

def rate_per_host(a,hosts):
    return[x/hosts for x in a]

#labels = ['1', '2', '3', '4', '5', '6', '7', '8', '9', '10' ]
#memory_qp=[18261484.86,37054262.29,56117045.81,74918129.99,93975076.02,112654762.33,117922784.98,117935091.21,117934017.01,117935898.95]


# keys = ["0", "1", "2", "4", "8", "16", "32", "64", "128", "256"]
# tput = [1205508,2191464,3454732, 3774092, 5937416, 6635000, 5516680, 9283378, 27555979, 27698208]

#manual collection
keys = ["1", "2", "5", "10", "20", "30", "40", "50", "60", "70",  "85", "100", "115",   "116",    "117",    "118",    "119",    "120",    "121",  "122",    "123",     "124",    "125",    "126",    "127"]#    "128"]
tput = [1573912, 2671500, 4498460, 5787036, 6249868, 6767805, 7062688, 7183620, 7436236, 7807648, 8111682, 10758896, 13513847, 14197721, 14151919, 14138816, 14299024, 15719324, 16601440, 16463966, 17311364, 18634872, 19739024, 20775460, 22269488]# 22868052]

#6 clients automatic steps of 8
keys = [str(x) for x in range(0,129,8)]
tput = [1270096,5178760,6589156,6442008,7164503,7068398,7317628,7390316,7417012,8776282,9094960,10290336,10855020,11575400,12740592,15744816,23060168,]

#1 client 56 threads
tput=[1132632,
2225596,2547920,2775932,2926522,
3359448,3424720,3804120,3739152,
4075284,4120403,4160655,4267279,
4312780,4300516,4305516,4317508,]

#4 clients 224 threads
tput=[1374760,
4083920,4320404,4766196,5459044,
4838504,5676648,3648152,6171944,
6850984,7470187,7743216,8403076,
9472296,9994812,8092124,16793652,]

#3 clients 168 threads
tput=[1350680,
3368428,3760071,4011214,4264014,
4366896,3280580,5079104,4863412,
4631832,4982792,6335912,6545865,
7408180,8075524,9815620,10351040,]

terr=tput

#three hosts with error three runs each zipf 1.0
tput=[324266, 843707, 774661, 917288, 1051689,1120983,1195665,1012591,1288367,1298131,1345462,1458922,1610240,1567769,1693890,2387891,3172379,]
terr=[69400,340114,592037,168189,122904,295540,38658,696423,136223,423032,1101432,535445,403394,161205,854131,481505,295704,]
terr=mul4(terr)
tput=mul4(tput)


# #three hosts zipf
# tput=[505900, 3765744,5041312,5969245,6702036,6537864,6630964,5944280,
# 7028592,7772459,7954856,7988448,8157231,8215416,7863136,8393588,
# 8192459,]
# terr=tput
# terr=[ 0 for x in tput]

# #6clients zipf 1.5
# tput=[432840,                                                         
# 3206623,6578585,7963668,9730236,
# 11475908,12108912,13020835,11437780,
# 12111672,12228324,14574452,13434751,
# 15639283,16092184,16213243,15903284,]



#six hosts zipf 1.5 3 trials
tput=[126264, 
1200764,1614114,1981764,2376874,
2849514,3117038,3330861,3431735,
3506670,3630810,3722979,3915607,
3914276,3958349,3989503,3970129,]
#
#8,16,24,32
#40,48,56,64
#72,80,88,96,
#104

terr=[26694,  
11221,69906,17897,104278,
71718,23941,120434, 116300, 
2005, 22281,102302, 108484,
79231,98445,165432, 146235,]
terr=mul4(terr)
tput=mul4(tput)

tput=rate_per_host(tput,6.0)
terr=rate_per_host(terr,6.0)




print(len(keys))
print(len(tput))



read_write_steering_color='#cf243cff'     #alice



tput=div_mil(tput)
terr=div_mil(terr)
treatment_label= 'queue pairs'


x = np.arange(len(keys))  # the label locations

#todo remove this 
#keys = [int(x) for x in keys]
# m=max(tput)
# tput=[m-x for x in tput]
# m=max(keys)
# x=[m-x for x in keys]
x=keys[:len(tput)]
#todo end remove

width = 0.7  # the width of the bars

plt.rcParams.update({'font.size': 12})
#fig, ax = plt.subplots()
fig, ax = plt.subplots(1,1, figsize=(8,4))
rects1 = ax.errorbar(x, tput, yerr=terr, color=read_write_steering_color) #, width, label=treatment_label, color='tab:blue', hatch='\\')

# Add some text for labels, title and custom x-axis tick labels, etc.
ax.set_ylabel('MOPS/ Host (56 threads)')
ax.set_xticks(x)
ax.set_xticklabels(keys[:len(tput)])
ax.set_xlabel("Keys")

fig.tight_layout()
plt.savefig("keys_tracked.pdf")
plt.savefig("keys_tracked.png")