/*
 * The MIT License (MIT)
 *
 * Copyright © 2014 George Spelvim <linux@horizon.com>
 * Copyright © 2016 Matthias "ailu" Günzel <a1lu@arcor.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "extglobmatch.h"
#include "criterion/internal/common.h"

struct context{
    int repeat;
    int except;
    int nested;
    char const *pat;
    char const *str;
    char const *pat_chunk_end;
    char const *back_pat;
    char const *back_str;
    char const *repeat_pat;
    char const *repeat_str;
};

struct ext_glob{
    char const *cur_pat;
    char const *end_pat;
    int ret;
};


static char const *find_pattern_end(char const *pat) {
    int len = 0;
    int paren = 0;
    int patlen = strlen(pat);
    do {
        if (*pat == '(')
            paren++;
        else if (*pat == ')')
            paren--;
        ++len; ++pat;
    } while (paren > 0 && len < patlen);
    if (paren) {
        fprintf(stderr, "pattern error: mismatching parenthesis\n");
        exit(3);
    }
    return pat;
}

static struct ext_glob find_next(struct ext_glob con) {
    if (*con.cur_pat == '(') {
        con.end_pat = find_pattern_end(con.cur_pat);
        ++con.cur_pat;
        return con;
    }

    int count = 0;
    unsigned char p = *con.cur_pat++;
    while (count >= 0 && p != '\0' && con.cur_pat < con.end_pat) {
        switch(p) {
            case '(': ++count;
                      break;
            case ')': --count;
                      break;
            case '|': if (count == 0)
                          return con;
                      break;
        }
        p = *con.cur_pat++;
    }
    con.ret = 1;
    return con;
}

CR_NORETURN static void malformed_pattern(char p) {
    fprintf(stderr, "pattern error: Unexpected '%c'\n", p);
    exit(3);
}

char const *repeat_stack[1000];
char const * *repeat_ptr=NULL;

#define push(sp, n) (*((sp)++) = (n))
#define pop(sp) (*--(sp))
#define stack_empty(sp) !(sp-repeat_stack)

/**
 * glob_match - Shell-style pattern matching, like !fnmatch(pat, str, 0)
 * @con: struct context object holding the necessary informations
 *
 * Perform shell-style glob matching, returning true (1) if the match
 * succeeds, or false (0) if it fails.  Equivalent to !fnmatch(@pat, @str, 0).
 * Supports extended glob matching.
 *
 * Pattern metacharacters are ?, *, [,\, @, +, (, ), !, and |.
 * (And, inside character classes, !, - and ].)
 *
 * This implementation is based on the glob matcher by George Spelvim
 * (lib/glob.c)
 * This is small and simple implementation intended for test selecting
 * where a string is matched against a pattern. Thus, it does not preprocess
 * the patterns. The original implementation was non-recursive, recursion is
 * now used to implement extended globs. The run-time is at most quadratic:
 * strlen(@str)*strlen(@pat).
 *
 * An example of the worst case is glob_match("*aaaaa", "aaaaaaaaaa");
 * it takes 6 passes over the pattern before matching the string.
 *
 * Like !fnmatch(@pat, @str, 0) and unlike the shell, this does NOT
 * treat / or leading . specially; it isn't actually used for pathnames.
 *
 * Note that according to glob(7) (and unlike bash), character classes
 * are complemented by a leading !; this does not support the regex-style
 * [^a-z] syntax.
 *
 * The supported globs are:
 * *
 *      Matches any string including the NULL string.
 * ?
 *      Matches any single character.
 * [...]
 *      Matches any of the enclosed characters. A pair of characters separated
 *      by a hyphen denotes a range expression.
 * [!...]
 *      Matches none of the enclosed characters.
 *
 * The supported extended globs are:
 * ?(pattern-list)
 *      Matches zero or one occurrence of the given patterns.
 * *(pattern-list)
 *      Matches zero or more occurrences of the given patterns.
 * +(pattern-list)
 *      Matches one or more occurrences of the given patterns.
 * @(pattern-list)
 *      Matches one of the given patterns.
 * !(pattern-list)
 *      Matches anything except one of the given patterns.
 */

