/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2006 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.0 of the PHP license,       |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_0.txt.                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Sara Golemon <pollita@php.net>                               |
  +----------------------------------------------------------------------+
*/

/* $Id: php_objectifier.h 256477 2008-03-31 10:01:43Z sfox $ */

#ifndef PHP_OBJECTIFIER_H
#define PHP_OBJECTIFIER_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php.h"

#define PHP_OBJECTIFIER_EXTNAME	"objectifier"
#define PHP_OBJECTIFIER_VERSION	"0.0.0-dev"

ZEND_BEGIN_MODULE_GLOBALS(objectifier)
	HashTable *objectifiers;
ZEND_END_MODULE_GLOBALS(objectifier)

extern ZEND_DECLARE_MODULE_GLOBALS(objectifier);

#ifdef ZTS
# define OBJTYPE_G(v) TSRMG(objectifier_globals_id, zend_objectifier_globals *, v)
#else
# define OBJTYPE_G(v) (objecttype_globals.v)
#endif

extern zend_module_entry objectifier_module_entry;
#define phpext_objectifier_ptr &objectifier_module_entry

#endif


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
