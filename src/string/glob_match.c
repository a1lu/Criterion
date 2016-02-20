#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "glob_match.h"
/**
 * glob_match - Shell-style pattern matching, like !fnmatch(pat, str, 0)
 * @pat: Shell-style pattern to match, e.g. "*.[ch]".
 * @str: String to match.  The pattern must match the entire string.
 *
 * Perform shell-style glob matching, returning true (1) if the match
 * succeeds, or false (0) if it fails.  Equivalent to !fnmatch(@pat, @str, 0).
 *
 * Pattern metacharacters are ?, *, [ and \.
 * (And, inside character classes, !, - and ].)
 *
 * This is small and simple implementation intended for device blacklists
 * where a string is matched against a number of patterns.  Thus, it
 * does not preprocess the patterns.  It is non-recursive, and run-time
 * is at most quadratic: strlen(@str)*strlen(@pat).
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
 * An opening bracket without a matching close is matched literally.
 */

int findPatternEnd(const char* pat)
{
    int len=0;
    int paren=0;
    int patlen=strlen(pat);
    /*printf("findpatlen: %s\n", pat);*/
    if(*pat=='|')
        while(*pat != '(')
        {
            ++len; ++pat;
        }
    do{
        if(*pat=='(')
            paren++;
        else if(*pat==')')
            paren--;
        else if (paren==0 && *pat=='|')
        {
            /*puts("bar found");*/
            return len;
        }
        ++len; ++pat;
        if(len>=patlen && paren != 0)
        {
            /*printf("parens: %d\n", paren);*/
            return -1;
        }
    }while(paren>0);
    return len;
}

/*char* getString(char* string,int len)*/
/*{*/
    /*char *tmp=malloc(len+1);*/
    /*strncpy(tmp,string,len);*/
    /*tmp[len]='\0';*/
    /*free(string);*/
    /*return tmp;*/
/*}*/

char *getPattern(char** pat, int len)
{
     /*len=strlen(pat), end=0;*/
    char *tmp=0, *a=*pat;
    int count=0;
    puts("getPattern");
    if(**pat=='(')
    {
        if(len<=0)
        {
            puts("len ist null");
            return 0;
        }
        *pat+=1;
        count=findPatternEnd(*pat);
        printf("len: %d\n",count);
        if(count > 0)
        {
            tmp=malloc(count+1);
            strncpy(tmp,*pat,count);
            tmp[count]='\0';
            a[len-1]='\0';
            *pat+=count;
            /*if(**pat=='|')*/
                /**pat+=1;*/
            return tmp;
        }
    }else if(**pat == '|')
    {
       printf("else\n") ;
       /**pat+=1;*/
       count = findPatternEnd(*pat);
        printf("len else: %d\n",count);
        if(count > 0)
        {
            tmp=malloc(count+1);
            strncpy(tmp,*pat,count);
            tmp[count]='\0';
            a[len-1]='\0';
            *pat+=count;
            /*if(**pat=='|')*/
                /**pat+=1;*/
            return tmp;
        }
    }
        /*end=findPatternEnd(tmp+1);*/
    /*printf("%d\n",end);*/
    /**pat=getString(pat,end-1);*/
    /*char *t=*pat+end-1;*/
    /**t='\0';*/
    /*pat+=1;*/
    /*len=strlen(pat);*/
    /*printf("end: %s\n", *pat);*/
    /*printf("%p %c\n",t ,*t);*/
    /**pat[end-1]='\0';*/

    /**pat+=1;*/
    /*end=findPatternEnd(pat);*/
    /*printf("getpattern: %s %d\n", *pat, end);*/
    /**pat[end]='\0';*/

    /*printf("%d\n",len);*/
    /*int count=0;*/
    /*char *tmp=pat;*/
    /*while(*tmp!='|' && count < len)*/
    /*{*/
        /*tmp++;*/
        /*count++;*/
    /*}*/
    /*tmp=malloc(count+1);*/
    /*strncpy(tmp,pat,count);*/
    /*tmp[count]='\0';*/
    /*printf("count: %d\n", count);*/
    /**pat+=count;*/
    /*if(*pat=='|')*/
        /*pat+=1;*/
    /*return tmp;*/
}
#include <ctype.h>

