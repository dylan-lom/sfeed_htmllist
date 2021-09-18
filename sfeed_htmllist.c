#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "util.h"

static struct feed *feeds;
static char *line;
static size_t linesize;
static time_t comparetime;
static unsigned long totalnew, total;

static void
loadfeedhead(struct feed *f)
{
    // Discard old head -- be careful this will break anything from parseline(f->head, ...)
    free(f->head);
    f->head = NULL;
    f->headts = 0;

    size_t n = 0;
    ssize_t linelen = getline(&f->head, &n, f->fp);
    if (linelen == -1) f->head = NULL; // Probably EOF

    if (f->head == NULL) return;

    char *tsend;
    if ((tsend = strchr(f->head, '\t')) != NULL) {
        size_t tslen = (tsend - f->head) + 1;
        char *ts = calloc(tslen + 1, sizeof(char));
        strlcpy(ts, f->head, tslen);
        strtotime(ts, &f->headts); // No need to check here -- ts will only be set if parsing succeeded.
    }
}

static void
printfeedhead(FILE *fpout, struct feed *f)
{
    if (f->head == NULL) return; // feed is exhausted

    char *fields[FieldLast];
    parseline(f->head, fields);

    struct tm *tm = localtime(&f->headts);
    if (tm == NULL) err(1, "localtime");

    fputs("<li>", fpout);
    fprintf(fpout, "(%04d-%02d-%02d) ", tm->tm_year + 1900, tm->tm_mon + 1,
            tm->tm_mday);
    fputs("<a href=\"", fpout);
    xmlencode(fields[FieldLink], fpout);
    fputs("\" target=\"_blank\">", fpout);
    xmlencode(fields[FieldTitle], fpout);
    fputs("</a> @ ", fpout);
    xmlencode(f->name, fpout);
    fputs("\n", fpout);
}

int main(int argc, char *argv[])
{
    FILE *fp;
    FILE *fpout = stdout;

    if (pledge("stdio rpath wpath cpath", NULL) == -1)
        err(1, "pledge");

    if (!(feeds = calloc(argc, sizeof(struct feed))))
        err(1, "calloc");

    if ((comparetime = time(NULL)) == -1)
        err(1, "time");
    /* 1 day is old news */
    comparetime -= 86400;

    if (pledge(argc == 1 ? "stdio" : "stdio rpath", NULL) == -1)
        err(1, "pledge");

    if (argc == 1) assert(0 && "TODO: Reading from stdin is not implemented yet");

    int feedsleft = argc-1;
    for (int i = 1; i < argc; i++) {
        struct feed *f = &feeds[i - 1];
        char *name = ((name = strrchr(argv[i], '/'))) ? name + 1 : argv[i];
        f->name = name;

        if (!(fp = fopen(argv[i], "r")))
            err(1, "fopen: %s", argv[i]);

        f->fp = fp;
        loadfeedhead(f);
        if (f->head == NULL) feedsleft--; // feed was empty
    }

    while (feedsleft) {
       struct feed *next = NULL;
       for (int i = 1; i < argc; i++) {
           if (!next || feeds[i].headts > next->headts) {
               next = &feeds[i];
           }
       }

       printfeedhead(fpout, next);
       loadfeedhead(next);
       if (next->head == NULL) feedsleft--; // feed is exhausted
    }
}
