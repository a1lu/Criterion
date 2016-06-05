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
    char const **pat_ptr;
    char const **end_ptr;
    char const **str_ptr;
    char *op_ptr;
    char op;
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

static char const *stack_pat[100];
static char const *stack_end[100];
static char const *stack_str[100];
static char stack_op[100];

#define push(sp, n) (*((sp)++) = (n))
#define pop(sp) (*--(sp))
#define stack_empty_p(sp) !(sp-stack_pat)

/*#define push(sp, n) (*((sp)++) = (n))*/
/*#define pop(sp) (*--(sp))*/
#define stack_empty_e(sp) !(sp-stack_end)

/*#define push(sp, n) (*((sp)++) = (n))*/
/*#define pop(sp) (*--(sp))*/
#define stack_empty_s(sp) !(sp-stack_str)
#define stack_empty_o(sp) !(sp-stack_op)

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
    push(con.op_ptr, con.op);
    /*if(con.repeat)*/
    /*{*/
        /*con.repeat_pat=pop(con.pat_ptr);*/
        /*con.pat_chunk_end=pop(con.end_ptr);*/
    /*}*/
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
            case '|':
                {
                    if(!con.nested)
                        malformed_pattern(p);
                    int count = 1;
                    // TODO use end_ptr?
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
                /* get op from stack */
                char op='\0';
                if(!stack_empty_o(con.op_ptr))
                    op = pop(con.op_ptr);
                      // TODO eval op, push it back

                if (con.nested < 0) {
                    malformed_pattern(p);
                } else /* dont get a new repeat string if we in a !() pattern
                        we only need this for repeating ops TODO better way */
                if (con.pat == con.pat_chunk_end && op != '!') {
                    con.repeat_str = con.repeat_pat ? con.str : NULL;
                    push(con.str_ptr, con.repeat_str);
                }
                break;
            case '?':
                /* Matches zero or one occurence of the given patterns */
                if (con.pat[0] == '(') {
                    struct ext_glob eg = find_next((struct ext_glob)
                                                {.cur_pat = con.pat});
                    /* Match zero occurence */
                    struct context arg = con;
                    arg.pat = eg.end_pat;
                    arg.back_pat = back_pat; arg.back_str = back_str;
                    int res = glob_match(arg);
                    if (res == 1) {
                        return res;
                    } else {
                        arg.op = '?';
                        ++arg.nested;
                        while (!eg.ret) {
                            arg.pat=eg.cur_pat;
                            res = glob_match(arg);
                            if (res == 1) {
                                return res;
                            }
                            eg = find_next(eg);
                        }
                    }
                    return 0;
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

                    struct context arg = con;
                    ++arg.nested; arg.op = '@';
                    arg.back_pat = back_pat; arg.back_str = back_str;

                    while (!eg.ret) {
                        arg.pat = eg.cur_pat;
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
                    char const ** sp = con.pat_ptr;

                    struct context arg = con;
                    arg.pat = eg.end_pat;
                    arg.back_pat = back_pat; arg.back_str = back_str;

                    res = glob_match(arg);
                    if (res == 1) {
                        return 1;
                    } else {
                        arg.repeat = 1;
                        ++arg.nested;
                        arg.op = '*';
                        arg.pat_chunk_end = eg.end_pat;

                        while (!eg.ret) {
                                arg.pat = eg.cur_pat;
                                arg.repeat_pat = eg.cur_pat;

                            if(con.repeat_pat) {
                                push(con.pat_ptr,con.repeat_pat);
                            }
                            /*push(con.pat_ptr,eg.cur_pat);*/
                            if(con.pat_chunk_end)
                            {
                                push(con.end_ptr, con.pat_chunk_end);
                            }
                            /*push(con.end_ptr, eg.end_pat);*/
                            /*struct context arg = {  .str = con.str,*/
                                /*.pat = eg.cur_pat,*/
                                /*.pat_chunk_end = eg.end_pat,*/
                                /*.back_pat = back_pat,*/
                                /*.back_str = back_str,*/
                                /*.repeat = 1,*/
                                /*.repeat_pat = eg.cur_pat,*/
                                /*.nested = con.nested + 1,*/
                                /*.pat_ptr = con.pat_ptr,*/
                                /*.end_ptr = con.end_ptr};*/
                            res = glob_match(arg);
                            if (res == 1) {
                                return res;
                            }
                            eg = find_next(eg);
                            con.pat_ptr = sp;
                        }
                    }
                    /*con.pat = eg.end_pat;*/
                    return 0;
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
                    struct context arg = con;
                    ++arg.nested; arg.op = '+';
                    arg.back_pat = back_pat; arg.back_str = back_str;
                    arg.pat_chunk_end = eg.end_pat; arg.repeat = 1;
                    arg.repeat_pat = eg.cur_pat;

                    while (!eg.ret) {
                        if(con.repeat_pat) {
                            push(con.pat_ptr,con.repeat_pat);
                        }
                        /*push(con.pat_ptr,eg.cur_pat);*/
                        if(con.pat_chunk_end)
                        {
                            push(con.end_ptr, con.pat_chunk_end);
                        }
                        /*push(con.end_ptr, eg.end_pat);*/
                        arg.pat = eg.cur_pat;
                        /*struct context arg = {  .str = con.str,*/
                                                /*.pat = eg.cur_pat,*/
                                                /*.pat_chunk_end = eg.end_pat,*/
                                                /*.back_pat = back_pat,*/
                                                /*.back_str = back_str,*/
                                                /*.repeat = 1,*/
                                                /*.repeat_pat = eg.cur_pat,*/
                                                /*.nested = con.nested + 1,*/
                                                /*.pat_ptr = con.pat_ptr,*/
                                                /*.end_ptr = con.end_ptr};*/
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
                    if (c == '\0' && *eg.end_pat == '\0') /// TODO check this
                    {
                        return 1 ^ con.except;
                    }
                    int res;
                    char const ** sp = con.pat_ptr;
                    while (!eg.ret) {
                        if(con.repeat_pat) {
                            push(con.pat_ptr,con.repeat_pat);
                        }
                        struct context arg = {  .str = con.str,
                                                .pat = eg.cur_pat,
                                                .pat_chunk_end = eg.end_pat,
                                                .back_pat = back_pat,
                                                .back_str = back_str,
                                                .except = 1,
                                                .nested = con.nested + 1,
                                                .repeat = con.repeat,
                                                .repeat_pat = con.repeat_pat,
                                                .pat_ptr = con.pat_ptr,
                                                .end_ptr = con.end_ptr};
                        res = glob_match(arg);
                        if (res == 1) {
                            goto backtrack;
                        }
                        eg = find_next(eg);
                        con.pat_ptr = sp;
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
                }
backtrack:
                if (c == '\0' || (!back_pat && !con.repeat)) {
                    return 0;
                }
                if (back_pat)
                {
                    /* No point continuing */
                    /* Try again from last *, one character later in str. */
                    con.pat = back_pat;
                    con.str = ++back_str;
                    con.nested = nested;
                    break;
                } else if (!con.except){
                    if (con.repeat && (con.pat >= con.pat_chunk_end)) {
                        /* try to match the pattern one more time */
                        con.pat = con.repeat_pat;
                        con.str = con.repeat_str;
                        con.nested = nested;
                    } else if (con.repeat) {
                        /* get previous repeat pattern */
                        if(!stack_empty_p(con.pat_ptr) &&
                                !stack_empty_e(con.end_ptr) &&
                                !stack_empty_s(con.str_ptr)) {
                            con.repeat_pat = pop(con.pat_ptr);
                            con.pat_chunk_end = pop(con.end_ptr);
                            con.repeat_str = pop(con.str_ptr);
                            break;
                        } else {
                            return 0;
                        }
                    }
                }
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
    return glob_match((struct context){.pat = pat, .str = str,
                                    .pat_ptr = stack_pat, .end_ptr = stack_end,
                                    .str_ptr = stack_str, .op_ptr = stack_op});
}
