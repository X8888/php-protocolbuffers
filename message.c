#include "php_protocol_buffers.h"
#include "message.h"
#include "ext/standard/php_smart_str.h"
#include "unknown_field_set.h"
#include "extension_registry.h"

enum ProtocolBuffers_MagicMethod {
	MAGICMETHOD_GET    = 1,
	MAGICMETHOD_SET    = 2,
	MAGICMETHOD_APPEND = 3,
	MAGICMETHOD_CLEAR  = 4,
	MAGICMETHOD_HAS    = 5,
};

static zend_object_handlers php_protocolbuffers_message_object_handlers;

#define PHP_PB_MESSAGE_CHECK_SCHEME2(instance, container, proto) \
	{\
		zend_class_entry *__ce;\
		int __err;\
		\
		__ce  = Z_OBJCE_P(instance);\
		__err = pb_get_scheme_container(__ce->name, __ce->name_length, container, proto TSRMLS_CC);\
		if (__err) {\
			if (EG(exception)) {\
				return;\
			} else {\
				/* TODO: improve displaying error message */\
				php_error_docref(NULL TSRMLS_CC, E_ERROR, "pb_get_scheme_cointainer failed. %s does not have getDescriptor method", __ce->name);\
				return;\
			}\
		}\
	}


#define PHP_PB_MESSAGE_CHECK_SCHEME PHP_PB_MESSAGE_CHECK_SCHEME2(instance, &container, proto)

static void php_protocolbuffers_message_free_storage(php_protocolbuffers_message *object TSRMLS_DC)
{
	zend_object_std_dtor(&object->zo TSRMLS_CC);
	efree(object);
}

zend_object_value php_protocolbuffers_message_new(zend_class_entry *ce TSRMLS_DC)
{
	zend_object_value retval;
	PHP_PROTOCOLBUFFERS_STD_CREATE_OBJECT(php_protocolbuffers_message);

	object->max    = 0;
	object->offset = 0;

	retval.handlers = &php_protocolbuffers_message_object_handlers;

	return retval;
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_pb_message___construct, 0, 0, 0)
	ZEND_ARG_INFO(0, params)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_pb_message_serialize_to_string, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_pb_message_parse_from_string, 0, 0, 1)
	ZEND_ARG_INFO(0, string)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_pb_message___call, 0, 0, 2)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, args)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_pb_message_merge_from, 0, 0, 1)
	ZEND_ARG_INFO(0, message)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_pb_message_discard_unknown_fields, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_pb_message_clear, 0, 0, 0)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_pb_message_clear_alls, 0, 0, 0)
	ZEND_ARG_INFO(0, clear_unknown_fields)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_pb_message_has, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_pb_message_get, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_pb_message_set, 0, 0, 2)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_pb_message_append, 0, 0, 2)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_pb_message_get_extension, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_pb_message_has_extension, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_pb_message_set_extension, 0, 0, 2)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_pb_message_clear_extension, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

static void php_protocolbuffers_get_hash(pb_scheme_container *container, pb_scheme *scheme, zval *object, char **name, int *name_len, HashTable **ht TSRMLS_DC)
{
	char *n;
	int n_len;
	HashTable *htt = NULL;

	if (container->use_single_property > 0) {
		n     = container->single_property_name;
		n_len = container->single_property_name_len;

		if (zend_hash_find(Z_OBJPROP_P(object), n, n_len, (void **)&htt) == FAILURE) {
			return;
		}

		n = scheme->name;
		n_len = scheme->name_len;
	} else {
		htt = Z_OBJPROP_P(object);

		n = scheme->mangled_name;
		n_len = scheme->mangled_name_len;
	}

	name = &n;
	name_len = &n_len;
	*ht = htt;

}

static void php_protocolbuffers_message_merge_from(pb_scheme_container *container, HashTable *htt, HashTable *hts TSRMLS_DC)
{
	pb_scheme *scheme;

	int i = 0;
	for (i = 0; i < container->size; i++) {
		zval **tmp = NULL;
		char *name;
		int name_len;

		scheme = &(container->scheme[i]);

		if (container->use_single_property > 0) {
			name = scheme->name;
			name_len = scheme->name_len;
		} else {
			name = scheme->mangled_name;
			name_len = scheme->mangled_name_len;
		}

		if (zend_hash_find(hts, name, name_len, (void **)&tmp) == SUCCESS) {
			zval *val;

			switch (Z_TYPE_PP(tmp)) {
			case IS_NULL:
			break;
			case IS_STRING:
				MAKE_STD_ZVAL(val);
				ZVAL_STRINGL(val, Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp), 1);

				Z_ADDREF_P(val);
				zend_hash_update(htt, name, name_len, (void **)&val, sizeof(zval *), NULL);
				zval_ptr_dtor(&val);
				break;
			case IS_LONG:
				MAKE_STD_ZVAL(val);
				ZVAL_LONG(val, Z_LVAL_PP(tmp));

				Z_ADDREF_P(val);
				zend_hash_update(htt, name, name_len, (void **)&val, sizeof(zval *), NULL);
				zval_ptr_dtor(&val);
			break;
			case IS_DOUBLE:
				MAKE_STD_ZVAL(val);
				ZVAL_DOUBLE(val, Z_DVAL_PP(tmp));

				Z_ADDREF_P(val);
				zend_hash_update(htt, name, name_len, (void **)&val, sizeof(zval *), NULL);
				zval_ptr_dtor(&val);
			break;
			case IS_OBJECT:
			{
				zval **tmp2 = NULL;

				if (zend_hash_find(htt, name, name_len, (void **)&tmp2) == SUCCESS) {
					if (Z_TYPE_PP(tmp2) == IS_OBJECT) {
						char *n;
						int n_len;
						pb_scheme_container *c;
						HashTable *p = NULL, *_htt = NULL, *_hts = NULL;

						PHP_PB_MESSAGE_CHECK_SCHEME2(*tmp, &c, p)
						php_protocolbuffers_get_hash(c, c->scheme, *tmp, &n, &n_len, &_htt TSRMLS_CC);
						php_protocolbuffers_get_hash(c, c->scheme, *tmp2, &n, &n_len, &_hts TSRMLS_CC);

						php_protocolbuffers_message_merge_from(c, _htt, _hts TSRMLS_CC);
					} else {
						MAKE_STD_ZVAL(val);
						ZVAL_ZVAL(val, *tmp, 1, 0);

						Z_ADDREF_P(val);
						zend_hash_update(htt, name, name_len, (void **)&val, sizeof(zval *), NULL);
						zval_ptr_dtor(&val);
					}
				} else {
					MAKE_STD_ZVAL(val);
					ZVAL_ZVAL(val, *tmp, 1, 0);

					Z_ADDREF_P(val);
					zend_hash_update(htt, name, name_len, (void **)&val, sizeof(zval *), NULL);
					zval_ptr_dtor(&val);
				}
			}
			break;
			case IS_ARRAY: {
				zval **tmp2 = NULL;

				if (zend_hash_find(htt, name, name_len, (void **)&tmp2) == SUCCESS) {
					if (Z_TYPE_PP(tmp2) == IS_ARRAY) {
						php_array_merge(Z_ARRVAL_PP(tmp2), Z_ARRVAL_PP(tmp), 1 TSRMLS_CC);
					} else {
						MAKE_STD_ZVAL(val);
						ZVAL_ZVAL(val, *tmp, 1, 0);

						Z_ADDREF_P(val);
						zend_hash_update(htt, name, name_len, (void **)&val, sizeof(zval *), NULL);
						zval_ptr_dtor(&val);
					}
				} else {
					MAKE_STD_ZVAL(val);
					ZVAL_ZVAL(val, *tmp, 1, 0);

					Z_ADDREF_P(val);
					zend_hash_update(htt, name, name_len, (void **)&val, sizeof(zval *), NULL);
					zval_ptr_dtor(&val);
				}
			}
			break;
			default:
				zend_error(E_NOTICE, "mergeFrom: zval type %d is not supported.", Z_TYPE_PP(tmp));
			}
		}

	}
}

