#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "glob_match.h"

/*struct pat_chunk{*/
    /*char *pat;*/
    /*char *str;*/
    /*char *pat_chunk_end;*/
    /*int pat_len;*/
    /*int repeat;*/
/*};*/
struct context{
    int repeat;
    int except;
    char const *pat;
    char const *str;
    char const *pat_chunk_end;
    int pat_chunk_len;
    int matched;
    char const *back_pat;
    char const *back_str;
};

enum ext_ret_val{FINISHED=1};

struct ext_glob{
    char const * cur_pat;
    char const * end_pat;
    int ret;
};

char const *findNext(char const * pat, char const* ep, int count)
{
    if(*pat=='(')
        return ++pat;

    /*printf("enter findNext: pe: %s , pat: %s\n",ep,pat);*/
    /*int count=1;*/
    /*printf("%s\n",pat);*/
    unsigned char p=*pat++;
    if(pat >= ep)
        return 0;
    while(count>0 && p!='\0')
    {
        /*printf("in f: %c\n", *pat);*/
        p=*pat++;
        if(pat >= ep)
            return 0;
        switch(p)
        {
            case '(': ++count;
                      break;
            case ')': --count;
                      if(count==0)
                          return pat;
                      break;
            case '|': if(count==1)
                          return pat;
                      break;
        }
    }
    /*puts("asd");*/
    return 0;
}

char const *findPatternEnd(char const *pat)
{
    int len=0;
    int paren=0;
    int patlen=strlen(pat);
    do{
        if(*pat=='(')
            paren++;
        else if(*pat==')')
            paren--;
        ++len; ++pat;
    }while(paren>0 && len<patlen);
    return pat;
}
/*struct context findNextS(char const * pat, char const* ep, int count)*/
struct ext_glob findNextS(struct ext_glob con)
{
    struct ext_glob ret ={.cur_pat=con.cur_pat};
    if(*ret.cur_pat=='(')
    {
        ret.end_pat = findPatternEnd(ret.cur_pat);
        ++ret.cur_pat;
        return ret;
    }

    /*printf("enter findNext: pe: %s , pat: %s\n",ep,pat);*/
    int count=0;
    /*printf("%s\n",pat);*/
    unsigned char p=*ret.cur_pat++;
    /*if(pat >= ep)*/
        /*return 0;*/
    /*printf("%s\n",ret.cur_pat < con.end_pat?"kleiner":"größer");*/
    /*printf("%c %d\n",p,count);*/
    while(count>=0 && p!='\0' && ret.cur_pat < con.end_pat)
    {
        /*printf("in f: %c , count: %d\n", p,count);*/
        switch(p)
        {
            case '(': ++count; puts("inc");
                      break;
            case ')': --count;
                      /*if(count<0)*/
                          /*return ret;*/
                      break;
            case '|': if(count==0)
                          return ret;
                      break;
        }
        p=*ret.cur_pat++;
    }
    /*puts("ret");*/
    /*puts("asd");*/
    return (struct ext_glob) {.ret=FINISHED};
}

void  print_ext(char const *pat)
{
    for (;;) {
        unsigned char p = *pat++;
        if(p=='\0')
            break;
        switch(p){
            case '?':
                if(*pat=='(')
                {
                    struct ext_glob eg=findNextS((struct ext_glob){.cur_pat=pat});
                    puts("start");
                    puts(eg.end_pat);
                    while(!eg.ret)
                    {
                        puts(eg.cur_pat);
                        eg=findNextS(eg);
                    }
                    puts("end\n");
                }
        }
    }
}

void printContext(struct context con)
{
    printf("rep: %d exc: %d str: %s pat: %s back_str: %s back_pat: %s ch_end: %s\
            matched: %d\n",
            con.repeat, con.except, con.str, con.pat,
            con.back_str, con.back_pat, con.pat_chunk_end, con.matched );
}

