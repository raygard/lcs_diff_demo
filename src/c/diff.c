/* diff.c -- show differences between two files
 * Copyright 2022-2025 Ray Gardner (rdg, raygard)
 * License: 0BSD
 * unified diff using LCS method
 * demo LCS algos of Hunt/Szymanski and Kuo/Cross
 */

// Compile with one of these defined:
//#define HS
//#define KC
//#define KCMOD
#ifndef HS
#ifndef KC
#ifndef KCMOD
#define KCMOD   // default is my (rdg) mod to KC
#endif
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <assert.h>
#include <time.h>
#include <sys/stat.h>


#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

void errif(int cond, char *fmt,...)
{
    va_list argptr;
    if (cond) {
      va_start(argptr, fmt);
      vfprintf(stderr, fmt, argptr);
      va_end(argptr);
      printf("\n");
      exit(2);
    }
}

void *xrealloc(void *p, size_t n)
{
  p = realloc(p, n);
  errif(!p, "can't alloc %ld", (long)n);
  return p;
}

void *xmalloc(size_t n)
{
  return xrealloc(0, n);
}

void *xzalloc(size_t n)
{
  void *p = xrealloc(0, n);
  memset(p, 0, n);
  return p;
}

char *xstrdup(char *s)
{
  char *p = xmalloc(strlen(s) + 1);
  return strcpy(p, s);
}

struct fdata {
  char **lines;
  int nlines;
};

struct lcs {
  int len;
  int *a;
  int *b;
};

struct line_num_ptr {
  char *line;
  int lnum;
};

int cmpline(char *a, char *b)
{
  // Stand-in for compare with options -i -b -w etc.
  return strcmp(a, b);
}

// readline() -- like getline() but not quite.
// assumes *buf is null or points to memory allocated by
// malloc/calloc/realloc, and *bufsize is length of buffer.
// readline() (re-)allocates buffer as needed;
// reads newline-terminated lines but handles last line unterminated;
// handles embedded nul (zero) bytes;
// returns length of line including newline if any;
// returns -1 at EOF; returns -2 on read error.
intmax_t readline(char **buf, size_t *bufsize, FILE *fp)
{
  char *b, *e, *p;
#define init_size 128   // must be > 1
  if (! *buf || *bufsize < 2) *buf = xrealloc(*buf, *bufsize = init_size);
  b = *buf;
  for (;;) {
    size_t n = *buf + *bufsize - b;
    memset(b, '\xff', n);
    *b = 0;   // ensure 0 byte in buffer if no data is read
    p = fgets(b, n, fp);
    if (!p && ferror(fp)) return -2;
    e = memchr(b, '\n', n);
    if (e) return e - *buf + 1;
    // No newline
    if (feof(fp)) {
      if (!p && b == *buf) return -1;
      p = *buf + *bufsize - 1; // last byte in buffer
      while (*p && p > *buf) p--;
      assert(!*p);    // must have a 0 byte
      return p - *buf;
    }
    // expand buffer and read more
    n = *bufsize * 5 / 3;
    *buf = xrealloc(*buf, n);
    b = *buf + *bufsize - 1;
    *bufsize = n;
  }
}

struct fdata read_fdata(char *fn)
{
  struct fdata fd;
  FILE *fp = strcmp(fn, "-") ? fopen(fn, "r") : stdin;
  errif(!fp, "can't open %s", fn);
  intmax_t linesmax = 100;
  fd.lines = xzalloc(linesmax * sizeof(*fd.lines));
  fd.nlines = 0;
  char *line = NULL;
  intmax_t k;
  size_t linesz = 0;
  k = readline(&line, &linesz, fp);
  while (k > 0) {
    if (fd.nlines == linesmax) {
      linesmax = linesmax * 5 / 3;
      fd.lines = xrealloc(fd.lines, linesmax * sizeof(*fd.lines));
    }
    fd.lines[fd.nlines++] = xstrdup(line);
    k = readline(&line, &linesz, fp);
    errif(k > 0 && (size_t)k != strlen(line), "null char in data?");
  }
  errif(k == -2, "read error on %s", fn);
  assert(k == -1);
  free(line);
  if (strcmp(fn, "-"))
    fclose(fp);
  return fd;
}