static inline void php_pb_typeconvert(pb_scheme *scheme, zval *vl TSRMLS_DC)
{
	// TODO: check message
#define TYPE_CONVERT(vl) \
		switch (scheme->type) {\
			case TYPE_STRING:\
				if (Z_TYPE_P(vl) != IS_STRING) {\
					convert_to_string(vl);\
				}\
			break;\
			case TYPE_SINT32:\
			case TYPE_INT32:\
			case TYPE_UINT32:\
				if (Z_TYPE_P(vl) != IS_LONG) {\
					convert_to_long(vl);\
				}\
			break;\
			case TYPE_SINT64:\
			case TYPE_INT64:\
			case TYPE_UINT64:\
				if (Z_TYPE_P(vl) != IS_LONG) {\
					convert_to_long(vl);\
				}\
			break;\
			case TYPE_DOUBLE:\
			case TYPE_FLOAT:\
				if (Z_TYPE_P(vl) != IS_DOUBLE) {\
					convert_to_double(vl);\
				}\
			break;\
			case TYPE_BOOL:\
				if (Z_TYPE_P(vl) != IS_BOOL) {\
					convert_to_boolean(vl);\
				}\
			break;\
		}\


	if (scheme->repeated < 1) {
		TYPE_CONVERT(vl)
	} else {
		zval **entry;
		HashPosition pos;

		if (Z_TYPE_P(vl) == IS_ARRAY) {
			zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(vl), &pos);
			while (zend_hash_get_current_data_ex(Z_ARRVAL_P(vl), (void **)&entry, &pos) == SUCCESS) {
				TYPE_CONVERT(*entry)

				zend_hash_move_forward_ex(Z_ARRVAL_P(vl), &pos);
			}
		}
	}

#undef TYPE_CONVERT
}


static pb_scheme *php_protocolbuffers_message_get_scheme_by_name(pb_scheme_container *container, char *name, int name_len, char *name2, int name2_len)
{
	int i = 0;
	pb_scheme *scheme = NULL;

	for (i = 0; i < container->size; i++) {
		scheme = &container->scheme[i];

		if (strcmp(scheme->name, name) == 0) {
			break;
		}
		if (name2 != NULL) {
			if (scheme->magic_type == 1 && strcasecmp(scheme->original_name, name2) == 0) {
				break;
			}
		}
		scheme = NULL;
	}

	return scheme;
}

static void php_protocolbuffers_message_get_hash_table_by_container(pb_scheme_container *container, pb_scheme *scheme, zval *instance, HashTable **hash, char **name, int *name_len TSRMLS_DC)
{
	char *n = NULL;
	int n_len = 0;
	HashTable *htt = NULL;

	if (container->use_single_property > 0) {
		n     = container->single_property_name;
		n_len = container->single_property_name_len;

		if (zend_hash_find(Z_OBJPROP_P(instance), n, n_len, (void **)&htt) == FAILURE) {
			return;
		}
	} else {
		htt = Z_OBJPROP_P(instance);

		n = scheme->mangled_name;
		n_len = scheme->mangled_name_len;
	}

	*hash = htt;
	*name = n;
	*name_len = n_len;
}

static void php_protocolbuffers_message_get(INTERNAL_FUNCTION_PARAMETERS, zval *instance, pb_scheme_container *container, char *name, int name_len, char *name2, int name2_len)
{
	char *n;
	int n_len;
	HashTable *htt = NULL;
	pb_scheme *scheme;
	zval **e;

	scheme = php_protocolbuffers_message_get_scheme_by_name(container, name, name_len, name2, name2_len);
	if (scheme == NULL) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "%s does not find", name);
		return;
	}

	if (scheme->is_extension) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "get method can't use for extension value", name);
		return;
	}

	php_protocolbuffers_message_get_hash_table_by_container(container, scheme, instance, &htt, &n, &n_len TSRMLS_CC);

	if (zend_hash_find(htt, n, n_len, (void **)&e) == SUCCESS) {
		RETURN_ZVAL(*e, 1, 0);
	}
}