static int glob_match(struct context con) {
    /*
     * Backtrack to previous * on mismatch and retry starting one
     * character later in the string.  Because * matches all characters
     * (no exception for /), it can be easily proved that there's
     * never a need to backtrack multiple levels.
     */
    char const *back_pat = con.back_pat, *back_str = con.back_str;

    /*
     * Backtrack to previous repeating pattern start and retry starting with the
     * string starting at the not matching character.
     */
    int nested = con.nested;
    if(con.repeat)
    {
        con.repeat_pat=pop(repeat_ptr);
    }
    /*
     * Loop over each token (character or class) in pat, matching
     * it against the remaining unmatched tail of str.  Return false
     * on mismatch, or true after matching the trailing nul bytes.
     */
    for (;;) {
        unsigned char c = *con.str;
        unsigned char p = *con.pat++;
        switch (p) {
            case '(':
                malformed_pattern(p);
                break;
            case '|': {
                    if(!con.nested)
                        malformed_pattern(p);
                    int count = 1;
                    while (count > 0) {
                        p = *con.pat++;
                        if (p == '(')
                            ++count;
                        if (p == ')')
                            --count;
                    }
                }
                /* FALLTHROUGH */
            case ')':
                con.nested--;
                if (con.nested < 0) {
                    malformed_pattern(p);
                }
                if(con.nested > 0 || nested == 1) {
                    con.repeat_str = con.repeat_pat ? con.str : NULL;
                    if (con.repeat && !stack_empty(repeat_ptr)) {
                        con.repeat_pat=pop(repeat_ptr);
                        push(repeat_ptr,con.repeat_pat);
                    }
                }
                break;
            case '?':
                /* Matches zero or one occurence of the given patterns */
                if (con.pat[0] == '(') {
                    struct ext_glob eg = find_next((struct ext_glob)
                                                        {.cur_pat = con.pat});
                    int res;
                    while (!eg.ret) {
                        if(con.repeat_pat) {
                            push(repeat_ptr,con.repeat_pat);
                        }
                        struct context arg = {  .str = con.str,
                                                .pat = eg.cur_pat,
                                                .back_pat = back_pat,
                                                .back_str = back_str,
                                                .pat_chunk_end = con.pat_chunk_end,
                                                .nested = con.nested + 1,
                                                .repeat = con.repeat,
                                                .repeat_pat = con.repeat_pat};
                        res = glob_match(arg);
                        if (res == 1) {
                            return res;
                        }
                        eg = find_next(eg);
                    }
                    con.pat = eg.end_pat;
                } else {   /* Wildcard: anything but nul */
                    if (c == '\0')
                        return 0;
                    ++con.str;
                }
                break;
            case '@':
                /* Matches one of the given patterns */
                if (con.pat[0] == '(') {
                    struct ext_glob eg = find_next((struct ext_glob)
                                                        {.cur_pat = con.pat});
                    if (c == '\0') /// TODO check this
                        return 0;
                    int res;
                    while (!eg.ret) {
                        if(con.repeat_pat) {
                            push(repeat_ptr,con.repeat_pat);
                        }
                        struct context arg = {  .str = con.str,
                                                .pat = eg.cur_pat,
                                                .back_pat = back_pat,
                                                .back_str = back_str,
                                                .pat_chunk_end = con.pat_chunk_end,
                                                .nested = con.nested + 1,
                                                .repeat = con.repeat,
                                                .repeat_pat = con.repeat_pat};
                        res = glob_match(arg);
                        if (res == 1) {
                            return res;
                        }
                        eg = find_next(eg);
                    }
                } else {
                    malformed_pattern(p);
                }
                return 0;
            case '*':
                /* Matches zero or more occurences of the given patterns */
                if (con.pat[0] == '(')
                {
                    struct ext_glob eg = find_next((struct ext_glob)
                                                        {.cur_pat = con.pat});
                    int res;
                    char const ** sp=repeat_ptr;
                    while (!eg.ret) {
                        if(con.repeat_pat) {
                            push(repeat_ptr,con.repeat_pat);
                        }
                        push(repeat_ptr,eg.cur_pat);
                        struct context arg = {  .str = con.str,
                                                .pat = eg.cur_pat,
                                                .pat_chunk_end = eg.end_pat,
                                                .back_pat = back_pat,
                                                .back_str = back_str,
                                                .repeat = 1,
                                                .repeat_pat = eg.cur_pat,
                                                .nested = con.nested + 1};
                        res = glob_match(arg);
                        if (res == 1) {
                            return res;
                        } else if (res == -1) {
                            return res;
                        }
                        eg = find_next(eg);
                        repeat_ptr=sp;
                    }
                    con.pat = eg.end_pat;
                    if(c =='\0' && *con.pat != '\0') {
                        return -1;
                    }
                } else { /* Any-length wildcard */
                    if  (*con.pat == '\0') { /* Optimize trailing * case */
                        return 1;
                    }
                    back_pat = con.pat;
                    back_str = con.str;   /* Allow zero-length match */
                }
                break;
            case '+':
                /* Matches one or more of the given patterns */
                if (con.pat[0] == '(') {
                    struct ext_glob eg = find_next((struct ext_glob)
                                                        {.cur_pat = con.pat});
                    if (c == '\0') /// TODO check this
                        return 0;
                    int res;
                    while (!eg.ret) {
                        if(con.repeat_pat) {
                            push(repeat_ptr,con.repeat_pat);
                        }
                        push(repeat_ptr,eg.cur_pat);
                        struct context arg = {  .str = con.str,
                                                .pat = eg.cur_pat,
                                                .pat_chunk_end = eg.end_pat,
                                                .back_pat = back_pat,
                                                .back_str = back_str,
                                                .repeat = 1,
                                                .repeat_pat = eg.cur_pat,
                                                .nested = con.nested + 1};
                        res = glob_match(arg);
                        if (res == 1) {
                            return res;
                        }
                        eg = find_next(eg);
                    }
                } else {
                    malformed_pattern(p);
                }
                return 0;
            case '!':
                /* Matches anything except one of the given patterns */
                if (con.pat[0] == '(') {
                    struct ext_glob eg = find_next((struct ext_glob)
                                                        {.cur_pat = con.pat});
                    if (c == '\0') /// TODO check this
                        return 1 ^ con.except;
                    int res;
                    char const ** sp=repeat_ptr;
                    while (!eg.ret) {
                        if(con.repeat_pat) {
                            push(repeat_ptr,con.repeat_pat);
                        }
                        struct context arg = {  .str = con.str,
                                                .pat = eg.cur_pat,
                                                .pat_chunk_end = eg.end_pat,
                                                .back_pat = back_pat,
                                                .back_str = back_str,
                                                .except = 1,
                                                .nested = con.nested + 1,
                                                .repeat = con.repeat,
                                                .repeat_pat = con.repeat_pat};
                        res = glob_match(arg);
                        if (res == 1) {
                            goto backtrack;
                        }
                        eg = find_next(eg);
                        repeat_ptr=sp;
                    }
                    con.pat = eg.end_pat;
                    back_pat = con.pat;
                    back_str = --con.str;
                    goto backtrack;
                } else {
                    malformed_pattern(p);
                }
                break;
            case '[': /* Character class */
                {
                    int match = 0, inverted = (*con.pat == '!');
                    char const *class = con.pat + inverted;
                    unsigned char a = *class++;
                    /*
                     * Iterate over each span in the character class.
                     * A span is either a single character a, or a
                     * range a-b.  The first span may begin with ']'.
                     */
                    do {
                        unsigned char b = a;
                        if (a == '\0')	/* Malformed */
                            malformed_pattern(p);
                        if (class[0] == '-' && class[1] != ']') {
                            b = class[1];
                            if (b == '\0')
                                malformed_pattern(p);
                            class += 2;
                            /* Any special action if a > b? */
                        }
                        match |= (a <= c && c <= b);
                    } while ((a = *class++) != ']');
                    if (match == inverted)
                        goto backtrack;
                    con.pat = class;
                    ++con.str;
                }
                break;
            case '\\':
                p = *con.pat++;
                /*FALLTHROUGH*/
            default:    /* Literal character */
                if (c == p) {
                    ++con.str;
                    if (p == '\0') {
                        return 1;
                    }
                    break;
                } else if (!con.except){
                    if (con.repeat && (con.pat >= con.pat_chunk_end)) {
                        /* try to match the pattern one more time */
                        con.pat = con.repeat_pat;
                        con.str = con.repeat_str;
                        con.nested = nested;
                        if (!con.str) {
                            return 0;
                        }
                        break;
                    } else if(con.repeat && !stack_empty(repeat_ptr)) {
                        /* get previous repeat pattern */
                        con.repeat_pat = pop(repeat_ptr);
                        if(con.repeat_pat)
                            break;
                    }
                }
backtrack:
                if (c == '\0' || !back_pat) {
                    return 0;
                }
                /* No point continuing */
                /* Try again from last *, one character later in str. */
                con.pat = back_pat;
                con.str = ++back_str;
                con.nested = nested;
                break;
        }
    }
}
/**
 * match - Shell-style pattern matching, like !fnmatch(pat, str, 0)
 * @pat: Shell-style pattern to match, e.g. "*.[ch]".
 * @str: String to match.  The pattern must match the entire string.
 *
 * Perform shell-style glob matching, returning true (1) if the match
 * succeeds, or false (0) if it fails.  Equivalent to !fnmatch(@pat, @str, 0).
 * Supports extended glob matching.
 *
 * Pattern metacharacters are ?, *, [,\, @, +, (, ), !, and |.
 * (And, inside character classes, !, - and ].)
 *
 * This implementation is based on the glob matcher by George Spelvim
 * (lib/glob.c)
 * This is small and simple implementation intended for test selecting
 * where a string is matched against a pattern. Thus, it does not preprocess
 * the patterns. The original implementation was non-recursive, recursion is
 * now used to implement extended globs. The run-time is at most quadratic:
 * strlen(@str)*strlen(@pat).
 *
 * An example of the worst case is glob_match("*aaaaa", "aaaaaaaaaa");
 * it takes 6 passes over the pattern before matching the string.
 *
 * Like !fnmatch(@pat, @str, 0) and unlike the shell, this does NOT
 * treat / or leading . specially; it isn't actually used for pathnames.
 *
 * Note that according to glob(7) (and unlike bash), character classes
 * are complemented by a leading !; this does not support the regex-style
 * [^a-z] syntax.
 *
 * The supported globs are:
 * *
 *      Matches any string including the NULL string.
 * ?
 *      Matches any single character.
 * [...]
 *      Matches any of the enclosed characters. A pair of characters separated
 *      by a hyphen denotes a range expression.
 * [!...]
 *      Matches none of the enclosed characters.
 *
 * The supported extended globs are:
 * ?(pattern-list)
 *      Matches zero or one occurrence of the given patterns.
 * *(pattern-list)
 *      Matches zero or more occurrences of the given patterns.
 * +(pattern-list)
 *      Matches one or more occurrences of the given patterns.
 * @(pattern-list)
 *      Matches one of the given patterns.
 * !(pattern-list)
 *      Matches anything except one of the given patterns.
 */
int extglob_match(char const *pat, char const *str)
{
    repeat_ptr = repeat_stack;
    return glob_match((struct context){.pat = pat, .str = str});
}
