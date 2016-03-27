#!/usr/bin/python
# -*- coding: utf-8 -*-

import numpy as np

from sklearn.linear_model import Perceptron
from sklearn.metrics import accuracy_score
from sklearn.preprocessing import StandardScaler

m1 = np.loadtxt("perceptron_train.csv", delimiter=",")
X_train = m1[:, 1:3]
y_train = m1[:, :1]
y_train = np.squeeze(np.asarray(y_train))
print y_train
print X_train, ", len: ", len(X_train)

m2 = np.loadtxt("perceptron_test.csv", delimiter=",")
X_test = m2[:, 1:3]
y_test = m2[:, :1]
y_test = np.squeeze(np.asarray(y_test))
print "len of test: ", len(X_test)


score1 = 0
score2 = 0

if 1:
    clf = Perceptron(random_state = 241)
    clf.fit(X_train, y_train)
    predictions = clf.predict(X_test)

    score1 = accuracy_score(y_test, predictions)
    print "score: ", score1

if 1:
    scaler = StandardScaler()
    X_train_scaled = scaler.fit_transform(X_train)
    X_test_scaled = scaler.transform(X_test)

    clf2 = Perceptron(random_state = 241)
    clf2.fit(X_train_scaled, y_train)
    predictions = clf2.predict(X_test_scaled)

    score2 = accuracy_score(y_test, predictions)
    print "score: ", score2

print "diff: ", score2 - score1