void free_fdata(struct fdata fd)
{
  for (int k = 0; k < fd.nlines; k++)
    free(fd.lines[k]);
  free(fd.lines);
}

int cmp_line_num_ptr(const void *a, const void *b)
{
  struct line_num_ptr *aa = (struct line_num_ptr *)a;
  struct line_num_ptr *bb = (struct line_num_ptr *)b;
  int k = cmpline(aa->line, bb->line);
  if (k < 0)
    return -1;
  else if (k > 0)
    return 1;
#ifdef HS
  // Hunt/Szymanski method needs matchlists in descending order.
  // Sort ascending here; they will reverse when built.
  return aa->lnum < bb->lnum ? -1 : 1;
#else
  // Kuo/Cross method needs matchlists in ascending order.
  // Sort descending here; they will reverse when built.
  return aa->lnum < bb->lnum ? 1 : -1;
#endif
}

struct line_num_ptr *get_sorted_line_num_ptrs(struct fdata a)
{
  struct line_num_ptr *aa = xzalloc((a.nlines + 1) * sizeof(*aa));
  for (int k = 0; k < a.nlines; k++) {
    aa[k].line = a.lines[k], aa[k].lnum = k + 1;
  }
  qsort(aa, a.nlines, sizeof(struct line_num_ptr), cmp_line_num_ptr);
  return aa;
}

struct lcs getlcs(struct fdata a, struct fdata b)
{
  // Compute longest common subsequence (LCS) using Hunt and Szymanski's method
  // or Kuo and Cross's modification of Hunt and Szymanski's method
  // or my (rdg) modification of Kuo-Cross method
  //
  // See "A Fast Algorithm for Computing Longest Common Subsequences",
  // J.W. Hunt and T.G. Szymanski, CACM vol 20 no. 5 (May 1977), p350-353
  // and "An Improved Algorithm to Find the Length of the Longest Common
  // Subsequence of Two Strings", S. Kuo and G.R. Cross, ACM SIGIR Forum,
  // vol 23, issue 3-4, Spring 1989, p89-99.
  struct lcs lcs;
  lcs.len = 0;
  // Step 1: build linked lists of matches.
  struct line_num_ptr *aa = get_sorted_line_num_ptrs(a);
  struct line_num_ptr *bb = get_sorted_line_num_ptrs(b);
  int *matchlist_a = xzalloc((a.nlines + 1) * sizeof(*matchlist_a));
  int *matchlist_b = xzalloc((b.nlines + 1) * sizeof(*matchlist_b));
  int ai = 0, bi = 0;
  while (ai < a.nlines && bi < b.nlines) {
    int k = cmpline(aa[ai].line, bb[bi].line);
    if (k < 0) ai++;
    else if (k > 0) bi++;
    else {
      k = aa[ai].lnum;
      while (bi < b.nlines && cmpline(bb[bi].line, aa[ai].line) == 0) {
        matchlist_b[bb[bi].lnum] = matchlist_a[k];
        matchlist_a[k] = bb[bi].lnum;
        bi++;
      }
      int ai0 = ai;
      ai++;
      while (ai < a.nlines && cmpline(aa[ai].line, aa[ai0].line) == 0)
        matchlist_a[aa[ai++].lnum] = matchlist_a[k];
    }
  }
  free(aa);
  free(bb);

  // Step 2: initialize the THRESH array.
  // Add an extra element to simplify Step 4 condition.
  int *thresh = xzalloc((a.nlines + 1 + 1) * sizeof(*thresh));
  for (int i = 1; i <= a.nlines + 1; i++)
    thresh[i] = b.nlines + 1;

  struct dmatch {
    int i, j;
    struct dmatch *prev_dmatch;
    struct dmatch *prev_alloc;
  };

  struct dmatch *dmatch_alloc_list = NULL;
  struct dmatch **link = xzalloc((a.nlines + 1) * sizeof(*link));
  link[0] = NULL;

