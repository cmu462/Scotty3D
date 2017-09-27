#ifndef GETOPT_H
#define GETOPT_H

#ifdef __cplusplus
extern "C" {
#endif

extern int opterr,   /* if error message should be printed */
    optind,          /* index into parent argv vector */
    optopt,          /* character checked for validity */
    optreset;        /* reset getopt */
extern char *optarg; /* argument associated with option */

int getopt(int nargc, char *const nargv[], const char *ostr);

#ifdef __cplusplus
}
#endif

#endif