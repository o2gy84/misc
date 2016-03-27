#!/usr/bin/python
# -*- coding: utf-8 -*-

import numpy as np

from sklearn.neighbors import KNeighborsClassifier
from sklearn import cross_validation
from sklearn import preprocessing

y = np.loadtxt("wine.data", delimiter=',', usecols=(0,))
X = np.loadtxt("wine.data", delimiter=',', usecols=(1,2,3,4,5,6,7,8,9,10,11,12,13))

print y, "len: ", len(y)
print X, "len: ", len(X)

kf = cross_validation.KFold(len(X), n_folds=5, shuffle = True, random_state = 42)
for train_index, test_index in kf:
    print("train: %s\ntest: %s\n" % (train_index, test_index))
    X_train, X_test = X[train_index], X[test_index]
    y_train, y_test = y[train_index], y[test_index]
    #print "X_test: ", X_test
    #print "y_test: ", y_test

    k = 10
    neigh = KNeighborsClassifier(n_neighbors = k)
    neigh.fit(X_train, y_train) 
    score = neigh.score(X_test, y_test)
    print "score: ", score

# то же самое, но автоматически
print "\n"
k = 10
neigh = KNeighborsClassifier(n_neighbors = k)
cross_score = cross_validation.cross_val_score(neigh, X, y, cv = kf)
print "cross_score [ceiled]: ", cross_score
cross_scaled = preprocessing.scale(cross_score)
print "scaled: ", cross_scaled

m = np.mean(cross_score)
std = np.std(cross_score)
print "scaled2: ", (cross_score - m)/std
print "mean (k = 10): ", m


for k in range(1, 51):
    neigh = KNeighborsClassifier(n_neighbors = k)
    cross_score = cross_validation.cross_val_score(neigh, X, y, cv = kf)
    m = np.mean(cross_score)
    print "k: ", k, ", mean accuracy: ", m


print "and now lets scaled !!!"

# А теперь отмасштабируем признаки, результат получится гораздо круче!
X_scaled = preprocessing.scale(X)
for k in range(1, 51):
    neigh = KNeighborsClassifier(n_neighbors = k)
    cross_score = cross_validation.cross_val_score(neigh, X_scaled, y, cv = kf)
    m = np.mean(cross_score)
    print "k: ", k, ", mean accuracy: ", m





