/* -*- c-file-style: "ruby"; indent-tabs-mode: nil -*- */
/*
  descriptor.c - part of ruby-oci8

  Copyright (C) 2002-2007 KUBO Takehiro <kubo@jiubao.org>
*/
#include "oci8.h"

static VALUE cOCIRowid;

#if defined(RUNTIME_API_CHECK)
#define USE_ROWID1
#define USE_ROWID2
#elif have_OCIRowidToChar
#define USE_ROWID1
#define Init_oci8_rowid1 Init_oci8_rowid
#define oci8_get_rowid1_attr oci8_get_rowid_attr
#else
#define USE_ROWID2
#define Init_oci8_rowid2 Init_oci8_rowid
#define oci8_get_rowid2_attr oci8_get_rowid_attr
#endif

#ifdef USE_ROWID1
/* use OCIRowidToChar. */

#define MAX_ROWID_LEN 128

typedef struct {
    oci8_base_t base;
    char id[MAX_ROWID_LEN + 1];
} oci8_rowid1_t;

static oci8_base_class_t oci8_rowid1_class = {
    NULL,
    NULL,
    sizeof(oci8_rowid1_t),
};

static void oci8_rowid1_set(VALUE self, char *ptr, ub4 len)
{
    oci8_rowid1_t *rowid = DATA_PTR(self);

    if (len > MAX_ROWID_LEN) {
        rb_raise(rb_eArgError, "too long String to set. (%u for %d)", len, MAX_ROWID_LEN);
    }
    memcpy(rowid->id, ptr, len);
    rowid->id[len] = '\0';
}

static VALUE oci8_rowid1_initialize(VALUE self, VALUE val)
{
    oci8_rowid1_t *rowid = DATA_PTR(self);

    rowid->base.hp.ptr = NULL;
    rowid->base.type = 0;
    if (!NIL_P(val)) {
        StringValue(val);
        oci8_rowid1_set(self, RSTRING_PTR(val), RSTRING_LEN(val));
    } else {
        rowid->id[0] = '\0';
    }
    return Qnil;
}

static VALUE oci8_rowid1_initialize_copy(VALUE lhs, VALUE rhs)
{
    oci8_rowid1_t *l, *r;

    rb_obj_init_copy(lhs, rhs);
    l = DATA_PTR(lhs);
    r = DATA_PTR(rhs);
    memcpy(l->id, r->id, sizeof(l->id));
    return lhs;
}

/*
 * bind_rowid
 */
#define BIND_ROWID1_SIZE (((sizeof(oci8_vstr_t) + MAX_ROWID_LEN) + 3) & ~3)
static VALUE bind_rowid1_get(oci8_bind_t *obind, void *data, void *null_struct)
{
    VALUE rowid = rb_funcall(cOCIRowid, oci8_id_new, 1, Qnil);
    oci8_vstr_t *vstr = (oci8_vstr_t *)data;
    oci8_rowid1_set(rowid, vstr->buf, vstr->size);
    return rowid;
}

static void bind_rowid1_set(oci8_bind_t *obind, void *data, void **null_structp, VALUE val)
{
    oci8_rowid1_t *rowid;
    oci8_vstr_t *vstr = (oci8_vstr_t *)data;
    if (!rb_obj_is_instance_of(val, cOCIRowid))
        rb_raise(rb_eArgError, "Invalid argument: %s (expect OCIRowid)", rb_class2name(CLASS_OF(val)));
    rowid = DATA_PTR(val);
    memcpy(vstr->buf, rowid->id, sizeof(rowid->id));
    vstr->size = strlen(rowid->id);
}

static void bind_rowid1_init(oci8_bind_t *obind, VALUE svc, VALUE val, VALUE length)
{
    obind->value_sz = BIND_ROWID1_SIZE;
    obind->alloc_sz = BIND_ROWID1_SIZE;
}

static oci8_bind_class_t bind_rowid1_class = {
    {
        NULL,
        oci8_bind_free,
        sizeof(oci8_bind_t)
    },
    bind_rowid1_get,
    bind_rowid1_set,
    bind_rowid1_init,
    NULL,
    NULL,
    NULL,
    NULL,
    SQLT_LVC
};

