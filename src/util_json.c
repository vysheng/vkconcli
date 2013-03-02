#include <jansson.h>
#include "util_json.h"
#include <string.h>


int parse_integer_value1 (const json_t *J, const char *f) {
  return json_object_get (J, f) ? json_integer_value (json_object_get (J, f)) : 0;
}

int parse_integer_value2 (const json_t *J, const char *f1, const char *f2) {
  json_t *J1;
  if (!(J1 = json_object_get (J, f1))) { return 0; }
  return parse_integer_value1 (J1, f2);
}

char *parse_string_value1 (const json_t *J, const char *f) {
  return json_object_get (J, f) ? strdup (json_string_value (json_object_get (J, f))) : strdup ("");
}

char *parse_string_value2 (const json_t *J, const char *f1, const char *f2) {
  json_t *J1;
  if (!(J1 = json_object_get (J, f1))) { return strdup (""); }
  return parse_string_value1 (J1, f2);
}