int globmatch(char const *pat, char const *str)
{
    /*
     * Backtrack to previous * on mismatch and retry starting one
     * character later in the string.  Because * matches all characters
     * (no exception for /), it can be easily proved that there's
     * never a need to backtrack multiple levels.
     */
    char const *back_pat = NULL, *back_str = back_str;
    /*
     * Loop over each token (character or class) in pat, matching
     * it against the remaining unmatched tail of str.  Return false
     * on mismatch, or true after matching the trailing nul bytes.
     */
    for (;;) {
        unsigned char c = *str++;
        unsigned char d = *pat++;
        switch (d) {
            case '?':	/* Wildcard: anything but nul */
                if (c == '\0')
                    return 0;
                break;
            case '*':	/* Any-length wildcard */
                if (*pat == '\0')	/* Optimize trailing * case */
                    return 1;
                back_pat = pat;
                back_str = --str;	/* Allow zero-length match */
                break;
            case '[': {	/* Character class */
                          int match = 0, inverted = (*pat == '!');
                          char const *class = pat + inverted;
                          unsigned char a = *class++;
                          /*
                           * Iterate over each span in the character class.
                           * A span is either a single character a, or a
                           * range a-b.  The first span may begin with ']'.
                           */
                          do {
                              unsigned char b = a;
                              if (a == '\0')	/* Malformed */
                                  goto literal;
                              if (class[0] == '-' && class[1] != ']') {
                                  b = class[1];
                                  if (b == '\0')
                                      goto literal;
                                  class += 2;
                                  /* Any special action if a > b? */
                              }
                              match |= (a <= c && c <= b);
                          } while ((a = *class++) != ']');
                          if (match == inverted)
                              goto backtrack;
                          pat = class;
                      }
                      break;
            case '\\':
                      d = *pat++;
                      /*FALLTHROUGH*/
            default:	/* Literal character */
literal:
                      if (c == d) {
                          if (d == '\0')
                              return 0;
                          break;
                      }
backtrack:
                      if (c == '\0' || !back_pat)
                          return 0;	/* No point continuing */
                      /* Try again from last *, one character later in str. */
                      pat = back_pat;
                      str = ++back_str;
                      break;
        }
    }
}

int globmatchI(char const *pat, char const *str, int *matched)
{
    /*
     * Backtrack to previous * on mismatch and retry starting one
     * character later in the string.  Because * matches all characters
     * (no exception for /), it can be easily proved that there's
     * never a need to backtrack multiple levels.
     */
    char const *back_pat = NULL, *back_str = back_str;
    /*
     * Loop over each token (character or class) in pat, matching
     * it against the remaining unmatched tail of str.  Return false
     * on mismatch, or true after matching the trailing nul bytes.
     */
    const char *tmp=str;
    for (;;) {
        unsigned char c = *str++;
        unsigned char d = *pat++;
        switch (d) {
            case '?':	/* Wildcard: anything but nul */
                if (c == '\0')
                {
                    *matched=str-tmp-2;
                    return 0;
                }
                break;
            case '*':	/* Any-length wildcard */
                if (*pat == '\0')	/* Optimize trailing * case */
                {
                    *matched=strlen(str);
                    return 1;
                }
                back_pat = pat;
                back_str = --str;	/* Allow zero-length match */
                break;
            case '[': {	/* Character class */
                          int match = 0, inverted = (*pat == '!');
                          char const *class = pat + inverted;
                          unsigned char a = *class++;
                          /*
                           * Iterate over each span in the character class.
                           * A span is either a single character a, or a
                           * range a-b.  The first span may begin with ']'.
                           */
                          do {
                              unsigned char b = a;
                              if (a == '\0')	/* Malformed */
                                  goto literal;
                              if (class[0] == '-' && class[1] != ']') {
                                  b = class[1];
                                  if (b == '\0')
                                      goto literal;
                                  class += 2;
                                  /* Any special action if a > b? */
                              }
                              match |= (a <= c && c <= b);
                          } while ((a = *class++) != ']');
                          if (match == inverted)
                              goto backtrack;
                          pat = class;
                      }
                      break;
            case '\\':
                      d = *pat++;
                      /*FALLTHROUGH*/
            default:	/* Literal character */
literal:
                      if (c == d) {
                          if (d == '\0')
                          {
                                *matched=str-tmp-1;
                              return 1;
                          }
                          break;
                      }
backtrack:
                      if (c == '\0' || !back_pat)
                      {
                          *matched=str-tmp-2;
                          return 0;	/* No point continuing */
                      }
                      /* Try again from last *, one character later in str. */
                      pat = back_pat;
                      str = ++back_str;
                      break;
        }
    }
}