VALUE oci8_get_rowid1_attr(oci8_base_t *base, ub4 attrtype)
{
    OCIRowid *riddp;
    VALUE rowid;
    sword rv;
    OraText buf[MAX_ROWID_LEN];
    ub2 buflen = MAX_ROWID_LEN;

    rv = OCIDescriptorAlloc(oci8_envhp, (dvoid*)&riddp, OCI_DTYPE_ROWID, 0, NULL);
    if (rv != OCI_SUCCESS)
        oci8_env_raise(oci8_envhp, rv);
    rv = OCIAttrGet(base->hp.ptr, base->type, riddp, 0, attrtype, oci8_errhp);
    if (rv != OCI_SUCCESS) {
        OCIDescriptorFree(riddp, OCI_DTYPE_ROWID);
        oci8_raise(oci8_errhp, rv, NULL);
    }
    rv = OCIRowidToChar(riddp, buf, &buflen, oci8_errhp);
    if (rv != OCI_SUCCESS) {
        OCIDescriptorFree(riddp, OCI_DTYPE_ROWID);
        oci8_raise(oci8_errhp, rv, NULL);
    }
    OCIDescriptorFree(riddp, OCI_DTYPE_ROWID);

    rowid = rb_funcall(cOCIRowid, oci8_id_new, 1, Qnil);
    oci8_rowid1_set(rowid, TO_CHARPTR(buf), buflen);
    return rowid;
}

static VALUE oci8_rowid1_to_s(VALUE self)
{
    oci8_rowid1_t *rowid = DATA_PTR(self);

    return rb_str_new2(rowid->id);
}

static VALUE oci8_rowid1_inspect(VALUE self)
{
    oci8_rowid1_t *rowid = DATA_PTR(self);
    char *name = rb_class2name(CLASS_OF(self));
    char *str = ALLOCA_N(char, strlen(name) + strlen(rowid->id) + 5);

    sprintf(str, "#<%s:%s>", name, rowid->id);
    return rb_str_new2(str);
}

/*
 *  call-seq:
 *    rowid._dump   => string
 *
 *  Dump <i>rowid</i> for marshaling.
 */
static VALUE oci8_rowid1_dump(int argc, VALUE *argv, VALUE self)
{
    oci8_rowid1_t *rowid = DATA_PTR(self);
    VALUE dummy;

    rb_scan_args(argc, argv, "01", &dummy);
    return rb_str_new2(rowid->id);
}

/*
 *  call-seq:
 *    OCIRowid._load(string)   => ocinumber
 *
 *  Unmarshal a dumped <code>OCIRowid</code> object.
 */
static VALUE oci8_rowid1_s_load(VALUE klass, VALUE str)
{
    VALUE rowid;
    StringValue(str);

    rowid = rb_funcall(klass, oci8_id_new, 1, Qnil);
    oci8_rowid1_set(rowid, RSTRING_PTR(str), RSTRING_LEN(str));
    return rowid;
}

void Init_oci8_rowid1(void)
{
    cOCIRowid = oci8_define_class("OCIRowid", &oci8_rowid1_class);
    rb_define_method(cOCIRowid, "initialize", oci8_rowid1_initialize, 1);
    oci8_define_bind_class("OCIRowid", &bind_rowid1_class);

    rb_define_method(cOCIRowid, "to_s", oci8_rowid1_to_s, 0);
    rb_define_method(cOCIRowid, "inspect", oci8_rowid1_inspect, 0);

    rb_define_method(cOCIRowid, "initialize_copy", oci8_rowid1_initialize_copy, 1);
    rb_define_method(cOCIRowid, "_dump", oci8_rowid1_dump, -1);
    rb_define_singleton_method(cOCIRowid, "_load", oci8_rowid1_s_load, 1);
}
#endif /* USE_ROWID2 */

#ifdef USE_ROWID2
/* don't use OCIRowidToChar. */

typedef struct {
    oci8_base_t base;
} oci8_rowid2_t;

static oci8_base_class_t oci8_rowid2_class = {
    NULL,
    NULL,
    sizeof(oci8_rowid2_t),
};

