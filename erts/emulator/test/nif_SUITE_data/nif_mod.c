#include "erl_nif.h"
#include <string.h>
#include <stdio.h>

#include "nif_mod.h"

#define CHECK(X) ((void)((X) || (check_abort(__LINE__),1)))
#ifdef __GNUC__
static void check_abort(unsigned line) __attribute__((noreturn));
#endif
static void check_abort(unsigned line)
{
    enif_fprintf(stderr, "Test CHECK failed at %s:%u\r\n",
	    __FILE__, line);
    abort();
}

static int static_cntA; /* zero by default */
static int static_cntB = NIF_LIB_VER * 100;

static ERL_NIF_TERM am_true;
static ERL_NIF_TERM am_null;
static ERL_NIF_TERM am_resource_type;
static ERL_NIF_TERM am_resource_dtor_A;
static ERL_NIF_TERM am_resource_dtor_B;

static void init(ErlNifEnv* env)
{
    am_true = enif_make_atom(env, "true");
    am_null = enif_make_atom(env, "null");
    am_resource_type = enif_make_atom(env, "resource_type");
    am_resource_dtor_A = enif_make_atom(env, "resource_dtor_A");
    am_resource_dtor_B = enif_make_atom(env, "resource_dtor_B");
}

static void add_call_with_arg(ErlNifEnv* env, NifModPrivData* data, const char* func_name,
			      const char* arg, int arg_sz)
{
    CallInfo* call = enif_alloc(env, sizeof(CallInfo)+strlen(func_name) + arg_sz);
    strcpy(call->func_name, func_name);
    call->lib_ver = NIF_LIB_VER;
    call->static_cntA = ++static_cntA;
    call->static_cntB = ++static_cntB;
    call->arg_sz = arg_sz;
    if (arg != NULL) {
	call->arg = call->func_name + strlen(func_name) + 1;
	memcpy(call->arg, arg, arg_sz);
    }
    else {
	call->arg = NULL;
    }
    enif_mutex_lock(data->mtx);
    call->next = data->call_history;
    data->call_history = call;
    enif_mutex_unlock(data->mtx);
}

static void add_call(ErlNifEnv* env, NifModPrivData* data,const char* func_name)
{
    add_call_with_arg(env, data, func_name, NULL, 0);
}

#define ADD_CALL(FUNC_NAME) add_call(env, enif_priv_data(env),FUNC_NAME)

#define STRINGIFY_(X) #X
#define STRINGIFY(X) STRINGIFY_(X)

static void resource_dtor_A(ErlNifEnv* env, void* a)
{
    const char dtor_name[] = "resource_dtor_A_v"  STRINGIFY(NIF_LIB_VER); 

    add_call_with_arg(env, enif_priv_data(env), dtor_name,
		      a, enif_sizeof_resource(env, a));
}

static void resource_dtor_B(ErlNifEnv* env, void* a)
{
    const char dtor_name[] = "resource_dtor_B_v"  STRINGIFY(NIF_LIB_VER); 

    add_call_with_arg(env, enif_priv_data(env), dtor_name,
		      a, enif_sizeof_resource(env, a));
}

/* {resource_type, Ix|null, ErlNifResourceFlags in, "TypeName", dtor(A|B|null), ErlNifResourceFlags out}*/
static void open_resource_type(ErlNifEnv* env, ERL_NIF_TERM op_tpl)
{
    NifModPrivData* data = enif_priv_data(env);
    const ERL_NIF_TERM* arr;
    int arity;
    char rt_name[30];
    union { enum ErlNifResourceFlags e; int i; } flags, exp_res, got_res;
    unsigned ix;
    ErlNifResourceDtor* dtor;
    ErlNifResourceType* got_ptr;

    CHECK(enif_get_tuple(env, op_tpl, &arity, &arr));
    CHECK(arity == 6);
    CHECK(enif_is_identical(env, arr[0], am_resource_type));
    CHECK(enif_get_int(env, arr[2], &flags.i));
    CHECK(enif_get_string(env, arr[3], rt_name, sizeof(rt_name), ERL_NIF_LATIN1) > 0);
    CHECK(enif_get_int(env, arr[5], &exp_res.i));
	
    if (enif_is_identical(env, arr[4], am_null)) {
	dtor = NULL;
    }
    else if (enif_is_identical(env, arr[4], am_resource_dtor_A)) {
	dtor = resource_dtor_A;
    }
    else {
	CHECK(enif_is_identical(env, arr[4], am_resource_dtor_B));
	dtor = resource_dtor_B;
    }

    got_ptr = enif_open_resource_type(env, rt_name, dtor,
				      flags.e, &got_res.e);

    if (enif_get_uint(env, arr[1], &ix) && ix < RT_MAX && got_ptr != NULL) {
	data->rt_arr[ix] = got_ptr;
    }
    else {
	CHECK(enif_is_identical(env, arr[1], am_null));
	CHECK(got_ptr == NULL);
    }
    CHECK(got_res.e == exp_res.e);
}

