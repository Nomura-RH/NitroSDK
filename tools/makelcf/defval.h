#ifndef DEFVAL_H_
#define DEFVAL_H_

#define DEFVAL_DEFAULT_BUFFER_SIZE (1024)

BOOL AddDefValFromFile(char *filename);
void AddDefVal(char *opt);
const char *SearchDefVal(const char *name);
const char *SearchDefValCleaned(const char *name);
char *ResolveDefVal(char *str);
char *ResolveStringModifier(const char *in_value, char modifier);

#endif // DEFVAL_H_