static void php_protocolbuffers_message_set(INTERNAL_FUNCTION_PARAMETERS, zval *instance, pb_scheme_container *container, char *name, int name_len, char *name2, int name2_len, zval *value)
{
	pb_scheme *scheme;
	zval **e;
	HashTable *htt;
	char *n;
	int n_len;

	scheme = php_protocolbuffers_message_get_scheme_by_name(container, name, name_len, name2, name2_len);
	if (scheme == NULL) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "%s does not find", name);
		return;
	}

	if (scheme->is_extension) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "set method can't use for extension value", name);
		return;
	}

	if (scheme->ce != NULL) {
		if (scheme->ce != Z_OBJCE_P(value)) {
			zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "expected %s class. given %s class", scheme->ce->name, Z_OBJCE_P(value)->name);
			return;
		}
	}

	php_protocolbuffers_message_get_hash_table_by_container(container, scheme, instance, &htt, &n, &n_len TSRMLS_CC);

	if (zend_hash_find(htt, n, n_len, (void **)&e) == SUCCESS) {
		zval *vl;

		if (container->use_single_property > 0) {
			MAKE_STD_ZVAL(vl);
			ZVAL_ZVAL(vl, *e, 1, 1);
			php_pb_typeconvert(scheme, vl TSRMLS_CC);

			zend_hash_update(htt, scheme->name, scheme->name_len, (void **)&vl, sizeof(zval *), NULL);
		} else {
			zval *garvage = *e;

			MAKE_STD_ZVAL(vl);
			ZVAL_ZVAL(vl, value, 1, 1);

			php_pb_typeconvert(scheme, vl TSRMLS_CC);
			*e = vl;
			zval_ptr_dtor(&garvage);
		}
	}
}

static void php_protocolbuffers_message_clear(INTERNAL_FUNCTION_PARAMETERS, zval *instance, pb_scheme_container *container, char *name, int name_len, char *name2, int name2_len)
{
	pb_scheme *scheme;
	zval **e;
	HashTable *htt;
	char *n;
	int n_len;

	scheme = php_protocolbuffers_message_get_scheme_by_name(container, name, name_len, name2, name2_len);
	if (scheme == NULL) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "%s does not find", name);
		return;
	}

	if (scheme->is_extension) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "clear method can't use for extension value", name);
		return;
	}
	php_protocolbuffers_message_get_hash_table_by_container(container, scheme, instance, &htt, &n, &n_len TSRMLS_CC);

	if (zend_hash_find(htt, n, n_len, (void **)&e) == SUCCESS) {
		zval *vl;

		if (container->use_single_property > 0) {
			MAKE_STD_ZVAL(vl);
			if (scheme->repeated > 0) {
				array_init(vl);
			} else {
				ZVAL_NULL(vl);
			}
			php_pb_typeconvert(scheme, vl TSRMLS_CC);

			zend_hash_update(htt, scheme->name, scheme->name_len, (void **)&vl, sizeof(zval *), NULL);
		} else {
			zval *garvage = *e;

			MAKE_STD_ZVAL(vl);
			if (scheme->repeated > 0) {
				array_init(vl);
			} else {
				ZVAL_NULL(vl);
			}

			*e = vl;
			zval_ptr_dtor(&garvage);
		}
	}
}

static void php_protocolbuffers_message_append(INTERNAL_FUNCTION_PARAMETERS, zval *instance, pb_scheme_container *container, char *name, int name_len, char *name2, int name2_len, zval *value)
{
	pb_scheme *scheme;
	zval **e;
	HashTable *htt;
	char *n;
	int n_len;

	scheme = php_protocolbuffers_message_get_scheme_by_name(container, name, name_len, name2, name2_len);
	if (scheme == NULL) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "%s does not find", name);
		return;
	}

	if (scheme->is_extension) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "set method can't use for extension value", name);
		return;
	}

	if (scheme->repeated < 1) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "append method can't use for non repeated value", name);
		return;
	}

	if (scheme->ce != NULL) {
		if (scheme->ce != Z_OBJCE_P(value)) {
			zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "expected %s class. given %s class", scheme->ce->name, Z_OBJCE_P(value)->name);
			return;
		}
	}

	php_protocolbuffers_message_get_hash_table_by_container(container, scheme, instance, &htt, &n, &n_len TSRMLS_CC);

	if (container->use_single_property > 0 && zend_hash_exists(htt, n, n_len) == 0) {
		zend_error(E_ERROR, "not initialized");
		return;
	}

	if (zend_hash_find(htt, n, n_len, (void **)&e) == SUCCESS) {
		zval *nval = NULL, *val = NULL;
		int flag = 0;

		if (Z_TYPE_PP(e) != IS_ARRAY) {
			MAKE_STD_ZVAL(nval);
			array_init(nval);
			flag = 1;
		} else {
			nval = *e;
		}

		MAKE_STD_ZVAL(val);
		ZVAL_ZVAL(val, value, 1, 1);

		Z_ADDREF_P(nval);
		zend_hash_next_index_insert(Z_ARRVAL_P(nval), &val, sizeof(zval *), NULL);
		zend_hash_update(htt, n, n_len, (void **)&nval, sizeof(zval *), NULL);

		if (flag == 1) {
			zval_ptr_dtor(&nval);
		}
	}
}

static void php_protocolbuffers_message_has(INTERNAL_FUNCTION_PARAMETERS, zval *instance, pb_scheme_container *container, char *name, int name_len, char *name2, int name2_len)
{
	char *n;
	int n_len;
	HashTable *htt = NULL;
	pb_scheme *scheme;
	zval **e;

	scheme = php_protocolbuffers_message_get_scheme_by_name(container, name, name_len, name2, name2_len);
	if (scheme == NULL) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "%s does not find", name);
		return;
	}

	if (scheme->is_extension) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "get method can't use for extension value", name);
		return;
	}

	php_protocolbuffers_message_get_hash_table_by_container(container, scheme, instance, &htt, &n, &n_len TSRMLS_CC);
	if (zend_hash_find(htt, n, n_len, (void **)&e) == SUCCESS) {
		if (Z_TYPE_PP(e) == IS_NULL) {
			RETURN_FALSE;
		} else if (Z_TYPE_PP(e) == IS_ARRAY) {
			if (zend_hash_num_elements(Z_ARRVAL_PP(e)) < 1) {
				RETURN_FALSE;
			} else {
				RETURN_TRUE;
			}
		} else {
			RETURN_TRUE;
		}
	}
}