static void do_load_info(ErlNifEnv* env, ERL_NIF_TERM load_info)
{
    NifModPrivData* data = enif_priv_data(env);
    ERL_NIF_TERM head, tail;
    unsigned ix;
    for (ix=0; ix<RT_MAX; ix++) {
	data->rt_arr[ix] = NULL;
    }
    for (head = load_info; enif_get_list_cell(env, head, &head, &tail);
	  head = tail) {

	open_resource_type(env, head);	
    }    
    CHECK(enif_is_empty_list(env, head));
}

static int load(ErlNifEnv* env, void** priv_data, ERL_NIF_TERM load_info)
{
    NifModPrivData* data;

    init(env);
    data = enif_alloc(env, sizeof(NifModPrivData));
    CHECK(data != NULL);
    *priv_data = data;
    data->mtx = enif_mutex_create("nif_mod_priv_data"); 
    data->ref_cnt = 1;
    data->call_history = NULL;
    add_call(env, data, "load");

    do_load_info(env, load_info);
    data->calls = 0;    
    return 0;
}

static int reload(ErlNifEnv* env, void** priv_data, ERL_NIF_TERM load_info)
{
    init(env);
    add_call(env, *priv_data, "reload");
    
    do_load_info(env, load_info);
    return 0;
}

static int upgrade(ErlNifEnv* env, void** priv_data, void** old_priv_data, ERL_NIF_TERM load_info)
{
    NifModPrivData* data = *old_priv_data;
    init(env);
    add_call(env, data, "upgrade");
    data->ref_cnt++;

    *priv_data = *old_priv_data;    
    do_load_info(env, load_info);
    
    return 0;
}

static void unload(ErlNifEnv* env, void* priv_data)
{
    NifModPrivData* data = priv_data;
    add_call(env, data, "unload");
    enif_mutex_lock(data->mtx);
    if (--data->ref_cnt == 0) {
	enif_mutex_unlock(data->mtx);
	enif_mutex_destroy(data->mtx);
	enif_free(env, data);
    }
    enif_mutex_unlock(data->mtx);
}

static ERL_NIF_TERM lib_version(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    ADD_CALL("lib_version");
    return enif_make_int(env, NIF_LIB_VER);
}

static ERL_NIF_TERM get_priv_data_ptr(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    ADD_CALL("get_priv_data_ptr");
    return enif_make_ulong(env, (unsigned long)enif_priv_data(env));
}

static ERL_NIF_TERM make_new_resource(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    NifModPrivData* data = (NifModPrivData*) enif_priv_data(env);
    ErlNifBinary ibin;
    char* a;
    ERL_NIF_TERM ret;
    unsigned ix;
    if (!enif_get_uint(env, argv[0], &ix) || ix >= RT_MAX 
	|| !enif_inspect_binary(env, argv[1], &ibin)) {
	return enif_make_badarg(env);
    }
    a = enif_alloc_resource(env, data->rt_arr[ix], ibin.size);
    memcpy(a, ibin.data, ibin.size);
    ret = enif_make_resource(env, a);
    enif_release_resource(env, a);
    return ret;
}

static ERL_NIF_TERM get_resource(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    NifModPrivData* data = (NifModPrivData*) enif_priv_data(env);
    ErlNifBinary obin;
    unsigned ix;
    void* a;
    if (!enif_get_uint(env, argv[0], &ix) || ix >= RT_MAX 
	|| !enif_get_resource(env, argv[1], data->rt_arr[ix], &a)
	|| !enif_alloc_binary(env, enif_sizeof_resource(env, a), &obin)) { 
	return enif_make_badarg(env);
    }
    memcpy(obin.data, a, obin.size);
    return enif_make_binary(env, &obin);
}

static ErlNifFunc nif_funcs[] =
{
    {"lib_version", 0, lib_version},
    {"get_priv_data_ptr", 0, get_priv_data_ptr},
    {"make_new_resource", 2, make_new_resource},
    {"get_resource", 2, get_resource}
};

#if NIF_LIB_VER != 3
ERL_NIF_INIT(nif_mod,nif_funcs,load,reload,upgrade,unload)
#else
ERL_NIF_INIT_GLOB /* avoid link error on windows */
#endif

