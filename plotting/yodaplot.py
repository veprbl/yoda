#! /usr/bin/env python


################
## Data reading:

import yoda, sys
refaos = yoda.read(sys.argv[1])
cmpaos = yoda.read(sys.argv[2])


#################
## Data handling:

import numpy as np

def mk_dataarray(s):
    data = np.zeros(s.numPoints, dtype={'names':['x', 'y', 'exminus', 'explus', 'eyminus', 'eyplus'],
                                              'formats':['f4', 'f4', 'f4', 'f4', 'f4', 'f4']})
    for i, p in enumerate(s.points):
        data["x"][i] = p.x
        data["y"][i] = p.y
        data["exminus"][i] = p.xErrs[0]
        data["explus"][i]  = p.xErrs[1]
        data["eyminus"][i] = p.yErrs[0]
        data["eyplus"][i]  = p.yErrs[1]
    return data

hname = sorted(cmpaos.keys())[0]
sref = mk_dataarray(refaos["/REF"+hname].mkScatter())
scmp = mk_dataarray(cmpaos[hname].mkScatter())
#print "Data:\n" + str(sref)

## Check that x bins are compatible and compute min and max x extents of each data point
assert (scmp["x"] == sref["x"]).all()
assert (scmp["exminus"] == sref["exminus"]).all()
assert (scmp["explus"] == sref["explus"]).all()
xmins, xmaxs = sref["x"]-sref["exminus"], sref["x"]+sref["explus"]
xmin, xmax = min(xmins), max(xmaxs)
xdiff = xmax-xmin
## Get the y min and max extents of each data point (over all of the ref and cmp datasets)
ref_ymins, ref_ymaxs = sref["y"]-sref["eyminus"], sref["y"]+sref["eyplus"]
ymin, ymax = None, None
for s in (sref, scmp):
    s_ymin, s_ymax = min(s["y"]-s["eyminus"]), max(s["y"]+s["eyplus"])
    ymin = min(ymin, s_ymin) if ymin else s_ymin
    ymax = min(ymax, s_ymax) if ymax else s_ymax
ydiff = ymax - ymin


############
## Plotting:

import matplotlib as mpl
mpl.rcParams.update({
    "text.usetex" : True,
    "font.size"   : 17,
    "font.family" : "serif",
    "font.serif"         : ["Palatino", "Computer Modern Roman"] + mpl.rcParams["font.serif"],
    #"font.sans-serif"    : ["Computer Modern Sans serif", "Helvetica"]
    #"font.cursive"       : "Zapf Chancery",
    #"font.monospace"     : "Courier, Computer Modern Typewriter",
    "pgf.rcfonts" : True,
    "pgf.preamble": [r"\usepackage[osf]{mathpazo}", r"\usepackage{amsmath,amssymb}"]
    # r"\usepackage{lmodern}",
    # r"\usepackage{sfmath}", ",]
    # r"\usepackage{siunitx},"]
    })
#mpl.use("pgf")
from matplotlib import pyplot as plt

## Make subplot grid layout
# fig, (axmain, axratio) = plt.subplots(nrows=2, ncols=1, sharex=True, squeeze=True)
fig = plt.figure(figsize=(8, 6))
gs = mpl.gridspec.GridSpec(2, 1, height_ratios=[3,1], hspace=0)

## Main plot
# TODO: take log axes and preference for round numbers into account in setting default axis limits
axmain = fig.add_subplot(gs[0])
axmain.set_ylabel("FOO $\in y$")
#axmain.xaxis.set_major_locator(mpl.ticker.NullLocator())
plt.setp(axmain.get_xticklabels(), visible=False)
axmain.set_ylim([ymin-0.1*ydiff, ymax+0.1*ydiff])
axmain.errorbar(sref["x"], sref["y"],
                xerr=sref["exminus"], yerr=sref["eyminus"],
                fmt="ko", linewidth=1.3, capthick=1.3)
axmain.step(np.append(xmins, [xmax]), np.append(scmp["y"], scmp["y"][-1]), where="post", color="r", linewidth=1.3)
#  xerr=sref["exminus"], yerr=sref["eyminus"], capthick=1.3)

## Ratio plot
axratio = fig.add_subplot(gs[1], sharex=axmain)
axratio.set_xlabel("BAR $\in x$")
axratio.set_ylabel("MC / Data")
# TODO: Would be nice to add a display space fixed padding rather than prop to xdiff
axratio.set_xlim([xmin-0.001*xdiff, xmax+0.001*xdiff])
# Draw fill_between ref error band
# Continuous:
# ref_ymax_ratios = ref_ymaxs/sref["y"]
# ref_ymin_ratios = ref_ymins/sref["y"]
# axratio.fill_between(sref["x"], ref_ymin_ratios, ref_ymax_ratios, edgecolor="none", facecolor='yellow', interpolate=False)
# Stepped:
ref_ymin_ratios = ref_ymins/sref["y"]
ref_ymax_ratios = ref_ymaxs/sref["y"]
ref_ymin_ratios_dbl, ref_ymax_ratios_dbl = [], []
for yminratio, ymaxratio in zip(ref_ymin_ratios, ref_ymax_ratios):
    ref_ymin_ratios_dbl += [yminratio, yminratio]
    ref_ymax_ratios_dbl += [ymaxratio, ymaxratio]
xs_dbl = np.empty((len(ref_ymin_ratios_dbl),), dtype=ref_ymin_ratios.dtype)
xs_dbl[0::2] = xmins
xs_dbl[1::2] = xmaxs
ref_ymax_ratio = ref_ymaxs/sref["y"]
ref_ymin_ratio = ref_ymins/sref["y"]
axratio.fill_between(xs_dbl, ref_ymin_ratios_dbl, ref_ymax_ratios_dbl, edgecolor="none", facecolor='yellow', interpolate=False)
# Ratio = 1 marker line:
axratio.axhline(1.0, color="gray")
yratios = scmp["y"]/sref["y"]
#axratio.plot(sref["x"], yratios, color="r", linestyle="--")
axratio.step(np.append(xmins, [xmax]), np.append(yratios, yratios[-1]), where="post", color="r", linewidth=1.3)
#axratio.set_ylim(0.5, 1.5)
axratio.yaxis.set_major_locator(mpl.ticker.MaxNLocator(4, prune="upper"))

plt.tight_layout()

fig.savefig("yodaplot.pdf")
#fig.savefig("yodaplot.pgf")
