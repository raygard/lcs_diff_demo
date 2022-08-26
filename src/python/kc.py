#! /usr/bin/env python3
# vim: set fileencoding=utf-8

# kc.py

# Copyright 2022 Ray Gardner (rdg, raygard)
# License: 0BSD

# Compute longest common subsequence (LCS) using Kuo and Cross's modification
# of Hunt and Szymanski's method
#
# See "A Fast Algorithm for Computing Longest Common Subsequences",
# J.W. Hunt and T.G. Szymanski, CACM vol 20 no. 5 (May 1977), p350-353
# and "An Improved Algorithm to Find the Length of the Longest Common
# Subsequence of Two Strings", S. Kuo and G.R. Cross, ACM SIGIR Forum,
# vol 23, issue 3-4, Spring 1989, p89-99.

def LCS(A, B):
    m, n = len(A), len(B)

    # Step 1: build linked lists
    matchlist = [[] for k in range(m + 1)]
    aa = sorted(zip(A, range(1, m+1)))
    bb = sorted(zip(B, range(1, n+1)))
    ai = bi = 0
    while ai < m and bi < n:
        av, bv = aa[ai][0], bb[bi][0]
        if av < bv:
            ai += 1
        elif av > bv:
            bi += 1
        else:
            k = aa[ai][1]
            while bi < n and bb[bi][0] == bv:
                matchlist[k] += [bb[bi][1]]
                bi += 1
            ai += 1
            while ai < m and aa[ai][0] == av:
                matchlist[aa[ai][1]] = matchlist[k]
                ai += 1

    # Step 2: initialize the THRESH array
    thresh = [n+1] * (m + 1)
    thresh[0] = 0

    # Step 3: compute successive THRESH values
    link = [None] * (m+1)
    for i in range(1, m+1):
        k = 0
        temp = 0
        r = 0
        c = link[0]
        for j in matchlist[i]:
            if j > temp:
                k += 1;
                while j > thresh[k]:
                    k += 1
                #assert thresh[k-1] < j <= thresh[k]
                temp = thresh[k]
                if j < temp:
                    thresh[k] = j
                    prev_c = link[k-1]
                    link[r] = c
                    r = k
                    c = (i, j, prev_c)
                    #print(f'dmatch({i}, {j})')
                    #assert A[i-1] == B[j-1]
        link[r] = c

    # Step 4: recover longest common subsequence pairs in reverse order
    k = 0
    while k < m and thresh[k+1] != n + 1:
        k += 1
    p = link[k]
    # v will hold (i,j) pairs
    v = []
    while p != None:
        v.append(p[:2])
        p = p[2]
    v.reverse()

    #print(f'lcslen: {len(v)=}')
    return v

    #print(v)
    # Return actual LCS
    # Do this if actually recovering the LCS itself.
    for k in range(len(v)):
        v[k] = A[v[k][0]-1]
    return v