/* {{{ proto ProtocolBuffersMessage ProtocolBuffersMessage::__construct([array $params])
*/
PHP_METHOD(protocolbuffers_message, __construct)
{
	zval *instance = getThis();
	zval *params = NULL;
	HashTable *proto = NULL;
	pb_scheme_container *container = NULL;
	pb_scheme *scheme = NULL;
	HashTable *htt = NULL;
	php_protocolbuffers_message *message;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"|a", &params) == FAILURE) {
		return;
	}

	if (params != NULL) {
		int i = 0;

		PHP_PB_MESSAGE_CHECK_SCHEME
		message = PHP_PROTOCOLBUFFERS_GET_OBJECT(php_protocolbuffers_message, instance);

		if (container->use_single_property > 0) {
			zval *z = NULL;

			MAKE_STD_ZVAL(z);
			array_init(z);

			Z_ADDREF_P(z);
			zend_hash_update(Z_OBJPROP_P(instance), container->single_property_name, container->single_property_name_len+1, (void **)&z, sizeof(zval), NULL);
			htt = Z_ARRVAL_P(z);
		}

		for (i = 0; i < container->size; i++) {
			zval **tmp = NULL;
			zval **e = NULL;
			zval *value = NULL;

			scheme = &container->scheme[i];

			if (zend_hash_find(Z_ARRVAL_P(params), scheme->name, scheme->name_len, (void **)&tmp) == SUCCESS) {
				if (container->use_single_property > 0) {
					if (scheme->type == TYPE_MESSAGE) {
						if (Z_TYPE_PP(tmp) == IS_ARRAY) {
							zval *p;
							MAKE_STD_ZVAL(value);
							MAKE_STD_ZVAL(p);

							ZVAL_ZVAL(p, *tmp, 1, 0);

							object_init_ex(value, scheme->ce);
							php_pb_properties_init(value, scheme->ce TSRMLS_CC);
							zend_call_method_with_1_params(&value, scheme->ce, NULL, ZEND_CONSTRUCTOR_FUNC_NAME, NULL, p);
						} else {
							if (Z_TYPE_PP(tmp) == IS_OBJECT && Z_OBJCE_PP(tmp) == scheme->ce) {
								MAKE_STD_ZVAL(value);
								ZVAL_ZVAL(value, *tmp, 0, 1);
							} else {
								zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "passed variable is not expected message class. expected %s, but %s given", scheme->ce->name, Z_OBJCE_PP(tmp)->name);
								return;
							}
						}
					} else {
						MAKE_STD_ZVAL(value);
						ZVAL_ZVAL(value, *tmp, 0, 1);
						php_pb_typeconvert(scheme, value TSRMLS_CC);

						Z_ADDREF_P(value);
					}
					zend_hash_update(htt, scheme->name, scheme->name_len, (void **)&value, sizeof(zval *), NULL);
				} else {
					if (zend_hash_find(Z_OBJPROP_P(instance), scheme->mangled_name, scheme->mangled_name_len, (void **)&e) == SUCCESS) {
						zval *garvage = NULL;

						garvage = *e;
						if (scheme->type == TYPE_MESSAGE) {
							if (Z_TYPE_PP(tmp) == IS_ARRAY) {
								zval *p = NULL;

								MAKE_STD_ZVAL(value);
								MAKE_STD_ZVAL(p);
								ZVAL_ZVAL(p, *tmp, 1, 0);
								Z_SET_REFCOUNT_P(p, 1);

								object_init_ex(value, scheme->ce);
								php_pb_properties_init(value, scheme->ce TSRMLS_CC);

								zend_call_method_with_1_params(&value, scheme->ce, NULL, ZEND_CONSTRUCTOR_FUNC_NAME, NULL, *tmp);
								zval_ptr_dtor(&p);
							} else {
								if (Z_TYPE_PP(tmp) == IS_OBJECT && Z_OBJCE_PP(tmp) == scheme->ce) {
									MAKE_STD_ZVAL(value);
									ZVAL_ZVAL(value, *tmp, 0, 1);
								} else {
									MAKE_STD_ZVAL(value); // probably missed something
									zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "passed variable is not expected message class. expected %s, but %s given", scheme->ce->name, Z_OBJCE_PP(tmp)->name);
									return;
								}
							}
						} else {
							MAKE_STD_ZVAL(value);
							ZVAL_ZVAL(value, *tmp, 1, 0);

							Z_SET_REFCOUNT_P(value, 1);
							php_pb_typeconvert(scheme, value TSRMLS_CC);
						}

						*e = value;
						zval_ptr_dtor(&garvage);
					}
				}
			}

		}
	}
}
/* }}} */

/* {{{ proto mixed ProtocolBuffersMessage::serializeToString()
*/
PHP_METHOD(protocolbuffers_message, serializeToString)
{
	zval *instance = getThis();

	php_protocolbuffers_encode(INTERNAL_FUNCTION_PARAM_PASSTHRU, Z_OBJCE_P(instance), instance);
}
/* }}} */

/* {{{ proto mixed ProtocolBuffersMessage::parseFromString()
*/
PHP_METHOD(protocolbuffers_message, parseFromString)
{
#if (PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 3)
	/* I tried to lookup current scope from EG(current_execute_data). but it doesn't have current scope. we can't do anymore */
	zend_throw_exception_ex(spl_ce_RuntimeException, 0 TSRMLS_CC, "ProtocolBuffersMessage::parseFromString can't work under PHP 5.3. please use ProtocolBuffers:decode directly");
	return;
#else
	const char *data;
	int data_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"s", &data, &data_len) == FAILURE) {
		return;
	}
	if (EG(called_scope)) {
		php_protocolbuffers_decode(INTERNAL_FUNCTION_PARAM_PASSTHRU, data, data_len, EG(called_scope)->name, EG(called_scope)->name_length);
	} else {
		zend_throw_exception_ex(spl_ce_RuntimeException, 0 TSRMLS_CC, "Missing EG(current_scope). this is bug");
	}

#endif
}
/* }}} */

/* {{{ proto mixed ProtocolBuffersMessage::mergeFrom($message)
*/
PHP_METHOD(protocolbuffers_message, mergeFrom)
{
	zval *instance = getThis();
	zval *object;
	pb_scheme_container *container = NULL;
	HashTable *proto = NULL;
	char *n;
	int n_len;
	HashTable *htt = NULL, *hts = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"z", &object) == FAILURE) {
		return;
	}

	if (Z_TYPE_P(object) != IS_OBJECT) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "%s::mergeFrom expects %s class", Z_OBJCE_P(instance)->name, Z_OBJCE_P(instance)->name);
		return;
	}

	if (Z_OBJCE_P(object) != Z_OBJCE_P(instance)) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "%s::mergeFrom expects %s class, but %s given", Z_OBJCE_P(instance)->name, Z_OBJCE_P(instance)->name, Z_OBJCE_P(object)->name);
		return;
	}

	PHP_PB_MESSAGE_CHECK_SCHEME

	php_protocolbuffers_get_hash(container, container->scheme, instance, &n, &n_len, &htt TSRMLS_CC);
	php_protocolbuffers_get_hash(container, container->scheme, object, &n, &n_len, &hts TSRMLS_CC);
	php_protocolbuffers_message_merge_from(container, htt, hts TSRMLS_CC);
}
/* }}} */


