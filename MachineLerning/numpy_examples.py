#!/usr/bin/python
# -*- coding: utf-8 -*-

import numpy as np

print "hello"

# loc - мат. ожидание, scale - дисперсия, size - (строки, [столбцы], [...])
#X = np.random.normal(loc=1, scale=10, size=(1000, 50))
X = np.random.normal(loc=1, scale=10, size=(2, 4))

print X

# нормировка матрицы (вычесть из каждого столбца среднее и подеоить на дисперсию)
# axis = 0 - по столбам, 1 - по строкам. если не указывать, то по всей матрице
m = np.mean(X, axis=0)
std = np.std(X, axis=0)

print "mean: ", m
print "std: ", std

X_norm = ((X - m)  / std)
print "normirovannaya:\n", X_norm

sums = np.sum(X_norm, axis=1)
print "sums in rows: ", sums

print "positives: ", sums >= 1

index_positives = np.nonzero(sums >= 1)
print "indexes: ", index_positives


#===========================
print "=======================\n\n"

# eye - единичная матрица
A = np.eye(3)
B = np.eye(3)
print A
print B

AB = np.vstack((A, B))
print AB

