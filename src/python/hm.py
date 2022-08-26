#! /usr/bin/env python3
# vim: set fileencoding=utf-8

# hm.py

# Copyright 2022 Ray Gardner (rdg, raygard)
# License: 0BSD

# Compute longest common subsequence (LCS) using Hunt and McIlroy's method
#
# See "An algorithm for differential file comparison", J. W. Hunt and M. D.
# McIlroy, Bell Telephone Laboratories Computing Sciences Technical Report
# #41 (1976). https://www.cs.dartmouth.edu/~doug/diff.pdf (text edited from
# OCR, figures redrawn). Erratum: coauthor of referenced Szymanski paper is J.
# W. Hunt, not H. B. Hunt III.

# I have tried to follow the paper's Appendix, "Summary of the diff algorithm"
# as closely as possible here. This is intended to help anyone trying to
# understand the algorithm from the paper.

import sys
HAVE_BISECT_WITH_KEY_ARG = sys.version_info[:2] >= (3, 10)

if HAVE_BISECT_WITH_KEY_ARG:
    import bisect

#ncand = [0]

def LCS(A, B):
    #ncand[0] = 0
    m, n = len(A), len(B)
    #if m > n:
    #    A, B, m, n = B, A, n, m

    #1
    V = list(zip(range(1, n+1), B))
    if V and isinstance(V[0][1], str):
        V.insert(0, (0, ''))
    elif V and isinstance(V[0][1], int):
        V.insert(0, (0, 0))
    elif V:
        print('V:', V)
        print(type(V[0][1]))
        raise TypeError('Input lists must be of str or int.')
    #print(f'{V=}')

    #2
    V.sort(key=lambda t: (t[1], t[0]))
    #print(f'{V=}')

    #3
    E = [(V[j][0], j == n or V[j][1] != V[j+1][1]) for j in range(1, n+1)]
    E.insert(0, (0, True))
    #print(f'{E=}')

    #4
    P = [0] * (m+1)
    for i in range(1, m+1):
        #print(f'{i, A[i-1]=}')
        # P[i] = lowest j s.t. V[j][1] == A[i-1] and E[j-1][1] is True
        #   or 0 if no such j

        # Use bisect_left() only if Python version is >= 3.10
        #  as that is when the key= param was added.
        if HAVE_BISECT_WITH_KEY_ARG:
            j = bisect.bisect_left(V, A[i-1], 1, len(V), key=lambda x: x[1])
        else:
            lo, hi = 1, len(V)
            while lo < hi:
                mid = (lo + hi) // 2
                #if (i==5): print(f'{lo,hi,mid=} {V[mid]=}')
                if V[mid][1] < A[i-1]:
                    lo = mid + 1
                else:
                    hi = mid
            #if (i==5): print(f'{lo,hi,mid=} {V[lo]=}')
            j = lo

        assert E[j-1][1]
        if j < len(V) and V[j][1] == A[i-1]:
            P[i] = j
    #print(f'{P=}')

    #5
    K = [None] * (min(m, n) + 2)
    K[0], K[1] = [(0, 0, None), (m + 1, n + 1, None)]
    k = 0

    #6
    for i in range(1, m+1):
        if P[i] != 0:
            k = merge(K, k, i, E, P[i])

    #7
    J = [0] * (m+1)

    #8
    c = K[k]
    #####print(f'{k=} {ncand=}')
    while c is not None:
        J[c[0]] = c[1]
        c = c[2]

    # pairings are now {(i, J[i]) | J[i] != 0}

    #for i in range(0, m+1):
    #    if J[i]:
    #        print(i, J[i], A[i-1], B[J[i]-1])

    # For this application, return the matching pairs in LCS
    return [(i, J[i]) for i in range(m+1) if J[i]]

    # Here if returning actual LCS elements:
    # Omit step 9. It weeds out jackpots, where the LCS contains matches
    # between values that hashed the same but are not actually equal
    # due to hash collisions. Instead just return LCS elements from B.

    return [B[J[i]-1] for i in range(0, m+1) if J[i]]

def merge(K, k, i, E, p):
    #1
    r, c = 0, K[0]
    #2
    while True:
        #3
        j = E[p][0]
        # Find K[s] in K[r:k+1] such that K[s][1] < j < K[s+1][1]
        # This is same as:
        # Find s such that r <= s <= k and K[s][1] < j < K[s+1][1]

        # Use bisect_left() only if Python version is >= 3.10
        #  as that is when the key= param was added.
        if HAVE_BISECT_WITH_KEY_ARG:
            s = bisect.bisect_left(K, j, r, k+1, key=lambda x: x[1])
        else:
            lo, hi = r, k+1
            while lo < hi:
                mid = (lo + hi) // 2
                if K[mid][1] < j:
                    lo = mid + 1
                else:
                    hi = mid
            s = lo

        # Now all(n < j for n in K[r:s][1]) and
        #     all(j <= n for n in K[s:k+1][1])
        # Or informally, all K[r:s][1] < j <= K[s:k+1][1]
        #assert K[s-1][1] < j <= K[s][1]
        if s == r:
            # Search failed to find a valid place for j
            assert j <= K[s][1]
            pass
        else:
            # Adjust s so that K[s][1] < j <= K[s+1][1]
            s -= 1
            assert K[s][1] < j <= K[s+1][1]

        # Steps 4 and 5 call for doing assignments "simultaneously".
        # If I understand correctly, this means all right sides must be
        # evaluated before any values are stored via the lvalues.
        # For step 5, I see no problem in doing them in the order specified.
        # For step 4, just doing them in the order shown in the paper may
        # result in K[s] being overwritten by the first step (K[r] <- c).

        #4
        if K[s][1] < j < K[s+1][1]:
            # Need to hold K[s] because next line might clobber it.
            temp_Ksub_s = K[s]
            K[r] = c
            r = s + 1
            #c = (i, j, K[s])
            c = (i, j, temp_Ksub_s)
            #print(f'dmatch({i}, {j})')

            #5
            if s == k:
                #assert len(K) == k + 2
                #K.append(K[k+1])
                K[k + 2] = K[k + 1]
                k += 1
                break

        #6
        if E[p][1]:
            break
        p += 1

    #7
    K[r] = c
    return k