  // Step 3: compute successive THRESH values.
#ifdef HS
  for (int i = 1; i <= a.nlines; i++) {
    for (int j = matchlist_a[i]; j; j = matchlist_b[j]) {
      int k = 0, hi = a.nlines + 1;
      while (k < hi) {
        int mid = (k + hi) / 2;
        if (thresh[mid] < j)
          k = mid + 1;
        else
          hi = mid;
      }
      assert(thresh[k-1] < j);
      assert(j <= thresh[k]);
      if (j < thresh[k]) {
        thresh[k] = j;
        struct dmatch *p = link[k] = xmalloc(sizeof(struct dmatch));
        p->prev_alloc = dmatch_alloc_list;
        dmatch_alloc_list = p;
        p->i = i, p->j = j, p->prev_dmatch = link[k-1];
        //printf("dmatch(%d, %d)\n", i, j);
      }
    }
  }
#else
  for (int i = 1; i <= a.nlines; i++) {
    int k = 0, temp = 0, r = 0;
    struct dmatch *c = link[0];

    for (int j = matchlist_a[i]; j; j = matchlist_b[j]) {
      if (j > temp) {
#ifdef KC
        // Original Kuo-Cross method.
        do k++; while (j > thresh[k]);
#endif
#ifdef KCMOD
        // Kuo-Cross method with my (rdg) mod.
        int hi = a.nlines + 1;
        while (k < hi) {
          int mid = (k + hi) / 2;
          if (thresh[mid] < j)
            k = mid + 1;
          else
            hi = mid;
        }
#endif
        assert(thresh[k-1] < j);
        assert(j <= thresh[k]);
        temp = thresh[k];
        if (j < temp) {
          thresh[k] = j;
          struct dmatch *prev = link[k-1];
          link[r] = c;
          r = k;
          struct dmatch *p = c = xmalloc(sizeof(struct dmatch));
          p->prev_alloc = dmatch_alloc_list;
          dmatch_alloc_list = p;
          p->i = i, p->j = j, p->prev_dmatch = prev;
          //printf("dmatch(%d, %d)\n", i, j);
        }
      }
    }
    link[r] = c;
  }
#endif
  free(matchlist_a);
  free(matchlist_b);

  // Step 4: recover longest common subsequence in reverse order.
  int k = 0;
  while (thresh[k+1] != b.nlines + 1) k++;
  lcs.len = k;
  free(thresh);

  // Put (0, 0) ahead of matches, (a.nlines+1, b.nlines+1) after last match
  // to facilitate finding first and last change.
  // Put (999999999, 999999999) at end as sentinel for find_next_change().
  lcs.a = xzalloc((k + 3) * sizeof(*lcs.a));
  lcs.b = xzalloc((k + 3) * sizeof(*lcs.b));
  lcs.a[k+1] = a.nlines + 1;
  lcs.b[k+1] = b.nlines + 1;
  lcs.a[k+2] = 999999999;
  lcs.b[k+2] = 999999999;

  struct dmatch *p = link[k];
  while (p) {
    //printf("match(%d, %d)\n", p->i, p->j);
    lcs.a[k] = p->i;
    lcs.b[k--] = p->j;
    p = p->prev_dmatch;
  }
  free(link);
  while (dmatch_alloc_list) {
    struct dmatch *p = dmatch_alloc_list->prev_alloc;
    free(dmatch_alloc_list);
    dmatch_alloc_list = p;
  }
  return lcs;
}

int at_change(struct lcs *lcs, int k)
{
  return lcs->a[k-1] + 1 != lcs->a[k] || lcs->b[k-1] + 1 != lcs->b[k];
}

int find_next_change(struct lcs *lcs, int k, int *newk, int *ncommon)
{
  int k0 = k++;
  while (!at_change(lcs, k))
    k++;
  *newk = k;
  *ncommon = k - k0;
  return k == lcs->len + 2;
}