/* {{{ proto mixed ProtocolBuffersMessage::current()
*/
PHP_METHOD(protocolbuffers_message, current)
{
	zval *instance = getThis();
	pb_scheme_container *container;
	HashTable *proto = NULL;
	php_protocolbuffers_message *message;
	const char *name;
	int name_len = 0;
	zval **tmp = NULL;
	HashTable *hash;

	PHP_PB_MESSAGE_CHECK_SCHEME
	message = PHP_PROTOCOLBUFFERS_GET_OBJECT(php_protocolbuffers_message, instance);

	if (container->use_single_property < 1) {
		name     = container->scheme[message->offset].mangled_name;
		name_len = container->scheme[message->offset].mangled_name_len;

		hash = Z_OBJPROP_P(instance);
	} else {
		zval **c;

		name     = container->scheme[message->offset].name;
		name_len = container->scheme[message->offset].name_len;

		if (zend_hash_find(Z_OBJPROP_P(instance), container->single_property_name, container->single_property_name_len+1, (void**)&c) == SUCCESS) {
			hash = Z_ARRVAL_PP(c);
		}

		hash = Z_OBJPROP_PP(c);
	}

	if (zend_hash_find(hash, name, name_len, (void **)&tmp) == SUCCESS) {
    	RETVAL_ZVAL(*tmp, 1, 0);
	}
}
/* }}} */

/* {{{ proto mixed ProtocolBuffersMessage::key()
*/
PHP_METHOD(protocolbuffers_message, key)
{
	zval *instance = getThis();
	HashTable *proto = NULL;
	pb_scheme_container *container;
	php_protocolbuffers_message *message;

	PHP_PB_MESSAGE_CHECK_SCHEME
	message = PHP_PROTOCOLBUFFERS_GET_OBJECT(php_protocolbuffers_message, instance);

	RETURN_STRING(container->scheme[message->offset].name, 1);
}
/* }}} */

/* {{{ proto mixed ProtocolBuffersMessage::next()
*/
PHP_METHOD(protocolbuffers_message, next)
{
	zval *instance = getThis();
	HashTable *proto = NULL;
	pb_scheme_container *container;
	php_protocolbuffers_message *message;

	PHP_PB_MESSAGE_CHECK_SCHEME
	message = PHP_PROTOCOLBUFFERS_GET_OBJECT(php_protocolbuffers_message, instance);
	message->offset++;
}
/* }}} */

/* {{{ proto void ProtocolBuffersMessage::rewind()
*/
PHP_METHOD(protocolbuffers_message, rewind)
{
	zval *instance = getThis();
	HashTable *proto = NULL;
	pb_scheme_container *container;
	php_protocolbuffers_message *message;

	PHP_PB_MESSAGE_CHECK_SCHEME
	message = PHP_PROTOCOLBUFFERS_GET_OBJECT(php_protocolbuffers_message, instance);

	if (message->max == 0) {
		message->max = container->size;
	}
	message->offset = 0;
}
/* }}} */

/* {{{ proto bool ProtocolBuffersMessage::valid()
*/
PHP_METHOD(protocolbuffers_message, valid)
{
	zval *instance = getThis();
	HashTable *proto = NULL;
	pb_scheme_container *container;
	php_protocolbuffers_message *message;

	PHP_PB_MESSAGE_CHECK_SCHEME
	message = PHP_PROTOCOLBUFFERS_GET_OBJECT(php_protocolbuffers_message, instance);

	if (-1 < message->offset && message->offset < message->max) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto void ProtocolBuffersMessage::discardUnknownFields()
*/
PHP_METHOD(protocolbuffers_message, discardUnknownFields)
{
	zval *instance = getThis();
	HashTable *proto = NULL;
	pb_scheme_container *container;
	int free = 0;

	PHP_PB_MESSAGE_CHECK_SCHEME
	if (container->process_unknown_fields > 0) {
		char *uname;
		int uname_len;
		zval **unknown;

		if (container->use_single_property > 0) {
			uname = pb_get_default_unknown_property_name();
			uname_len = pb_get_default_unknown_property_name_len();
		} else {
			zend_mangle_property_name(&uname, &uname_len, (char*)"*", 1, (char*)pb_get_default_unknown_property_name(), pb_get_default_unknown_property_name_len(), 0);
			free = 1;
		}

		if (zend_hash_find(Z_OBJPROP_P(instance), uname, uname_len+1, (void**)&unknown) == SUCCESS) {
			php_pb_unknown_field_clear(INTERNAL_FUNCTION_PARAM_PASSTHRU, *unknown);
		}

		if (free) {
			efree(uname);
		}
	}
}
/* }}} */

/* {{{ proto void ProtocolBuffersMessage::clear(string $name)
*/
PHP_METHOD(protocolbuffers_message, clear)
{
	zval *instance = getThis();
	char *name;
	int name_len;
	HashTable *proto = NULL;
	pb_scheme_container *container;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"s", &name, &name_len) == FAILURE) {
		return;
	}

	PHP_PB_MESSAGE_CHECK_SCHEME
	php_protocolbuffers_message_clear(INTERNAL_FUNCTION_PARAM_PASSTHRU, instance, container, name, name_len, NULL, 0);
}
/* }}} */

