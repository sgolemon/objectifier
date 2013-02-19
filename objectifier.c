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

#include "php_objectifier.h"
#include "zend_globals_macros.h"

ZEND_BEGIN_ARG_INFO(objectifier_register_arginfo, 0)
	ZEND_ARG_INFO(0, callback)
ZEND_END_ARG_INFO();

static PHP_FUNCTION(objectifier_register) {
	zval *callback;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z/", &callback) == FAILURE) {
		return;
	}

	if (!OBJTYPE_G(objectifiers)) {
		ALLOC_HASHTABLE(OBJTYPE_G(objectifiers));
		zend_hash_init(OBJTYPE_G(objectifiers), 4, NULL, ZVAL_PTR_DTOR, 0);
	}

	Z_ADDREF_P(callback);
	zend_hash_next_index_insert(OBJTYPE_G(objectifiers), &callback, sizeof(zval*), NULL);
}

static zend_function_entry php_objectifier_functions[] = {
	PHP_FE(objectifier_register,	objectifier_register_arginfo)
	{ NULL, NULL, NULL }
};

#ifndef ZEND_VM_KIND_CALL
/* ZEND_VM_KIND gets defined to this, but this doesn't get defined... go figure... */
#define ZEND_VM_KIND_CALL       1
#endif

#if (PHP_MAJOR_VERSION > 5 || PHP_MINOR_VERSION > 0) && ZEND_VM_KIND != ZEND_VM_KIND_CALL
# error "ObjectTypes support requires CALL style Zend VM"
#endif

ZEND_DECLARE_MODULE_GLOBALS(objectifier);

static inline int php_obj_decode(zend_op *opline)
{
	int ret = opline->opcode * 25;
	switch (opline->op1_type) {
		case IS_CONST:                          break;
		case IS_TMP_VAR:        ret += 5;       break;
		case IS_VAR:            ret += 10;      break;
		case IS_UNUSED:         ret += 15;      break;
		case IS_CV:             ret += 20;      break;
	}
	switch (opline->op2_type) {
		case IS_CONST:                          break;
		case IS_TMP_VAR:        ret += 1;       break;
		case IS_VAR:            ret += 2;       break;
		case IS_UNUSED:         ret += 3;       break;
		case IS_CV:             ret += 4;       break;
	}
	return ret;
}

#define PHP_OBJ_OPHANDLER_COUNT                ((164*25)+1)
#define PHP_OBJ_REPLACE_OPCODE(opname)         { int i; for(i = 0; i < 25; i++) if (php_obj_opcode_handlers[(opname*25) + i]) php_obj_opcode_handlers[(opname*25) + i] = php_obj_op_##opname; }

static opcode_handler_t *php_obj_original_opcode_handlers;
static opcode_handler_t php_obj_opcode_handlers[PHP_OBJ_OPHANDLER_COUNT];

static void php_obj_objectify(zval **arg TSRMLS_DC) {
	HashPosition pos;
	HashTable *ht = OBJTYPE_G(objectifiers);
	zval **callback;

	for(zend_hash_internal_pointer_reset_ex(ht, &pos);
		SUCCESS == zend_hash_get_current_data_ex(ht, (void**)&callback, &pos);
		zend_hash_move_forward_ex(ht, &pos)) {
		zval *retval = NULL;
		if (FAILURE == call_user_function_ex(EG(function_table), NULL, *callback, &retval, 1, &arg, 0, NULL TSRMLS_CC)) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Objectification callback failed");
			/* TODO: More descriptive error message */
			continue;
		}
		zval_ptr_dtor(arg);
		if (!retval) {
			MAKE_STD_ZVAL(retval);
			ZVAL_NULL(retval);
		}
		*arg = retval;
	}
}

