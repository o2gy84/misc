#!/usr/bin/python
# -*- coding: utf-8 -*-

import numpy as np
from sklearn.svm import SVC
from sklearn import datasets

newsgroups = datasets.fetch_20newsgroups(subset='all', categories=['alt.atheism', 'sci.space'])

#print newsgroups.data
#print newsgroups.target


#m1 = np.loadtxt("svm_data.csv", delimiter=",")
#X_train = m1[:, 1:3]
#y_train = m1[:, :1]
#y_train = np.squeeze(np.asarray(y_train))
#print y_train
#print X_train, ", len: ", len(X_train)


#if 0:
    #clf = SVC(random_state = 241, C = 100000, kernel = 'linear')
    #clf.fit(X_train, y_train)
    #print "support: ", clf.support_
    #print "support_vectors:\n", clf.support_vectors_









