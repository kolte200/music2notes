import matplotlib.pyplot as plt
from math import *

x = []
y = []


for k in range(1, 100):
    k = k / 100 * 0.1
    x.append(k)
    d = -1
    s = 0
    n = 0
    while d < 0:
        s += k * (1 - d)
        d += s
        n += 1
    y.append(n)
    print("%f ; %f" % (k, (pi/(2*n))**2 / 2))
    # n = 1/sqrt(k)
    # k = (1/n)^2


"""
k = 0.001
d = -1
s = 0
n = 0
while d < 0:
    s += k * (1 - d)
    d += s
    n += 1
    x.append(n)
    y.append(s)
    x.append(n)
    y.append(0.5**(k/(n)))
"""

plt.scatter(x, y)
plt.show()
