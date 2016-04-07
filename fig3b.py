import matplotlib.pyplot as plt
import matplotlib
import pylab as pl
import numpy as np

x = [1, 4, 9, 16, 25]
y = [0.01, 0.35, 0.48, 0.61, 1]
plt.figure()
plt.errorbar(x, y, xerr=2, yerr=0.1, fmt='--o')
plt.grid()
matplotlib.rcParams.update({'font.size':18})
plt.ylabel('Normalized Reflection Energy')
plt.xlabel('Retro-reflector area (cm^2)')
plt.show()
