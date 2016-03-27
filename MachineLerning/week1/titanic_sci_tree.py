#!/usr/bin/python
# -*- coding: utf-8 -*-

import numpy as np
import pandas as pd
import matplotlib 
from sklearn.tree import DecisionTreeClassifier

def percent(size, total):
    return (size/float(total)) * 100;

def sex_to_int(sex):
    if sex == 'male':
        return 1
    return 0

Location = '/home/o2gy/downloads/_ea07570741a3ec966e284208f588e50e_titanic.csv'
#df = pd.read_csv(Location, index_col='PassengerId', header=0, names=['PassengerId','Survived','Pclass','Name','Sex','Age','SibSp','Parch','Ticket','Fare','Cabin','Embarked'])
df_orig = pd.read_csv(Location, index_col='PassengerId', header=0)

df = df_orig[['Survived', 'Pclass', 'Fare', 'Age', 'Sex']]

#data = data[np.isfinite(data['Age'])]
df = df.dropna(axis = 0)

total = 0
nans = 0
for index, row in df.iterrows():
    total += 1
    #print row['Age']
    if pd.isnull(row['Age']):
        nans += 1
        print "\t\tNAN!"

print "total: ", total, ", nans: ", nans

#data=data.replace(to_replace=['male', 'female'], value=[0, 1])
df['Sex'] = df['Sex'].apply(sex_to_int)

print "\n\n========================"

y = df['Survived'].values
#y = np.array([0, 1, 0])
print y, "len: ", len(y)

X = df[['Fare', 'Sex', 'Age', 'Pclass']].values
#X = np.array([[1, 2], [3, 4], [5, 6]])
print X, "len: ", len(X)

clf = DecisionTreeClassifier(random_state=241)
print clf

clf.fit(X, y)

importances = clf.feature_importances_
print importances