int isextglob(const char* tmp)
{
    switch(tmp[0])
    {
        case '?':
        case '*':
        case '@':
        case '!':
        case '+':
                if(tmp[1]=='(')
                    return 1;
        default: return 0;
    }
}
void printAllPattern(const char * pat,const char* str, int len)
{
    const char * tmp=pat;
    const char * s=str;
    char c;
    int i=0;
    char buf[len];
    char* b=buf;
    /*printf("enter:");*/
    /*while(i<len)*/
        /*printf("%c",pat[i++]);*/
    /*puts("");*/
    /*while(*tmp && --len)*/
    /*{*/
    while(*tmp)
    {
        while(!isextglob(tmp) && *tmp)
            *b++=*tmp++;

        *b=0;
        puts(buf);
        int m=0;
        printf("%s %s\n",buf,s);
        int res=globmatchI(buf, s,&m );
        if(res==0)
            s+=m;
        printf("%d %d %s\n",res, m, s);
        if(isextglob(tmp))
        {
            puts(buf);
            return;
        }
    }
        /*c=*tmp;*/
        /*switch(c)*/
        /*{*/
            /*case '?':*/
                /*if(tmp[0]!='(')*/
                    /*printf("?");*/
                /*else*/
                /*{*/
                    /*printf("\n");*/
                    /*int len=findPatternEnd(tmp);*/
                    /*printf("len: %d\n",len);*/
                    /*printAllPattern(tmp, len);*/
                    /*printf("\n");*/
                    /*tmp+=len;*/
                /*}*/
                /*break;*/
            /*case '*':*/
                /*if(tmp[0]!='(')*/
                    /*printf("*");*/
                /*else*/
                /*{*/

                    /*tmp++;*/
                    /*printf("\n");*/
                /*}*/
                /*break;*/
            /*case '[':*/
                    /*printf("[");*/
                /*break;*/
            /*case '\\':*/
                /*c=*tmp++;*/
            /*default:*/
                /*printf("%c",c);*/
        /*}*/
    /*}*/
    printf("\n");
}
int glob_match(char const *pat, char const *str, int *matched)
{
        /*
         * Backtrack to previous * on mismatch and retry starting one
         * character later in the string.  Because * matches all characters
         * (no exception for /), it can be easily proved that there's
         * never a need to backtrack multiple levels.
         */
        char const *back_pat = NULL, *back_str = back_str;



        printAllPattern(pat, str,strlen(pat));
        exit(0);





        printf("enter function: pattern: %s string: %s\n", pat, str);
        /*
         * Loop over each token (character or class) in pat, matching
         * it against the remaining unmatched tail of str.  Return false
         * on mismatch, or true after matching the trailing nul bytes.
         */
        for (;;) {
                unsigned char c = *str++;
                unsigned char d = *pat++;
                printf("str: %c pat: %c\n", c, d);

                switch (d) {
                case '?':       /* Wildcard: anything but nul */
                        if (c == '\0')
                                return 0;
                        if(pat[0]=='(')
                        {
                            char *pattern=0;
                            char *tmpPat=0;
                            char *holdPattern=0;
                            int len1= findPatternEnd(pat);
                            char *tmp=0;
                            if(len1<0)
                            {
                                printf("malformed patern\n");
                                exit(len1);
                            }
                            pattern=malloc(len1+1);
                            strncpy(pattern,pat,len1);
                            pattern[len1]='\0';
                            printf("pattern: %s %lu\n", pattern, strlen(pattern));
                            /*if(holdPattern==0)*/
                            /*{*/
                                /*holdPattern=malloc(len1+1);*/
                                /*strncpy(holdPattern,pattern,len1);*/
                                /*holdPattern[len1]='\0';*/
                            /*}*/
                            /*puts(holdPattern);*/
                            puts("");
                            tmp=pattern;
                            tmpPat=getPattern(&tmp, len1);
                            printf("parsing %s, pat: %s\n", tmpPat, tmp);
                            glob_match(tmpPat, str-1, matched);
                            free(tmpPat);

                            puts("");
                            printf("pattern: %s\n",tmp);
                            tmpPat=getPattern(&tmp, len1);
                            printf("parsing %s, pat: %s\n", tmpPat, tmp);
                            glob_match(tmpPat, str-1, matched);
                            free(tmpPat);
                            /*pattern+=2;*/

                            /*pattern++;*/
                            /*tmpPat=getPattern(&pattern);*/
                            /*printf("parsing %s, pat: %s\n", tmpPat, pattern);*/
                            /*free(tmpPat);*/

                            /*pattern++;*/
                            /*tmpPat=getPattern(&pattern);*/
                            /*[>printf("%s\n", tmpPat);<]*/
                            /*printf("parsing %s, pat: %s\n", tmpPat, pattern);*/
                            /*free(tmpPat);*/

                            /*free(holdPattern);*/
                            free(pattern);
                            exit(0);
                            ++pat;
                            int count=0, len=0;;
                            /*int x=0;*/
                            /*const char *tmp=pat;*/
                            while(pat[count] != ')')
                            {
                                len=0;

                                while(pat[count+len] != '|' && pat[count+len]!=')')
                                {
                                    /*puts(pat+count+len);*/
                                    ++len;
                                }
                                /*printf("count: %d\n",count);*/
                                int m=0;
                                /*printf("pattern: %s\n",pat+count);*/
                                char *rec_pat=malloc(len+1);
                                strncpy(rec_pat,pat+count,len);
                                int match=glob_match(rec_pat, str-1, &m);
                                free(rec_pat);
                                /*printf("MATCHED = %d\n",m);*/
                                if(m>1)
                                    str+=m;
                                if(match && *str=='\0')
                                    return 1;
                                /*printf("str: %s\n",str);*/
                                /*printf("len: %d: %s : match: %d\n",len,rec_pat, match);*/
                                count+=len;
                                if(pat[count]=='|')
                                    count++;
                                /*pat+=len;*/

                                /*printf("count: %d\n",count);*/
                                /*printf("PATTER COUNT: %c\n",pat[count]);*/
                                /*puts("");*/
                                /*if(x==3)*/
                                    /*exit(1);*/
                                /*x++;*/
                                /*back_str=--str;*/
                                /*back_pat = pat;*/
                                /*printf("back: %s %s\n",back_pat, back_str);*/

                            }
                            pat+=count+1;
                        }else
                        {
                            *matched+=1;
                        }
                        break;
                case '*':       /* Any-length wildcard */
                        if (*pat == '\0')       /* Optimize trailing * case */
                        {
                                /*puts("ende");*/
                                /*printf("matched: %d\n",*matched);*/
                                *matched+=strlen(str);
                                return 1;
                        }
                        back_pat = pat;
                        back_str = --str;       /* Allow zero-length match */
                        /*printf("back: %s %s\n",back_pat, back_str);*/
                        break;
                case '[': {     /* Character class */
                        int match = 0, inverted = (*pat == '!');
                        printf("inv %d ", inverted);
                        char const *class = pat + inverted;
                        printf("class %s ", class);
                        unsigned char a = *class++;
                        printf("class %s\n", class);
                        /*
                         * Iterate over each span in the character class.
                         * A span is either a single character a, or a
                         * range a-b.  The first span may begin with ']'.
                         */
                        do {
                                unsigned char b = a;

                                if (a == '\0')  /* Malformed */
                                        goto literal;

                                if (class[0] == '-' && class[1] != ']') {
                                        b = class[1];

                                        if (b == '\0')
                                                goto literal;

                                        class += 2;
                                        /* Any special action if a > b? */
                                }
                                match |= (a <= c && c <= b);
                                printf("match %d\n",match);
                        } while ((a = *class++) != ']');

                        if (match == inverted)
                                goto backtrack;
                        pat = class;
                        }
                        break;
                case '\\':
                        d = *pat++;
                        /*FALLTHROUGH*/
                default:        /* Literal character */
literal:
                        if (c == d) {
                            printf("match: %c %c\n",c,d);
                                if (d == '\0')
                                {
                                        printf("matched: %d\n",*matched);
                                        return 1;
                                }
                                *matched+=1;;
                                break;
                        }
backtrack:
                        /*printf("backtrace: %c %s\n",c, back_pat);*/
                        if (c == '\0' || !back_pat)
                        {
                                printf("matched: %d\n",*matched);
                                return 0;   /* No point continuing */
                        }
                        /* Try again from last *, one character later in str. */
                        pat = back_pat;
                        str = ++back_str;
                        *matched+=1;
                        puts("back");
                        /*printf("back: %s %s\n",pat, str);*/
                        break;
                }
        }
}
