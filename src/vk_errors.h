#ifndef __VK_ERRORS_H__
#define __VK_ERRORS_H__

#define ERROR_COMMAND_LINE 1
#define ERROR_CURL_INIT 2
#define ERROR_NET 3
#define ERROR_PARSE_ANSWER 4
#define ERROR_UNEXPECTED_ANSWER 5
#define ERROR_NO_ACCESS_TOKEN 6
#define ERROR_SQLITE 7
#define ERROR_NO_DB 8
#define ERROR_BUFFER_OVERFLOW 9
#define ERROR_NO_CONF 10
#define ERROR_CURL 11
#define ERROR_SYS 12
#define ERROR_SDL 13
#define ERROR_CONFIG 14
#define ERROR_IN 15
#define ERROR_NOT_COMPILED 16

#define _FATAL_ERROR -9
#define _ERROR -1

#define VK_TRY(f) { int _r = f; if (_r < 0) { return _r;}}
#define VK_TRY_ANS(a,f) a = f; if (a < 0) { return a; }

#define UNUSED __attribute__ ((unused))
void vk_error (int error_code, const char *format, ...) __attribute__ ((format(printf,2,3)));
void vk_log (int level, const char *format, ...) __attribute__ ((format(printf,2,3)));
#endif