struct context globmatchS(struct context con)
{
    /*const char *save_str=str;*/
    /*const char *save_pat=pat;*/
    /*puts("enter");*/
    /*
     * Backtrack to previous * on mismatch and retry starting one
     * character later in the string.  Because * matches all characters
     * (no exception for /), it can be easily proved that there's
     * never a need to backtrack multiple levels.
     */
    char const *back_pat = con.back_pat, *back_str = con.back_str;
    char const *repeat_pat= con.repeat? con.pat : NULL;
    char const *repeat_str=NULL;
    /*
     * Loop over each token (character or class) in pat, matching
     * it against the remaining unmatched tail of str.  Return false
     * on mismatch, or true after matching the trailing nul bytes.
     */
    /*printf(" enter: ");*/
    /*printContext(con);*/
    for (;;) {
        unsigned char c = *con.str++;
        unsigned char p = *con.pat++;
        /*printContext(con);*/
        printf("begin c: %c p: %c\n", c , p);
        /*if(p==')')*/
        /*{*/
            /*puts("klammer");*/
            /*count--;*/
            /*printf("count: %d\n",count);*/
            /*while(count>0)*/
            /*{*/
                /*printf("%c\n",p);*/
                /*p=*pat++;*/
                /*if(p==')')*/
                    /*count--;*/
                /*if(p=='(')*/
                    /*count++;*/
                /*printf("count-- %d\n",count);*/
            /*}*/
            /*p=*pat++;*/
            /*printf("char: %c p: %c, pat: %s\n",c ,p,pat);*/
            /*if(p=='\0' || c == '\0')*/
            /*{*/
                /*puts("ende klammer");*/
                /*return str-1;*/
            /*}*/
        /*}*/
        /*printf("p: %c , c: %c\n",p,c);*/
        if(p==')' || p=='|')
        {
                puts(") or |");
            if(repeat_pat)
            {
                puts("repeat");
                repeat_str=con.str-1;
            }
            if(con.pat_chunk_end)
            {
                con.pat=con.pat_chunk_end;
                p = *con.pat++;
                printf("c: %c p: %c\n", c , p);
                /*con.except=0;*/
            }
            /*con.except=0;*/
            if(con.except)
            {
                puts("! pattern matched");
                return (struct context){.matched=1, .str=con.str};
            }
        }
        /// TODO: malformed bracket strings
            /*else if (p==')')*/
            /*{*/
                /*puts("unexpected word \')\'");*/
                /*exit(1);*/
            /*}*/
            /*printf("c: %c p: %c\n", c , p);*/
        switch (p) {
            /*case '|':*/
                /*if(con.pat_chunk_end)*/
                    /*con.pat=con.pat_chunk_end;*/
                /*con.str--;*/
                /*printf("count %d\n",count);*/
                      /*puts("ende |");*/
                      /*while(count>0)*/
                      /*{*/
                          /*[>printf("%c\n",p);<]*/
                          /*p=*pat++;*/
                          /*if(p==')')*/
                              /*count--;*/
                        /*if(p=='(')*/
                            /*count++;*/
                      /*}*/
                      /*printf("|: %s char: %c\n",pat,c);*/
                      /*--str;*/
                      /*break;*/
                      /*return str;*/
                /*break;*/
            case '?':
                /* Matches zero or one occurence of the given patterns */
                if(con.pat[0]=='(')
                {
                    /*printf("aa: c: %c p: %c\n",c ,p);*/
                    /*if(c == '\0')*/
                    /*return (struct context){.matched=1};*/
                    puts("?");
                    /*printf("%s\n",pat+1);*/
                    --con.str;
                    struct ext_glob eg=findNextS((struct ext_glob){.cur_pat=con.pat});
                    char const *pat_end=eg.end_pat > con.pat_chunk_end ?
                        eg.end_pat: con.pat_chunk_end;
                    struct context res;
                    while(!eg.ret)
                    {
                        struct context arg={.str=con.str, .pat=eg.cur_pat,
                            .pat_chunk_end=pat_end, .back_pat=back_pat,
                            .back_str=back_str};
                        /*printContext(arg);*/
                        res=globmatchS(arg);
                        /*printContext(res);*/
                        if(res.matched==1)
                        {
                            /*puts("rec matched");*/
                            return res;
                        }
                        eg=findNextS(eg);
                    }
                    con.pat=pat_end;
                    printf("str: %s pat: %s \n",con.str, con.pat);
                    /*p=*con.pat++;*/
                    /*c=*con.str++;*/
                    /*printf("c: %c p: %c\n",c ,p);*/
                    /*goto literal;*/
                }else   /* Wildcard: anything but nul */
                {
                    if (c == '\0')
                        return (struct context){.matched=0, .str=con.str};
                }
                break;
            case '@':
                /* Matches one of the given patterns */
                if(con.pat[0]=='(')
                {
                    /*printf("aa: c: %c p: %c\n",c ,p);*/
                    if(c == '\0') /// TODO check this
                        return (struct context){.matched=0, .str=con.str};
                    puts("@");
                    /*printf("%s\n",pat+1);*/
                    --con.str;
                    struct ext_glob eg=findNextS((struct ext_glob){.cur_pat=con.pat});
                    char const *pat_end=eg.end_pat > con.pat_chunk_end ?
                        eg.end_pat: con.pat_chunk_end;
                    struct context res;
                    while(!eg.ret)
                    {
                        struct context arg={.str=con.str, .pat=eg.cur_pat,
                            .pat_chunk_end=pat_end, .back_pat=back_pat,
                            .back_str=back_str};
                        /*printContext(arg);*/
                        res=globmatchS(arg);
                        /*printContext(res);*/
                        if(res.matched==1)
                        {
                            /*puts("rec matched");*/
                            return res;
                        }
                        eg=findNextS(eg);
                    }
                    return res;
                }else
                {
                    /// TODO malformed string
                    goto literal;
                }
                    /*if(pat[0]=='(')*/
                    /*{*/
                        /*puts("@");*/
                        /*count++;*/
                        /*[>printf("%s\n",pat+1);<]*/
                        /*[>pat=findNext(pat);<]*/
                        /*char const* fp=pat;*/
                        /*char const* ep=findPatternEnd(pat);*/
                        /*[>printf("pat: %s pat end %s\n",pat,ep);<]*/
                        /*str--;*/
                        /*char const* res=NULL;*/
                        /*while((fp=findNext(fp,ep, count)))*/
                        /*{*/
                            /*[>printf("fp: %s %d\n",fp, count);<]*/
                            /*pat=fp;*/
                            /*res=globmatch(fp,str, count);*/
                            /*[>printf("res: %s ",res?"true":"false");<]*/
                            /*[>printf("%p- %p\n", res, (save_str+strlen(save_str)));<]*/
                            /*if(res==save_str+strlen(save_str))*/
                                /*return res;*/
                            /*if(res)*/
                            /*{*/
                                /*puts("break");*/
                                /*break;*/
                            /*}*/
                            /*[>else<]*/
                                /*[>return 0;<]*/
                            /*[>printf("char: %c str: %s pattern: %s, p %c res: %s\n\n",c,str,pat,p,res);<]*/
                        /*}*/
                        /*if(!res)*/
                            /*return 0;*/
                        /*[>printf("fp null: %s\n",fp);<]*/
                        /*[>printf("%s %d\n",pat,count);<]*/
                        /*while(count>0)*/
                        /*{*/
                            /*p=*pat++;*/
                            /*printf("dec: %c\n",p);*/
                            /*if(p==')')*/
                                /*count--;*/
                        /*}*/
                        /*[>printf("char: %c p: %c, pat: %s str: %s\n",c ,p,pat,str);<]*/
                    /*}else goto literal;*/

                break;
            case '*':
                /* Matches zero or more occurences of the given patterns */
                if(con.pat[0]=='(')
                {
                    /*printf("aa: c: %c p: %c\n",c ,p);*/
                    /*if(c == '\0')*/
                    /*return (struct context){.matched=1};*/
                    puts("*");
                    /*printf("%s\n",pat+1);*/
                    --con.str;
                    struct ext_glob eg=findNextS((struct ext_glob){.cur_pat=con.pat});
                    char const *pat_end=eg.end_pat > con.pat_chunk_end ?
                        eg.end_pat: con.pat_chunk_end;
                    struct context res;
                    while(!eg.ret)
                    {
                        struct context arg={.str=con.str, .pat=eg.cur_pat,
                            .pat_chunk_end=pat_end, .back_pat=back_pat,
                            .back_str=back_str, .repeat=1};
                        /*printContext(arg);*/
                        res=globmatchS(arg);
                        /*printContext(res);*/
                        if(res.matched==1)
                        {
                            /*puts("rec matched");*/
                            return res;
                        }
                        eg=findNextS(eg);
                    }
                    con.pat=pat_end;
                    printf("no match str: %s pat: %s \n",con.str, con.pat);
                    /*p=*con.pat++;*/
                    /*c=*con.str++;*/
                    printf("c: %c p: %c\n",c ,p);
                    /*goto literal;*/
                }else /* Any-length wildcard */
                {
                    if (*con.pat == '\0')   /* Optimize trailing * case */
                    {
                        puts("stern");
                        return (struct context){.matched=1, .str=con.str};
                    }
                    puts("stern");
                    back_pat = con.pat;
                    back_str = --con.str;   /* Allow zero-length match */
                }
                break;
            case '+':
                /* Matches one or more of the given patterns */
                if(con.pat[0]=='(')
                {
                    /*printf("aa: c: %c p: %c\n",c ,p);*/
                    if(c == '\0') /// TODO check this
                        return (struct context){.matched=0, .str=con.str};
                    puts("+");
                    /*printf("%s\n",pat+1);*/
                    --con.str;
                    struct ext_glob eg=findNextS((struct ext_glob){.cur_pat=con.pat});
                    char const *pat_end=eg.end_pat > con.pat_chunk_end ?
                        eg.end_pat: con.pat_chunk_end;
                    struct context res;
                    while(!eg.ret)
                    {
                        struct context arg={.str=con.str, .pat=eg.cur_pat,
                            .pat_chunk_end=pat_end, .back_pat=back_pat,
                            .back_str=back_str, .repeat=1};
                        /*printContext(arg);*/
                        res=globmatchS(arg);
                        /*printContext(res);*/
                        if(res.matched==1)
                        {
                            /*puts("rec matched");*/
                            return res;
                        }
                        eg=findNextS(eg);
                    }
                    return res;
                }else{
                    /// TODO malformed string
                    goto literal;
                }
                break;
            case '!':
                /* Matches anything except one of the given patterns */
                if(con.pat[0]=='(')
                {
                    /*printf("aa: c: %c p: %c\n",c ,p);*/
                    if(c == '\0') /// TODO check this
                        return (struct context){.matched=1, .str=con.str};
                    puts("!");
                    /*printf("%s\n",pat+1);*/
                    --con.str;
                    struct ext_glob eg=findNextS((struct ext_glob){.cur_pat=con.pat});
                    char const *pat_end=eg.end_pat > con.pat_chunk_end ?
                        eg.end_pat: con.pat_chunk_end;
                    struct context res;
                    while(!eg.ret)
                    {
                        struct context arg={.str=con.str, .pat=eg.cur_pat,
                            .pat_chunk_end=pat_end, .back_pat=back_pat,
                            .back_str=back_str, .except=1};
                        /*printContext(arg);*/
                        res=globmatchS(arg);
                        /*printContext(res);*/
                        if(res.matched==1)
                        {
                            /*puts("rec matched");*/
                            /*return res;*/
                              return (struct context){.matched=0, .str=con.str};
                            /*goto backtrack;*/
                        }
                        eg=findNextS(eg);
                    }
                    puts("ende !");
                    printContext(res);
                    con.pat=pat_end;
                    con.str=res.str;
                }else{
                    /// TODO malformed string
                    goto literal;
                }
                break;
            case '[': { /* Character class */
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
                          con.pat = class;
                      }
                      break;
            case '\\':
                      p = *con.pat++;
                      /*FALLTHROUGH*/
            default:    /* Literal character */
literal:
                      /*if ((c == p)^con.except) {*/
                      if (c == p) {
                          printf("match %c %c\n", c,p);
                          if (p == '\0')
                          {
                              puts("ende literal");
                              return (struct context){.matched=1, .str=con.str};
                          }
                          /*if(c!='\0') // TODO check this*/
                              break;
                      }else if(con.repeat && (con.pat == con.pat_chunk_end+1))
                      {
                          puts("reset pat");
                          con.pat=repeat_pat;
                          con.str=repeat_str;
                          /*printf("%c %c %s %s\n", c,p, con.pat, con.str);*/
                          if(!con.str)
                              return (struct context){.matched=0, .str=con.str};
                          break;
                      }
backtrack:
                      /*printf("%p %p\n", con.pat, con.pat_chunk_end);*/
                      if (c == '\0' || !back_pat)
                      {
                          puts("ret backtrack");
                          return (struct context){.matched=0, .str=con.str};
                      }
                          /* No point continuing */
                      /* Try again from last *, one character later in str. */
                      con.pat = back_pat;
                      con.str = ++back_str;
                      break;
        }
    }
}

