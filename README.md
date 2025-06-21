# lcs_diff_demo

## Demo LCS and diff implementations

I have written implementations of two Longest Common Subsequence algorithms for your study, use, and enjoyment. I have included a Python version of the Hunt/McIlroy algorithm and Python and C versions of the Hunt/Szymanski and Kuo/Cross algorithms, and my own modification of the Kuo/Cross algorithm.

These files are demos for the explanation of the algorithms found in my [blog post](https://www.raygard.net/2022/08/26/diff-LCS-Hunt-Szymanski-Kuo-Cross/).

You will also find a diff.py Python diff program that can use all the Python implementations and produce a unified diff (diff -u) output, and a diff.c C program that can produce unified diff with 3 line context (-u) by default or any other context (-U number_of_lines) including 0 lines (no context).

These are all under 0BSD license so you can use them as you wish. They are technically copyrighted but use them freely. The license does not require any attribution but if you do use them I would appreciate a mention in the code and/or documention etc.