/* {{{ proto void ProtocolBuffersMessage::clearAll(bool $clear_unknown_fields = true)
*/
PHP_METHOD(protocolbuffers_message, clearAll)
{
	zval *instance = getThis();
	int i = 0;
	HashTable *proto = NULL, *hash = NULL;
	pb_scheme_container *container;
	zend_bool clear_unknown_fields = 1;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"|b", &clear_unknown_fields) == FAILURE) {
		return;
	}

	PHP_PB_MESSAGE_CHECK_SCHEME

	if (container->use_single_property < 1) {
		hash = Z_OBJPROP_P(instance);
	} else {
		zval **c;
		if (zend_hash_find(Z_OBJPROP_P(instance), container->single_property_name, container->single_property_name_len+1, (void**)&c) == SUCCESS) {
			hash = Z_ARRVAL_PP(c);
		} else {
			return;
		}
	}

	for (i = 0; i < container->size; i++) {
		char *name;
		int name_len;
		zval **tmp = NULL;
		zval *val = NULL;

		if (container->use_single_property < 1) {
			name = container->scheme[i].mangled_name;
			name_len = container->scheme[i].mangled_name_len;
		} else {
			name = container->scheme[i].name;
			name_len = container->scheme[i].name_len;
		}

		if (zend_hash_find(hash, name, name_len, (void **)&tmp) == SUCCESS) {
			switch (Z_TYPE_PP(tmp)) {
			case IS_ARRAY:
				MAKE_STD_ZVAL(val);
				array_init(val);

				Z_ADDREF_P(val);
				zend_hash_update(hash, name, name_len, (void **)&val, sizeof(zval *), NULL);
				zval_ptr_dtor(&val);
				break;
			case IS_STRING:
			case IS_LONG:
			case IS_DOUBLE:
			case IS_OBJECT:
				MAKE_STD_ZVAL(val);
				ZVAL_NULL(val);

				Z_ADDREF_P(val);
				zend_hash_update(hash, name, name_len, (void **)&val, sizeof(zval *), NULL);
				zval_ptr_dtor(&val);
				break;
			}
		}
	}

	if (clear_unknown_fields > 0 && container->process_unknown_fields > 0) {
		char *uname;
		int uname_len;
		zval **unknown;

		if (container->use_single_property > 0) {
			uname = pb_get_default_unknown_property_name();
			uname_len = pb_get_default_unknown_property_name_len();
		} else {
			zend_mangle_property_name(&uname, &uname_len, (char*)"*", 1, (char*)pb_get_default_unknown_property_name(), pb_get_default_unknown_property_name_len(), 0);
		}

		if (zend_hash_find(Z_OBJPROP_P(instance), uname, uname_len, (void**)&unknown) == SUCCESS) {
			php_pb_unknown_field_clear(INTERNAL_FUNCTION_PARAM_PASSTHRU, *unknown);
		}
	}
}
/* }}} */

/* {{{ proto bool ProtocolBuffersMessage::__call()
*/
PHP_METHOD(protocolbuffers_message, __call)
{
	zval *instance = getThis();
	HashTable *proto = NULL;
	pb_scheme_container *container;
	char *name;
	int name_len;
	zval *params;
	int i = 0;
	int flag = 0;
	smart_str buf = {0};
	smart_str buf2 = {0};

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"sz", &name, &name_len, &params) == FAILURE) {
		return;
	}

	for (i = 0; i < name_len; i++) {
		if (flag == 0) {
			if (i+2 < name_len && name[i] == 'g' && name[i+1] == 'e' && name[i+2] == 't') {
				i += 2;
				flag = MAGICMETHOD_GET;
				continue;
			} else if (i+2 < name_len && name[i] == 's' && name[i+1] == 'e' && name[i+2] == 't') {
				i += 2;
				flag = MAGICMETHOD_SET;
				continue;
			} else if (i+6 < name_len &&
				name[i] == 'a' &&
				name[i+1] == 'p' &&
				name[i+2] == 'p' &&
				name[i+3] == 'e' &&
				name[i+4] == 'n' &&
				name[i+5] == 'd'
			) {
				i += 6;
				flag = MAGICMETHOD_APPEND;
			} else if (i+5 < name_len &&
				name[i] == 'c' &&
				name[i+1] == 'l' &&
				name[i+2] == 'e' &&
				name[i+3] == 'a' &&
				name[i+4] == 'r') {
				i += 5;
				flag = MAGICMETHOD_CLEAR;
			} else if (i+3 < name_len &&
				name[i] == 'h' &&
				name[i+1] == 'a' &&
				name[i+2] == 's'
			) {
				i += 3;
				flag = MAGICMETHOD_HAS;
			} else {
				break;
			}
		}

		if (name[i] >= 'A' && name[i] <= 'Z') {
			if (buf.len > 1) {
				smart_str_appendc(&buf, '_');
			}
			smart_str_appendc(&buf, name[i] + ('a' - 'A'));
			smart_str_appendc(&buf2, name[i]);
		} else {
			smart_str_appendc(&buf, name[i]);
			smart_str_appendc(&buf2, name[i]);
		}
	}
	smart_str_0(&buf);
	smart_str_0(&buf2);

	if (flag == 0) {
		zend_error(E_ERROR, "Call to undefined method %s::%s()", Z_OBJCE_P(instance)->name, name);
		return;
	}

	PHP_PB_MESSAGE_CHECK_SCHEME
	switch (flag) {
		case MAGICMETHOD_GET:
			php_protocolbuffers_message_get(INTERNAL_FUNCTION_PARAM_PASSTHRU, instance, container, buf.c, buf.len, buf2.c, buf2.len);
		break;
		case MAGICMETHOD_SET:
		{
			zval **tmp = NULL;

			zend_hash_get_current_data(Z_ARRVAL_P(params), (void **)&tmp);
			Z_ADDREF_PP(tmp);
			php_protocolbuffers_message_set(INTERNAL_FUNCTION_PARAM_PASSTHRU, instance, container, buf.c, buf.len, buf2.c, buf2.len, *tmp);
		}
		break;
		case MAGICMETHOD_APPEND:
		{
			zval **tmp = NULL;

			zend_hash_get_current_data(Z_ARRVAL_P(params), (void **)&tmp);
			Z_ADDREF_PP(tmp);
			php_protocolbuffers_message_append(INTERNAL_FUNCTION_PARAM_PASSTHRU, instance, container, buf.c, buf.len, buf2.c, buf2.len, *tmp);
		}
		break;
		case MAGICMETHOD_CLEAR:
			php_protocolbuffers_message_clear(INTERNAL_FUNCTION_PARAM_PASSTHRU, instance, container, buf.c, buf.len, buf2.c, buf2.len);
		case MAGICMETHOD_HAS:
			php_protocolbuffers_message_has(INTERNAL_FUNCTION_PARAM_PASSTHRU, instance, container, buf.c, buf.len, buf2.c, buf2.len);
		break;
	}

	smart_str_free(&buf);
	smart_str_free(&buf2);
}
/* }}} */