// Formats time_t in ISO format ("2018-06-28 15:08:58.846386216 -0700").
char *fmt_iso_time(char *buf, size_t len, time_t *mtime)
{
  char *s = buf;
  s += strftime(s, len, "%Y-%m-%d %H:%M:%S", localtime(mtime));
  s += strftime(s, len, " %z", localtime(mtime));
  return buf;
}

void printhdr(char *prefix, char *fn)
{
  char tm[64];
  struct stat statbuf;
  time_t mtim;
  if (strcmp(fn, "-")) {
    int r = stat(fn, &statbuf);
    errif(r, "can't stat %s", fn);
    mtim = statbuf.st_mtime;
  } else {
    mtim = time(NULL);
  }
  printf("%s %s\t%s\n", prefix, fn, fmt_iso_time(tm, sizeof(tm), &mtim));
}

void print_diff(char *fn1, char *fn2, struct fdata a, struct fdata b,
    struct lcs lcs, int nctx)
{
  printhdr("---", fn1);
  printhdr("+++", fn2);
  int k, ncommon;
  // Find first change; ncommon will be meaningless but no matter.
  int endflag = find_next_change(&lcs, 0, &k, &ncommon);
  assert(!endflag);
  while (!endflag) {
    // k is at first change or change after ncommon > 2 * nctx.
    // last will refer to last change before ncommon > 2 * nctx or
    //    last change in file at very end (before 999999999 sentinels).
    int first = k, last = k;    // first and last in group of changes
    endflag = find_next_change(&lcs, k, &k, &ncommon);
    while (!endflag && ncommon <= 2 * nctx) {
      last = k;
      endflag = find_next_change(&lcs, k, &k, &ncommon);
    }
    int begin_lcs = max(first - 1 - nctx, 0);
    int begin_a = lcs.a[begin_lcs] + 1;
    int begin_b = lcs.b[begin_lcs] + 1;
    int limit_lcs = min(last + nctx, lcs.len + 1);
    int cnt_a = lcs.a[limit_lcs] - begin_a;
    int cnt_b = lcs.b[limit_lcs] - begin_b;

    // Adjust begin line numbers if count is zero, to match gnu diff -U 0
    // Not sure it's incorrect without this, but patch doesn't work without it.
    if (!cnt_a)
      begin_a--;
    if (!cnt_b)
      begin_b--;

    printf("@@ -%d", begin_a);
    if (cnt_a != 1)
      printf(",%d", cnt_a);
    printf(" +%d", begin_b);
    if (cnt_b != 1)
      printf(",%d", cnt_b);
    printf(" @@\n");
    int kk = begin_lcs;
    while (kk < limit_lcs) {
      kk++;
      if (at_change(&lcs, kk)) {
        for (int n = lcs.a[kk-1] + 1; n < lcs.a[kk]; n++)
          printf("-%s", a.lines[n-1]);
        for (int n = lcs.b[kk-1] + 1; n < lcs.b[kk]; n++)
          printf("+%s", b.lines[n-1]);
      }
      if (kk < limit_lcs)
        printf(" %s", a.lines[lcs.a[kk]-1]);
    }
  }
}

void diff(char *fn1, char *fn2, int nctx)
{
  //printf("Diff: %s %s\n", fn1, fn2);
  struct fdata a = read_fdata(fn1);
  struct fdata b = read_fdata(fn2);
  struct lcs lcs = getlcs(a, b);
  if (lcs.len != a.nlines || lcs.len != b.nlines)
    print_diff(fn1, fn2, a, b, lcs, nctx);
  free(lcs.a);
  free(lcs.b);
  free_fdata(a);
  free_fdata(b);
}

int main(int argc, char **argv)
{
  if (argc < 3) {
    printf("diff demo -- print unified diff.\n"
        "Usage: diff file1 file2 [num_context_lines]\n"
        "  file1 or file2 can be - for stdin\n");
    return 42;
  }
  // nctx is number of lines of context (as in diff -U number)
  int nctx = argc > 3 ? atoi(argv[3]) : 3;
  diff(argv[1], argv[2], nctx);
  return 0;
}
