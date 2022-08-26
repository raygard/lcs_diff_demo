#! /usr/bin/env python3
# vim: set fileencoding=utf-8

"""diff.py -- basic diff -u using LCS algorithms

Usage:
    diff file1 file2
    or
    diff algo file1 file2
    where algo is one of hm hs kc kcmod

 * Copyright 2022 Ray Gardner (rdg, raygard)
 * License: 0BSD
"""

import sys
import os
import time

import hm   # Hunt-McIlroy
import hs   # Hunt-Szymanski
import kc   # Kuo-Cross
import kcmod    # Kuo-Cross modified


def fmt_time(mtime_ns):
    mtime, ns = mtime_ns // 1000000000, mtime_ns % 1000000000
    loctime = time.localtime(mtime)
    tz = time.strftime('%z',  loctime)
    return time.strftime('%Y-%m-%d %H:%M:%S', loctime) + '.%09d %s' % (ns, tz)


def frng(i, j):
    return '%s' % i if j == 1 else '%s,%s' % (i, j) # style used by gnu diff
    #return '%s,%s' % (i, j)     # alternate style in Posix diff spec


def at_change(lcs, k):
    return lcs[k-1][0] + 1 != lcs[k][0] or lcs[k-1][1] + 1 != lcs[k][1]


def find_next_change(lcs, k):
    """Return endflag, index of next change, count of common lines before it."""
    k0 = k
    k += 1
    while not at_change(lcs, k):
        k += 1
    return k == len(lcs) - 1, k, k - k0


def diff(fna, fnb, fa, fb, lcs_algo):
    a = fa.read().splitlines()  # a and b are 0-based lists (like arrays)
    b = fb.read().splitlines()
    # lcs is 1-based list of (i, j) matched pairs (a[i-1] == b[j-1])
    lcs = lcs_algo(a, b)
    #print(f'{len(lcs)=}')
    if len(lcs) == len(a) == len(b):
        return      # No difference found.
    # Must have at least one change.
    a_time, b_time = os.stat(fna).st_mtime_ns, os.stat(fnb).st_mtime_ns
    print(f'--- {fna}\t{fmt_time(a_time)}')
    print(f'+++ {fnb}\t{fmt_time(b_time)}')
    # Insert dummy matches to facilitate finding change at start or end of file
    lcs.insert(0, (0, 0))
    lcs.append((len(a)+1, len(b)+1))
    # Insert dummy change as sentinel for find_next_change
    BIG = 999999999
    lcs.append((BIG, BIG))
    nctx = 3    # Lines of context of common lines.
    # Find first change; ncommon will be meaningless but no matter.
    endflag, k, ncommon = find_next_change(lcs, 0)
    assert not endflag
    while not endflag:
        # k is at first change or change after ncommon > 2 * nctx
        # last will refer to last change before ncommon > 2 * nctx or
        #   last change in file at very end (before BIG sentinels).
        first = last = k   # first and last change in a range of changes
        endflag, k, ncommon = find_next_change(lcs, k)
        while not endflag and ncommon <= 2 * nctx:
            last = k
            endflag, k, ncommon = find_next_change(lcs, k)

        begin_lcs = max(first - 1 - nctx, 0)
        begin_a, begin_b = lcs[begin_lcs]
        begin_a += 1
        begin_b += 1
        limit_lcs = min(last + nctx, len(lcs) - 2)
        end_a, end_b = lcs[limit_lcs]
        cnt_a, cnt_b = end_a - begin_a, end_b - begin_b
        print('@@ -%s +%s @@' % (frng(begin_a, cnt_a), frng(begin_b, cnt_b)))

        kk = begin_lcs
        while kk < limit_lcs:
            kk += 1
            if at_change(lcs, kk):
                for n in range(lcs[kk-1][0] + 1, lcs[kk][0]): 
                    print(f'-{a[n-1]}')
                for n in range(lcs[kk-1][1] + 1, lcs[kk][1]): 
                    print(f'+{b[n-1]}')
            if kk < limit_lcs:
                print(f' {a[lcs[kk][0]-1]}')


def main():
    args = sys.argv[1:]

    lcs_algo = kcmod.LCS
    if 'hm' in args:
        lcs_algo = hm.LCS
    elif 'hs' in args:
        lcs_algo = hs.LCS
    elif 'kc' in args:
        lcs_algo = kc.LCS
    elif 'kcmod' in args:
        lcs_algo = kcmod.LCS
    for a in 'hm hs kc kcmod'.split():
        if a in args:
            args.remove(a)
    fna, fnb = args
    enc='utf8'
    with open(fna, encoding=enc) as fa:
        with open(fnb, encoding=enc) as fb:
            diff(fna, fnb, fa, fb, lcs_algo)


main()