/* {{{ proto ProtocolBuffers_FieldDescriptor ProtocolBuffersMessage::has($key)
*/
PHP_METHOD(protocolbuffers_message, has)
{
	zval *instance = getThis();
	char *name;
	int name_len;
	HashTable *proto = NULL;
	pb_scheme_container *container;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"s", &name, &name_len) == FAILURE) {
		return;
	}

	PHP_PB_MESSAGE_CHECK_SCHEME
	php_protocolbuffers_message_has(INTERNAL_FUNCTION_PARAM_PASSTHRU, instance, container, name, name_len, NULL, 0);
}
/* }}} */

/* {{{ proto ProtocolBuffers_FieldDescriptor ProtocolBuffersMessage::get($key)
*/
PHP_METHOD(protocolbuffers_message, get)
{
	zval *instance = getThis();
	char *name;
	int name_len;
	HashTable *proto = NULL;
	pb_scheme_container *container;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"s", &name, &name_len) == FAILURE) {
		return;
	}

	PHP_PB_MESSAGE_CHECK_SCHEME
	php_protocolbuffers_message_get(INTERNAL_FUNCTION_PARAM_PASSTHRU, instance, container, name, name_len, NULL, 0);
}
/* }}} */

/* {{{ proto void ProtocolBuffersMessage::set($name, $value)
*/
PHP_METHOD(protocolbuffers_message, set)
{
	zval *instance = getThis();
	zval *value;
	char *name;
	int name_len;
	pb_scheme_container *container;
	HashTable *proto = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"sz", &name, &name_len, &value) == FAILURE) {
		return;
	}

	PHP_PB_MESSAGE_CHECK_SCHEME
	php_protocolbuffers_message_set(INTERNAL_FUNCTION_PARAM_PASSTHRU, instance, container, name, name_len, NULL, 0, value);
}


/* {{{ proto void ProtocolBuffersMessage::append($name, $value)
*/
PHP_METHOD(protocolbuffers_message, append)
{
	zval *instance = getThis();
	zval *value;
	char *name;
	int name_len;
	pb_scheme_container *container;
	HashTable *proto = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"sz", &name, &name_len, &value) == FAILURE) {
		return;
	}

	PHP_PB_MESSAGE_CHECK_SCHEME
	php_protocolbuffers_message_append(INTERNAL_FUNCTION_PARAM_PASSTHRU, instance, container, name, name_len, NULL, 0, value);
}



/* {{{ proto mixed ProtocolBuffersMessage::getExtension(string $name)
*/
PHP_METHOD(protocolbuffers_message, getExtension)
{
	zval *instance = getThis();
	zval *registry = php_protocolbuffers_extension_registry_get_instance(TSRMLS_C);
	zval *extension_registry = NULL;
	zval *field_descriptor = NULL;
	zend_class_entry *ce;
	HashTable *htt = NULL;
	char *name, *n;
	int name_len, n_len;
	pb_scheme_container *container;
	zval **e, **b;
	int is_mangled = 0;
	HashTable *proto = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"s", &name, &name_len) == FAILURE) {
		return;
	}

	ce = Z_OBJCE_P(instance);

	if (!php_protocolbuffers_extension_registry_get_registry(registry, ce->name, ce->name_length, &extension_registry TSRMLS_CC)) {
		goto err;
	}

	if (!php_protocolbuffers_extension_registry_get_descriptor_by_name(extension_registry, name, name_len, &field_descriptor TSRMLS_CC)) {
		goto err;
	}

	PHP_PB_MESSAGE_CHECK_SCHEME
	if (container->use_single_property > 0) {
		if (zend_hash_find(Z_OBJPROP_P(instance), container->single_property_name, container->single_property_name_len, (void **)&b) == FAILURE) {
			return;
		}

		n = name;
		n_len = name_len;
		htt = Z_ARRVAL_PP(b);
	} else {
		htt = Z_OBJPROP_P(instance);
		zend_mangle_property_name(&n, &n_len, (char*)"*", 1, (char*)name, name_len+1, 0);
		is_mangled = 1;
	}

	if (zend_hash_find(htt, n, n_len, (void **)&e) == SUCCESS) {
		if (is_mangled) {
			efree(n);
		}

		RETURN_ZVAL(*e, 1, 0);
	}

	return;
err:
	zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "extension %s does not find", name);
}
/* }}} */

/* {{{ proto bool ProtocolBuffersMessage::hasExtension(string $name)
*/
PHP_METHOD(protocolbuffers_message, hasExtension)
{
	zval *instance = getThis();
	zval *registry = php_protocolbuffers_extension_registry_get_instance(TSRMLS_C);
	zval *extension_registry = NULL;
	zval *field_descriptor = NULL;
	zend_class_entry *ce;
	HashTable *htt = NULL;
	char *name, *n;
	int name_len, n_len;
	pb_scheme_container *container;
	zval **e, **b;
	int is_mangled = 0;
	HashTable *proto = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"s", &name, &name_len) == FAILURE) {
		return;
	}

	ce = Z_OBJCE_P(instance);

	if (!php_protocolbuffers_extension_registry_get_registry(registry, ce->name, ce->name_length, &extension_registry TSRMLS_CC)) {
		goto err;
	}

	if (!php_protocolbuffers_extension_registry_get_descriptor_by_name(extension_registry, name, name_len, &field_descriptor TSRMLS_CC)) {
		goto err;
	}

	PHP_PB_MESSAGE_CHECK_SCHEME
	if (container->use_single_property > 0) {
		if (zend_hash_find(Z_OBJPROP_P(instance), container->single_property_name, container->single_property_name_len, (void **)&b) == FAILURE) {
			return;
		}

		n = name;
		n_len = name_len;
		htt = Z_ARRVAL_PP(b);
	} else {
		htt = Z_OBJPROP_P(instance);
		zend_mangle_property_name(&n, &n_len, (char*)"*", 1, (char*)name, name_len+1, 0);
		is_mangled = 1;
	}

	if (zend_hash_find(htt, n, n_len, (void **)&e) == SUCCESS) {
		if (is_mangled) {
			efree(n);
		}

		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
err:
	zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "extension %s does not find", name);
}
/* }}} */

