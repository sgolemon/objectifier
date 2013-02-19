dnl $Id: config.m4 204999 2006-01-12 05:52:15Z pollita $
dnl config.m4 for extension objectifier

PHP_ARG_ENABLE(objectifier, whether to enable objectifier support,
[  --enable-objectifier        Enable objectifier support])

if test "$PHP_OBJECTIFIER" != "no"; then
  PHP_NEW_EXTENSION(objectifier, objectifier.c, $ext_shared)
fi
