AC_PREREQ([2.68])
AC_INIT([vkconcli], [0.1])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([m], [sqrt])
AC_CHECK_LIB([curl], [curl_easy_init],,AC_MSG_ERROR([curl library missing]))
AC_CHECK_LIB([jansson], [json_loadb],,AC_MSG_ERROR([jansson library missing]))
AC_CHECK_LIB([sqlite3], [sqlite3_open],,AC_MSG_ERROR([sqlite3 library missing]))
AC_CHECK_LIB([config], [config_init],,AC_MSG_ERROR([config library missing]))
AC_CHECK_LIB([SDL], [SDL_Init])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h malloc.h stdlib.h string.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([getpass memset select strdup strndup])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
