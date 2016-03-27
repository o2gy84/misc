#!/usr/bin/python
# -*- coding: utf-8 -*-

import numpy as np

from sklearn.neighbors import KNeighborsRegressor
from sklearn import cross_validation
from sklearn import preprocessing
from sklearn.datasets import load_boston

boston = load_boston()
print (boston.data.shape)

X = boston.data
X_scaled = preprocessing.scale(X)
#X_scaled = X

y = boston.target

print "X: ", X, ", len: ", len(X)
print "X_scaled: ", X_scaled, ", len: ", len(X_scaled)
print "y: ", y, ", len: ", len(y)


kf = cross_validation.KFold(len(X), n_folds=5, shuffle = True, random_state = 42)

max1 = -100
p_index1 = 1

max2 = -100
p_index2 = 1

for _p in np.linspace(1.0, 10.0, num=200):
    neigh = KNeighborsRegressor(n_neighbors = 5, weights='distance', p=_p, metric='minkowski')
    cross_score = cross_validation.cross_val_score(neigh, X_scaled, y, scoring='mean_squared_error', cv = kf)
    m = np.mean(cross_score)
    maximum = np.max(cross_score)
    if maximum > max1:
        max1 = maximum
        p_index1 = _p

    if m > max2:
        max2 = m
        p_index2 = _p

    print "p: ", _p, ", mean: ", m, ", max: ", maximum, ", cross_score: ", cross_score

# тут почему-то надо максимизировать результат! Т.е. берем ответ из p_index1 
print "max1: ", max1, ", p: ", p_index1

print "max2: ", max2, ", p: ", p_index2





