#ifndef __UTIL_JSON_H__
#define __UTIL_JSON_H__

int parse_integer_value1 (const json_t *J, const char *f);
int parse_integer_value2 (const json_t *J, const char *f1, const char *f2);
char *parse_string_value1 (const json_t *J, const char *f);
char *parse_string_value2 (const json_t *J, const char *f1, const char *f2);

#endif