static zval **php_obj_lookup_cv(const zend_execute_data *execute_data, zval ***ptr, zend_uint var TSRMLS_DC) {
	zend_compiled_variable *cv = &execute_data->op_array->vars[var];
	
	if (!EG(active_symbol_table)) {
                Z_ADDREF(EG(uninitialized_zval));
                *ptr = (zval**)EX_CV_NUM(execute_data, EG(active_op_array)->last_var + var);
                **ptr = &EG(uninitialized_zval);
                zend_error(E_NOTICE, "Undefined variable: %s", cv->name);
        } else if (zend_hash_quick_find(EG(active_symbol_table), cv->name, cv->name_len+1, cv->hash_value, (void **)ptr)==FAILURE) {
                Z_ADDREF(EG(uninitialized_zval));
                zend_hash_quick_update(EG(active_symbol_table), cv->name, cv->name_len+1, cv->hash_value, &EG(uninitialized_zval_ptr), sizeof(zval *), (void **)ptr);
                zend_error(E_NOTICE, "Undefined variable: %s", cv->name);
        }
        return *ptr;
}

static int ZEND_FASTCALL php_obj_op_ZEND_INIT_METHOD_CALL(ZEND_OPCODE_HANDLER_ARGS) {
	zend_op *opline = execute_data->opline;

	if (!OBJTYPE_G(objectifiers) || !zend_hash_num_elements(OBJTYPE_G(objectifiers))) {
		/* No objectifiers available, shortcut out of this */
		return php_obj_original_opcode_handlers[php_obj_decode(opline)](ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);
	}

	if (opline->op1_type == IS_VAR) {
		zval **ppzv = EX_TMP_VAR(execute_data, opline->op1.var)->var.ptr_ptr;
		if (*ppzv) {
			php_obj_objectify(ppzv TSRMLS_CC);
			EX_TMP_VAR(execute_data, opline->op1.var)->var.ptr = *ppzv;
		}
	} else if (opline->op1_type == IS_CV) {
		zval ***v = EX_CV_NUM(execute_data, opline->op1.var);
		zval *newzv;
		if (!*v) {
			*v = php_obj_lookup_cv(execute_data, v, opline->op1.var TSRMLS_CC);
		}
		if (*v && **v) {
			php_obj_objectify(*v TSRMLS_CC);
		}
	} else if (opline->op1_type == IS_TMP_VAR) {
		zval *tmp, **ptmp = &tmp;
		MAKE_STD_ZVAL(tmp);
		*tmp = EX_TMP_VAR(execute_data, opline->op1.var)->tmp_var;
		php_obj_objectify(ptmp TSRMLS_CC);
		if (ptmp != &tmp) {
			EX_TMP_VAR(execute_data, opline->op1.var)->tmp_var = **ptmp;
			efree(*ptmp);
		}
	}

	/* Now that the opline's been rewritten, just run with it */
	return php_obj_original_opcode_handlers[php_obj_decode(opline)](ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);
}

static PHP_MINIT_FUNCTION(objectifier) {
	memcpy(php_obj_opcode_handlers, zend_opcode_handlers, sizeof(php_obj_opcode_handlers));
	php_obj_original_opcode_handlers = zend_opcode_handlers;
	zend_opcode_handlers = php_obj_opcode_handlers;
	PHP_OBJ_REPLACE_OPCODE(ZEND_INIT_METHOD_CALL)
	return SUCCESS;
}

static PHP_RINIT_FUNCTION(objectifier) {
	OBJTYPE_G(objectifiers) = NULL;
	return SUCCESS;
}

static PHP_RSHUTDOWN_FUNCTION(objectifier) {
	if (OBJTYPE_G(objectifiers)) {
		zend_hash_destroy(OBJTYPE_G(objectifiers));
		FREE_HASHTABLE(OBJTYPE_G(objectifiers));
	}
	return SUCCESS;
}

zend_module_entry objectifier_module_entry = {
	STANDARD_MODULE_HEADER,
	PHP_OBJECTIFIER_EXTNAME,
	php_objectifier_functions,
	PHP_MINIT(objectifier),
	NULL, /* MSHUTDOWN */
	PHP_RINIT(objectifier),
	PHP_RSHUTDOWN(objectifier),
	NULL, /* MINFO */
	PHP_OBJECTIFIER_VERSION,
	PHP_MODULE_GLOBALS(objectifier),
	NULL, /* GINIT */
	NULL, /* GSHUTDOWN */
	NULL, /* RPOSTSHUTDOWN */
	STANDARD_MODULE_PROPERTIES_EX
};

#ifdef COMPILE_DL_OBJECTIFIER
ZEND_GET_MODULE(objectifier)
#endif