/* {{{ proto void ProtocolBuffersMessage::setExtension(string $name, mixed $value)
*/
PHP_METHOD(protocolbuffers_message, setExtension)
{
	zval *instance = getThis();
	zval *registry = php_protocolbuffers_extension_registry_get_instance(TSRMLS_C);
	zval *extension_registry = NULL;
	zval *field_descriptor = NULL;
	zend_class_entry *ce;
	HashTable *proto = NULL, *htt = NULL;
	char *name, *n;
	int name_len, n_len;
	pb_scheme_container *container;
	zval *value, **b;
	int is_mangled = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"sz", &name, &name_len, &value) == FAILURE) {
		return;
	}

	ce = Z_OBJCE_P(instance);

	if (!php_protocolbuffers_extension_registry_get_registry(registry, ce->name, ce->name_length, &extension_registry TSRMLS_CC)) {
		goto err;
	}

	if (!php_protocolbuffers_extension_registry_get_descriptor_by_name(extension_registry, name, name_len, &field_descriptor TSRMLS_CC)) {
		goto err;
	}

	PHP_PB_MESSAGE_CHECK_SCHEME
	if (container->use_single_property > 0) {
		if (zend_hash_find(Z_OBJPROP_P(instance), container->single_property_name, container->single_property_name_len+1, (void **)&b) == FAILURE) {
			return;
		}

		n = name;
		n_len = name_len+1;
		htt = Z_ARRVAL_PP(b);
	} else {
		htt = Z_OBJPROP_P(instance);
		zend_mangle_property_name(&n, &n_len, (char*)"*", 1, (char*)name, name_len+1, 0);
		is_mangled = 1;
	}

	Z_ADDREF_P(value);
	zend_hash_update(htt, n, n_len, (void **)&value, sizeof(zval *), NULL);
	if (is_mangled) {
		efree(n);
	}

	return;
err:
	zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "extension %s does not find", name);
}
/* }}} */

/* {{{ proto void ProtocolBuffersMessage::setExtension(string $name)
*/
PHP_METHOD(protocolbuffers_message, clearExtension)
{
	zend_throw_exception_ex(spl_ce_RuntimeException, 0 TSRMLS_CC, "Not implemented yet");
}
/* }}} */


/* {{{ proto ProtocolBuffersUnknownFieldSet ProtocolBuffersMessage::getUnknownFieldSet()
*/
PHP_METHOD(protocolbuffers_message, getUnknownFieldSet)
{
	zval *instance = getThis();
	HashTable *target = NULL, *proto = NULL;
	pb_scheme_container *container;
	zval **unknown_fieldset;

	PHP_PB_MESSAGE_CHECK_SCHEME

	if (container->process_unknown_fields > 0) {
		char *unknown_name;
		int unknown_name_len;
		int free = 0;

		if (container->use_single_property > 0) {
			zval **b;
			if (zend_hash_find(Z_OBJPROP_P(instance), container->single_property_name, container->single_property_name_len, (void **)&b) == FAILURE) {
				return;
			}

			unknown_name     = pb_get_default_unknown_property_name();
			unknown_name_len = pb_get_default_unknown_property_name_len();
			target = Z_ARRVAL_PP(b);
		} else {
			zend_mangle_property_name(&unknown_name, &unknown_name_len, (char*)"*", 1, (char*)pb_get_default_unknown_property_name(), pb_get_default_unknown_property_name_len(), 0);
			target = Z_OBJPROP_P(instance);
			free = 1;
		}

		if (zend_hash_find(target, (char*)unknown_name, unknown_name_len+1, (void **)&unknown_fieldset) == SUCCESS) {
			RETVAL_ZVAL(*unknown_fieldset, 1, 0);
		}

		if (free) {
			efree(unknown_name);
		}
	} else {
		zend_throw_exception_ex(spl_ce_RuntimeException, 0 TSRMLS_CC, "process unknown fileds flag seems false");
	}
}
/* }}} */


static zend_function_entry php_protocolbuffers_message_methods[] = {
	PHP_ME(protocolbuffers_message, __construct,          arginfo_pb_message___construct, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
	PHP_ME(protocolbuffers_message, serializeToString,    arginfo_pb_message_serialize_to_string, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, parseFromString,      arginfo_pb_message_parse_from_string, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(protocolbuffers_message, mergeFrom,            arginfo_pb_message_merge_from, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, discardUnknownFields, arginfo_pb_message_discard_unknown_fields, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, clear,                arginfo_pb_message_clear, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, clearAll,             NULL, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, has,                  arginfo_pb_message_has, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, get,                  arginfo_pb_message_get, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, set,                  arginfo_pb_message_set, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, append,               arginfo_pb_message_append, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, hasExtension,         arginfo_pb_message_has_extension, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, getExtension,         arginfo_pb_message_get_extension, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, setExtension,         arginfo_pb_message_set_extension, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, clearExtension,       arginfo_pb_message_clear_extension, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, getUnknownFieldSet,   NULL, ZEND_ACC_PUBLIC)
	/* iterator */
	PHP_ME(protocolbuffers_message, current,   NULL, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, key,       NULL, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, next,      NULL, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, rewind,    NULL, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, valid,     NULL, ZEND_ACC_PUBLIC)
	/* magic method */
	PHP_ME(protocolbuffers_message, __call,   arginfo_pb_message___call, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

void php_pb_message_class(TSRMLS_D)
{
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "ProtocolBuffersMessage", php_protocolbuffers_message_methods);
	protocol_buffers_message_class_entry = zend_register_internal_class(&ce TSRMLS_CC);
	zend_class_implements(protocol_buffers_message_class_entry TSRMLS_CC, 1, zend_ce_iterator);
	zend_class_implements(protocol_buffers_message_class_entry TSRMLS_CC, 1, protocol_buffers_serializable_class_entry);

	protocol_buffers_message_class_entry->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;
	protocol_buffers_message_class_entry->create_object = php_protocolbuffers_message_new;

	memcpy(&php_protocolbuffers_message_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

	PHP_PROTOCOLBUFFERS_REGISTER_NS_CLASS_ALIAS(PHP_PROTOCOLBUFFERS_NAMESPACE, "Message", protocol_buffers_message_class_entry);
}
