#!/usr/bin/python
# -*- coding: utf-8 -*-

import numpy as np
import pandas as pd
import matplotlib 

def percent(size, total):
    return (size/float(total)) * 100;

Location = '/home/o2gy/downloads/_ea07570741a3ec966e284208f588e50e_titanic.csv'
#df = pd.read_csv(Location, index_col='PassengerId', header=0, names=['PassengerId','Survived','Pclass','Name','Sex','Age','SibSp','Parch','Ticket','Fare','Cabin','Embarked'])
df = pd.read_csv(Location, index_col='PassengerId', header=0)

print df.head(5)

print "\n\n========================"
print "count:\n", df.count()

print "\n\n========================"
print "value_counts:\n", df['Pclass'].value_counts()

print "\n\n========================"
print df['Sex'].value_counts()

print "\n\n========================"
grouped = df.groupby(['Survived'])
print grouped.size()

df2 = df['Survived']
print df2.count()
print df2.value_counts()

total = 0
survived = 0
for index, row in df.iterrows():
    total += 1
    if row['Survived'] == 1:
        survived += 1

print "total: ", total, ", survived: ", survived
print "percent of survived: ", percent(survived, total)

print "\n\n========================"
df2 = df['Pclass']
print df2.value_counts()

total = 0
first_class = 0
for index, row in df.iterrows():
    total += 1
    if row['Pclass'] == 1:
        first_class += 1

print "total: ", total, ", first_class: ", first_class 
print "percent of first_class passangers: ", percent(first_class, total)

print "\n\n========================"
df2 = df['Age']
print df2.describe()

print df2.median()
print df2.mean()

print df.mean()
print df.mean(axis=0, numeric_only=True)   # by cols

print "\n\n========================"
print "pearson correlation:\n"
print df.corr()

print "\n\n========================"

womens = df['Sex'] == 'female'
#print df[womens]

total = 0
names = {'a': 1}

import re

for index, row in df[womens].iterrows():
    total += 1
    name = row['Name']
    name_regex = re.match(r".*(Mrs\.|Miss\.|Ms\.|Mme\.|Mlle\.)(.*)", name)
    if name_regex:
        #pass
        stripped = name_regex.group(2).strip()
        
        test2 = re.match(r".*\((.+)\)", name)
        if test2:
            stripped = test2.group(1)
        
        parts = stripped.split(" ")
        if len(parts) > 1:
            stripped = parts[0]


        #print stripped
        if stripped in names:
            names[stripped] += 1
        else:
            names[stripped] = 1
    else : 
        pass
        #print name

import operator
sorted_names = sorted(names.items(), key=operator.itemgetter(1))    # by val
#sorted_names = sorted(names.items(), key=operator.itemgetter(0))   # by key

for i in sorted_names:
    print i





