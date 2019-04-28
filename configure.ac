AC_INIT([cryptobot],[1.1],[])
AM_INIT_AUTOMAKE
AM_INIT_AUTOMAKE([tar-ustar subdir-objects])
AC_CANONICAL_BUILD
ac_default_prefix=/usr

echo Install autoconf-archive autoconf g++ among others

AC_PROG_CC
AC_PROG_CXX
AC_LANG([C++])
AX_CXX_COMPILE_STDCXX_11([noext],[mandatory])
AC_PROG_RANLIB
AC_CONFIG_HEADER(config.h)

PKG_PROG_PKG_CONFIG
AC_PATH_PROG(PKG_CONFIG_PATH, pkg-config, no)
if test x$PKG_CONFIG_PATH = xno; then
  AC_MSG_ERROR(Program "pkg-config" is needed. Install this first.)
fi

AC_PATH_PROG(MYSQL_CONFIG_PATH, mysql_config, no)
if test x$MYSQL_CONFIG_PATH = xno; then
AC_MSG_ERROR(Program "mysql_config" is needed. Install this first.)
fi

PKG_CHECK_MODULES(CURL, libcurl >= 7.20.0,,AC_MSG_ERROR(libcurl development version >= 7.20.0 is needed.))
AC_SUBST(CURL_CFLAGS)
AC_SUBST(CURL_LIBS)
AC_CHECK_HEADER(curl/curl.h, , AC_MSG_ERROR([Header file curl/curl.h was not found. Usually this is in libcurl4-openssl-dev or libcurl4-gnutls-dev.]))

PKG_CHECK_MODULES(OPENSSL, openssl >= 0.9.0,,AC_MSG_ERROR(openssl development version >= 0.9.0 is needed.))
AC_SUBST(OPENSSL_CFLAGS)
AC_SUBST(OPENSSL_LIBS)

PKG_CHECK_MODULES(SQLITE, sqlite3 >= 3.0,,AC_MSG_ERROR(sqlite3 development version >= 3.0 is needed.))
AC_SUBST([SQLITE_CFLAGS])
AC_SUBST([SQLITE_LIBS])

dnl No longer uses Boost libraries.
dnl AX_BOOST_BASE()
dnl AX_BOOST_SYSTEM
dnl AX_BOOST_FILESYSTEM
dnl AX_BOOST_ASIO

AC_CHECK_HEADER(execinfo.h,AC_DEFINE([HAVE_EXECINFO],[1],[Define whether execinfo.h is present]),)

AC_CHECK_HEADER(libproc.h,AC_DEFINE([HAVE_LIBPROC],[1],[Define whether libproc.h is present]),)

AC_OUTPUT(Makefile)

echo "
Cryptobot $VERSION configuration summary:

Installation prefix : ${prefix}
"