const char*  globmatch(char const *pat, char const *str, int count)
{
    print_ext(pat);
    exit(0);

    const char *save_str=str;
    const char *save_pat=pat;
    /*puts("enter");*/
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
        unsigned char p = *pat++;
        if(p==')')
        {
            /*puts("klammer");*/
            count--;
            /*printf("count: %d\n",count);*/
            while(count>0)
            {
                /*printf("%c\n",p);*/
                p=*pat++;
                if(p==')')
                    count--;
                if(p=='(')
                    count++;
                /*printf("count-- %d\n",count);*/
            }
            p=*pat++;
            /*printf("char: %c p: %c, pat: %s\n",c ,p,pat);*/
            /*if(p=='\0' || c == '\0')*/
            /*{*/
                /*puts("ende klammer");*/
                /*return str-1;*/
            /*}*/
        }
        printf("p: %c , c: %c\n",p,c);
        switch (p) {
            case '|':
                printf("count %d\n",count);
                      /*puts("ende |");*/
                      while(count>0)
                      {
                          /*printf("%c\n",p);*/
                          p=*pat++;
                          if(p==')')
                              count--;
                        if(p=='(')
                            count++;
                      }
                      printf("|: %s char: %c\n",pat,c);
                      --str;
                      break;
                      /*return str;*/
            case '?':   /* Wildcard: anything but nul */
                if (c == '\0')
                    return 0;

                    if(pat[0]=='(')
                    {
                        puts("?");
                        count++;
                        /*printf("%s\n",pat+1);*/
                        /*pat=findNext(pat);*/
                        char const* fp=pat;
                        char const* ep=findPatternEnd(pat);
                        printf("pat: %s pat end %s\n",pat,ep);
                        str--;
                        char const* res=NULL;
                        while((fp=findNext(fp,ep, count)))
                        {
                            /*printf("fp: %s %d\n",fp, count);*/
                            pat=fp;
                            res=globmatch(fp,str, count);
                            /*printf("res: %s ",res?"true":"false");*/
                            /*printf("%p- %p\n", res, (save_str+strlen(save_str)));*/
                            if(res==save_str+strlen(save_str))
                                return res;
                            /*else puts("res not equal");*/
                            if(res)
                            {
                                puts("break");
                                break;
                            }
                            /*printf("char: %c str: %s pattern: %s, p %c res: %s\n\n",c,str,pat,p,res);*/
                        }
                        /*printf("fp null: %s\n",fp);*/
                        /*printf("%s %d\n",pat,count);*/
                        while(count>0)
                        {
                            p=*pat++;
                            printf("dec: %c\n",p);
                            if(p==')')
                                count--;
                            if(p=='(')
                                count++;
                        }
                        printf("char: %c p: %c, pat: %s str: %s\n",c ,p,pat,str);
                    }
                break;
            case '@':
                    if(pat[0]=='(')
                    {
                        puts("@");
                        count++;
                        /*printf("%s\n",pat+1);*/
                        /*pat=findNext(pat);*/
                        char const* fp=pat;
                        char const* ep=findPatternEnd(pat);
                        /*printf("pat: %s pat end %s\n",pat,ep);*/
                        str--;
                        char const* res=NULL;
                        while((fp=findNext(fp,ep, count)))
                        {
                            /*printf("fp: %s %d\n",fp, count);*/
                            pat=fp;
                            res=globmatch(fp,str, count);
                            /*printf("res: %s ",res?"true":"false");*/
                            /*printf("%p- %p\n", res, (save_str+strlen(save_str)));*/
                            if(res==save_str+strlen(save_str))
                                return res;
                            if(res)
                            {
                                puts("break");
                                break;
                            }
                            /*else*/
                                /*return 0;*/
                            /*printf("char: %c str: %s pattern: %s, p %c res: %s\n\n",c,str,pat,p,res);*/
                        }
                        if(!res)
                            return 0;
                        /*printf("fp null: %s\n",fp);*/
                        /*printf("%s %d\n",pat,count);*/
                        while(count>0)
                        {
                            p=*pat++;
                            printf("dec: %c\n",p);
                            if(p==')')
                                count--;
                        }
                        /*printf("char: %c p: %c, pat: %s str: %s\n",c ,p,pat,str);*/
                    }else goto literal;

                break;
            case '*':   /* Any-length wildcard */
                if (*pat == '\0')   /* Optimize trailing * case */
                {
                    puts("stern");
                    return str-1;
                }
                puts("stern");
                back_pat = pat;
                back_str = --str;   /* Allow zero-length match */
                break;
            case '[': { /* Character class */
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
                      p = *pat++;
                      /*FALLTHROUGH*/
            default:    /* Literal character */
literal:
                      if (c == p) {
                          /*printf("match %c %c\n", c,p);*/
                          if (p == '\0')
                          {
                              puts("ende literal");
                              return str-1;
                          }
                          break;
                      }
backtrack:
                      if (c == '\0' || !back_pat)
                          return 0; /* No point continuing */
                      /* Try again from last *, one character later in str. */
                      pat = back_pat;
                      str = ++back_str;
                      break;
        }
    }
}


int match(char const *pat, char const *str)
{
    /*char const* res=globmatch(pat, str,0);*/
    struct context res=globmatchS((struct context){.pat=pat, .str=str});
    if(res.matched)
        return 1;
    return 0;
}