static VALUE oci8_rowid2_initialize(VALUE self)
{
    oci8_rowid2_t *rowid = DATA_PTR(self);
    sword rv;

    rv = OCIDescriptorAlloc(oci8_envhp, &rowid->base.hp.ptr, OCI_DTYPE_ROWID, 0, NULL);
    if (rv != OCI_SUCCESS)
        oci8_env_raise(oci8_envhp, rv);
    rowid->base.type = OCI_DTYPE_ROWID;
    return Qnil;
}

static VALUE oci8_rowid2_initialize_copy(VALUE lhs, VALUE rhs)
{
    rb_notimplement();
}

/*
 * bind_rowid
 */
static VALUE bind_rowid2_get(oci8_bind_t *obind, void *data, void *null_struct)
{
    oci8_hp_obj_t *oho = (oci8_hp_obj_t *)data;
    return oho->obj;
}

static void bind_rowid2_set(oci8_bind_t *obind, void *data, void **null_structp, VALUE val)
{
    oci8_hp_obj_t *oho = (oci8_hp_obj_t *)data;
    oci8_base_t *h;

    if (!rb_obj_is_instance_of(val, cOCIRowid))
        rb_raise(rb_eArgError, "Invalid argument: %s (expect OCIRowid)", rb_class2name(CLASS_OF(val)));
    h = DATA_PTR(val);
    oho->hp = h->hp.ptr;
    oho->obj = val;
}

static void bind_rowid2_init(oci8_bind_t *obind, VALUE svc, VALUE val, VALUE length)
{
    obind->value_sz = sizeof(void *);
    obind->alloc_sz = sizeof(oci8_hp_obj_t);
}

static void bind_rowid2_init_elem(oci8_bind_t *obind, VALUE svc)
{
    oci8_hp_obj_t *oho = (oci8_hp_obj_t *)obind->valuep;
    oci8_base_t *h;
    ub4 idx = 0;

    do {
        oho[idx].obj = rb_funcall(cOCIRowid, oci8_id_new, 0);
        h = DATA_PTR(oho[idx].obj);
        oho[idx].hp = h->hp.ptr;
    } while (++idx < obind->maxar_sz);
}

static oci8_bind_class_t bind_rowid2_class = {
    {
        oci8_bind_hp_obj_mark,
        oci8_bind_free,
        sizeof(oci8_bind_t)
    },
    bind_rowid2_get,
    bind_rowid2_set,
    bind_rowid2_init,
    bind_rowid2_init_elem,
    NULL,
    NULL,
    SQLT_RDD
};

VALUE oci8_get_rowid2_attr(oci8_base_t *base, ub4 attrtype)
{
    oci8_base_t *rowid;
    VALUE obj;
    sword rv;

    obj = rb_funcall(cOCIRowid, oci8_id_new, 0);
    rowid = DATA_PTR(obj);
    rv = OCIAttrGet(base->hp.ptr, base->type, rowid->hp.ptr, 0, attrtype, oci8_errhp);
    if (rv != OCI_SUCCESS) {
        oci8_base_free(rowid);
        oci8_raise(oci8_errhp, rv, NULL);
    }
    return obj;
}

void Init_oci8_rowid2(void)
{
    cOCIRowid = oci8_define_class("OCIRowid", &oci8_rowid2_class);
    rb_define_method(cOCIRowid, "initialize", oci8_rowid2_initialize, 0);
    rb_define_method(cOCIRowid, "initialize_copy", oci8_rowid2_initialize_copy, 1);
    oci8_define_bind_class("OCIRowid", &bind_rowid2_class);
}
#endif /* USE_ROWID2 */

#ifdef RUNTIME_API_CHECK
void Init_oci8_rowid(void)
{
    if (have_OCIRowidToChar) {
        Init_oci8_rowid1();
    } else {
        Init_oci8_rowid2();
    }
}

VALUE oci8_get_rowid_attr(oci8_base_t *base, ub4 attrtype)
{
    if (have_OCIRowidToChar) {
        return oci8_get_rowid1_attr(base, attrtype);
    } else {
        return oci8_get_rowid2_attr(base, attrtype);
    }
}
#endif /* RUNTIME_API_CHECK */