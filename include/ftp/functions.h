#ifndef OPF_FUNCTIONS_H
#define OPF_FUNCTIONS_H

#define low(a,b)		(a > b ? b : a)
#define high(a,b)		(a > b ? a : b)
#define strcmp2(a,b)		(a[0] == b[0] ? strcmp(a, b) : -1)
#define ftpresp(str,code,msg)	sprintf(str, "%i %s\r\n", code, msg) // plain response
#define is_empty(str)		(str[0] == '\0')
#define fis_dir(entry)		S_ISDIR(entry.st_mode)
#define ftp_port(p1,p2)		(p1 << 8 | p2)

void abspath(const char *relpath, const char *cwd, char *abspath);
int exists(const char *path);
int is_dir(const char *path);
int strpos(const char *haystack, int needle);
int strsplit(const char *str, char *left, int lmaxlen, char *right, int rmaxlen);
void strreplace(char *str, int oldc, int newc);
void strtoupper(char *str);

#endif /* OPF_FUNCTIONS_H */
