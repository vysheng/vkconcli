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

EXTRA_LIBS=""

AC_MSG_CHECKING([Checking for libconfig])
AC_ARG_ENABLE(libconfig,[--enable-libconfig/--disable-libconfig],
  [ 
  if test "x$enableval" = "xno" ; then
    AC_MSG_RESULT([disabled])
  else
    AC_MSG_RESULT([enabled])
    AC_CHECK_LIB([config],[config_init],AC_DEFINE([ENABLE_LIBCONFIG]),AC_MSG_ERROR([No libconfig found. Try --disable-libconfig]))
    [EXTRA_LIBS+=" -lconfig" ; ]
  fi 
  ],[
    AC_MSG_RESULT([enabled])
    AC_CHECK_LIB([config],[config_init],AC_DEFINE([ENABLE_LIBCONFIG]),AC_MSG_ERROR([No libconfig found. Try --disable-libconfig]))
    [EXTRA_LIBS+=" -lconfig" ; ]
  ])
    
AC_ARG_ENABLE(audio,[--enable-audio/--disable-audio],
  [ 
  if test "x$enableval" = "xno" ; then
    AC_MSG_RESULT([disabled])
  else
    AC_MSG_RESULT([enabled])
    AC_CHECK_LIB([SDL],[SDL_Init],AC_DEFINE([ENABLE_AUDIO]),AC_MSG_ERROR([No libSDL found. Try --disable-audio]))
    [ EXTRA_LIBS+=" -lSDL" ;]
  fi 
  ],[
    AC_MSG_RESULT([enabled])
    AC_CHECK_LIB([SDL],[SDL_Init],AC_DEFINE([ENABLE_AUDIO]),AC_MSG_ERROR([No libSDL found. Try --disable-audio]))
    [ EXTRA_LIBS+=" -lSDL" ; ]
  ])
    
AC_ARG_ENABLE(readline,[--enable-readline/--disable-readline],
  [ 
  if test "x$enableval" = "xno" ; then
    AC_MSG_RESULT([disabled])
  else
    AC_MSG_RESULT([readline enabled])
    AC_CHECK_LIB([readline],[readline],AC_DEFINE([ENABLE_READLINE]),AC_MSG_ERROR([No libreadline found. Try --disable-readline]))
    [ EXTRA_LIBS+=" -lreadline" ;]
  fi 
  ],[
    AC_MSG_RESULT([readline enabled])
    AC_CHECK_LIB([readline],[readline],AC_DEFINE([ENABLE_READLINE]),AC_MSG_ERROR([No libreadline found. Try --disable-readline]))
    [ EXTRA_LIBS+=" -lreadline" ;]
  ])
    
# Checks for header files.
AC_CHECK_HEADERS([fcntl.h malloc.h stdlib.h string.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([getpass memset select strdup strndup])

AC_SUBST(EXTRA_LIBS)
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
