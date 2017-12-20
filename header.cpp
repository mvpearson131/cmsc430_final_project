

//#include "gc.h"    // Add back in and change tags if we want to use GC
#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "uthash.h"

#define CLO_TAG 0
#define CONS_TAG 1
#define INT_TAG 2
#define STR_TAG 3
#define SYM_TAG 4
#define OTHER_TAG 6
#define ENUM_TAG 7


#define VECTOR_OTHERTAG 1
// Hashes, Sets, gen records, can all be added here
#define HASH_OTHERTAG 2

#define V_HASH 47  //40 + 7
#define V_VOID 39  //32 +7 (+7 is for anything enumerable other than null)
#define V_TRUE 31  //24 +7
#define V_FALSE 15 //8  +7
#define V_NULL 0



#define MASK64 0xffffffffffffffff // useful for tagging related operations


#define ASSERT_TAG(v,tag,msg) \
    if(((v)&7ULL) != (tag)) \
        fatal_err(msg);

#define ASSERT_VALUE(v,val,msg) \
    if(((u64)(v)) != (val))     \
        fatal_err(msg);

#define ASSERT_VALUE_NOT(v,val,msg) \
	if(((u64)(v)&7ULL) == (val))			\
		fatal_err(msg);


#define DECODE_CLO(v) ((u64*)((v)&(7ULL^MASK64)))
#define ENCODE_CLO(v) (((u64)(v)) | CLO_TAG)

#define DECODE_CONS(v) ((u64*)((v)&(7ULL^MASK64)))
#define ENCODE_CONS(v) (((u64)(v)) | CONS_TAG)

#define DECODE_INT(v) ((s32)((u32)(((v)&(7ULL^MASK64)) >> 32)))
#define ENCODE_INT(v) ((((u64)((u32)(v))) << 32) | INT_TAG)

#define DECODE_STR(v) ((char*)((v)&(7ULL^MASK64)))
#define ENCODE_STR(v) (((u64)(v)) | STR_TAG)

#define DECODE_SYM(v) ((char*)((v)&(7ULL^MASK64)))
#define ENCODE_SYM(v) (((u64)(v)) | SYM_TAG)

#define DECODE_OTHER(v) ((u64*)((v)&(7ULL^MASK64)))
#define ENCODE_OTHER(v) (((u64)(v)) | OTHER_TAG)


// some apply-prim macros for expecting 1 argument or 2 arguments
#define GEN_EXPECT1ARGLIST(f,g) \
    u64 f(u64 lst) \
    { \
        u64 v0 = expect_args1(lst); \
        return g(v0); \
    } 

#define GEN_EXPECT2ARGLIST(f,g) \
    u64 f(u64 lst) \
    { \
        u64 rest; \
        u64 v0 = expect_cons(lst, &rest); \
        u64 v1 = expect_cons(rest, &rest); \
        if (rest != V_NULL) \
            fatal_err("prim applied on more than 2 arguments."); \
        return g(v0,v1);                                           \
    } 

#define GEN_EXPECT3ARGLIST(f,g) \
    u64 f(u64 lst) \
    { \
        u64 rest; \
        u64 v0 = expect_cons(lst, &rest); \
        u64 v1 = expect_cons(rest, &rest); \
        u64 v2 = expect_cons(rest, &rest); \
        if (rest != V_NULL) \
            fatal_err("prim applied on more than 2 arguments."); \
        return g(v0,v1,v2);                                        \
    } 





// No mangled names
extern "C"
{



typedef uint64_t u64;
typedef int64_t s64;
typedef uint32_t u32;
typedef int32_t s32;

u64 curmem = 0;
u64 maxmem = 268435456;
    
// UTILS

void fatal_err(const char* msg)
{
    printf("library run-time error: ");
    printf("%s", msg);
    printf("\n");
    exit(1);
}

void check_mem(u64 m)
{
	curmem += m;
	if(maxmem <= curmem)
		fatal_err("Ran out of memory. Memory cap is 256MB");
}

typedef struct {
	int tag;
	u64 id;
	u64 val;
	UT_hash_handle hh;
} my_hash;

void add_val(my_hash **hashptr, u64 key, u64 val)
{
	my_hash* s;

	HASH_FIND(hh, *hashptr, &key, sizeof(u64), s);
	if(s == NULL)
	{	
		check_mem(sizeof(my_hash));
		s = (my_hash *)malloc(sizeof(my_hash));
		s->id = key;
		HASH_ADD(hh, *hashptr, id, sizeof(u64), s);
	}
	s->val = val;
}

my_hash *get_val(my_hash **hashptr, u64 key)
{
	my_hash *s;
	HASH_FIND(hh, *hashptr, &key, sizeof(u64), s);
	return s;
}


u64* alloc(const u64 m)
{
    return (u64*)(malloc(m));
    //return new u64[m];
    //return (u64*)GC_MALLOC(m);
}

void print_u64(u64 i)
{
    printf("%lu\n", i);
}

u64 prim_fatal()
{
	fatal_err("Uninitialized Variable.");
	return V_NULL;
}

u64 expect_args0(u64 args)
{
    if (args != V_NULL)
        fatal_err("Expected value: null (in expect_args0). Prim cannot take arguments.");
    return V_NULL;
}

u64 expect_args1(u64 args)
{
    ASSERT_TAG(args, CONS_TAG, "Expected cons value (in expect_args1). Prim applied on an empty argument list.")
    u64* p = DECODE_CONS(args);
    ASSERT_VALUE((p[1]), V_NULL, "Expected null value (in expect_args1). Prim can only take 1 argument.")
    return p[0];
}

u64 expect_cons(u64 p, u64* rest)
{
    // pass a pair value p and a pointer to a word *rest                          
    // verifiies (cons? p), returns the value (car p) and assigns *rest = (cdr p) 
    ASSERT_TAG(p, CONS_TAG, "Expected a cons value. (expect_cons)")               

    u64* pp = DECODE_CONS(p);
    *rest = pp[1];
    return pp[0];
}

u64* expect_pair(u64 p)
{
    // pass a pair value p and a pointer to a word *rest                          
    // verifiies (cons? p), returns the value (car p) and assigns *rest = (cdr p) 
    ASSERT_TAG(p, CONS_TAG, "Expected a cons value. (expect_cons)")               
    u64* pp = DECODE_CONS(p);
    return pp;
}

u64 expect_other(u64 v, u64* rest)
{
    // returns the runtime tag value
    // puts the untagged value at *rest
    ASSERT_TAG(v, OTHER_TAG, "Expected a vector or special value. (expect_other)")
    
    u64* p = DECODE_OTHER(v);
    *rest = p[1];
    return p[0];
}


/////// CONSTANTS
    
    
u64 const_init_int(s64 i)
{
    return ENCODE_INT((s32)i);
}

u64 const_init_void()
{
    return V_VOID;
}


u64 const_init_null()
{
    return V_NULL;
}


u64 const_init_true()
{
    return V_TRUE;
}

    
u64 const_init_false()
{
    return V_FALSE;
}

    
u64 const_init_string(const char* s)
{
    return ENCODE_STR(s);
}
        
u64 const_init_symbol(const char* s)
{
    return ENCODE_SYM(s);
}







/////////// PRIMS

    
///// effectful prims:

    
u64 prim_print_aux(u64 v) 
{
    if (v == V_NULL)
        printf("()");
    else if ((v&7) == CLO_TAG)
        printf("#<procedure>");
    else if ((v&7) == CONS_TAG)
    {
        u64* p = DECODE_CONS(v);
        printf("(");
        prim_print_aux(p[0]);
        printf(" . ");
        prim_print_aux(p[1]);
        printf(")");
    }
    else if ((v&7) == INT_TAG)
    {
        printf("%d", (int)((s32)(v >> 32)));
    }
    else if ((v&7) == STR_TAG)
    {   // needs to handle escaping to be correct
        printf("\"%s\"", DECODE_STR(v));
    }
    else if ((v&7) == SYM_TAG)
    {   // needs to handle escaping to be correct
        printf("%s", DECODE_SYM(v));
    }
    else if ((v&7) == OTHER_TAG
             && (VECTOR_OTHERTAG == (((u64*)DECODE_OTHER(v))[0] & 7)))
    {
        printf("#(");
        u64* vec = (u64*)DECODE_OTHER(v);
        u64 len = vec[0] >> 3;
        prim_print_aux(vec[1]);
        for (u64 i = 2; i <= len; ++i)
        {
            printf(",");
            prim_print_aux(vec[i]);
        }
        printf(")");
    }
    else
        printf("(print.. v); unrecognized value %lu", v);
    //...
    return V_VOID; 
}

u64 prim_print(u64 v) 
{
    if (v == V_NULL)
        printf("'()");
    else if ((v&7) == CLO_TAG)
        printf("#<procedure>");
    else if ((v&7) == CONS_TAG)
    {
        u64* p = (u64*)(v&(7ULL^MASK64));
        printf("'(");
        prim_print_aux(p[0]);
        printf(" . ");
        prim_print_aux(p[1]);
        printf(")");
    }
    else if ((v&7) == INT_TAG)
    {
        printf("%d", ((s32)(v >> 32)));
    }
    else if ((v&7) == STR_TAG)
    {   // needs to handle escaping to be correct
        printf("\"%s\"", DECODE_STR(v));
    }
    else if ((v&7) == SYM_TAG)
    {   // needs to handle escaping to be correct
        printf("'%s", DECODE_SYM(v));
    }
    else if ((v&7) == OTHER_TAG
             && (VECTOR_OTHERTAG == (((u64*)DECODE_OTHER(v))[0] & 7)))
    {
        printf("#(");
        u64* vec = (u64*)DECODE_OTHER(v);
        u64 len = vec[0] >> 3;
        prim_print(vec[1]);
        for (u64 i = 2; i <= len; ++i)
        {
            printf(",");
            prim_print(vec[i]);
        }
        printf(")");
    }
	else if ((v&7) == OTHER_TAG 
			&& (HASH_OTHERTAG == (((my_hash*)DECODE_OTHER(v))->tag))) 
	{
		my_hash *hash = (my_hash*)DECODE_OTHER(v);
		my_hash *s;

		printf("#hash(");
		for(s=hash; s != NULL; s=(my_hash *)s->hh.next) {
			printf("(");
			prim_print(s->id);
			printf(" . ");
			prim_print(s->val);
			printf(")");
			if(s->hh.next != NULL)
				printf(" ");
		}
		printf(")");

	}
    else
        printf("(print v); unrecognized value %lu", v);
    //...
    return V_VOID; 
}
GEN_EXPECT1ARGLIST(applyprim_print,prim_print)


u64 prim_halt(u64 v) // halt
{
    prim_print(v); // display the final value
    printf("\n");
    exit(0);
    return V_NULL; 
}


u64 applyprim_vector(u64 lst)
{
    // pretty terrible, but works
    u64* buffer = (u64*)malloc(512*sizeof(u64));
    u64 l = 0;
    while ((lst&7) == CONS_TAG && l < 512) 
        buffer[l++] = expect_cons(lst, &lst);
    u64* mem = alloc((l + 1) * sizeof(u64));
	check_mem((l+1) * sizeof(u64));
    mem[0] = (l << 3) | VECTOR_OTHERTAG;
    for (u64 i = 0; i < l; ++i)
        mem[i+1] = buffer[i];
    delete [] buffer;
    return ENCODE_OTHER(mem);
}

u64 prim_hash_63(u64 v)
{
	if ((v&7) == OTHER_TAG 
			&& (HASH_OTHERTAG == (((my_hash*)DECODE_OTHER(v))->tag)))
	{
		return V_TRUE;
	} else 
	{
		return V_FALSE;
	}
}

u64 prim_hash_45has_45key_63(u64 h, u64 k)
{

	ASSERT_TAG(h, OTHER_TAG, "first argument to hash-has-key? must be a hash");

	if(HASH_OTHERTAG != (((my_hash*)DECODE_OTHER(h))->tag))
		fatal_err("hash-has-key? not given a properly formed hash");

	my_hash *hash = (my_hash*)DECODE_OTHER(h);

	my_hash * s = get_val(&hash, k);

	if(s == NULL)
		return V_FALSE;
	else
		return V_TRUE;

}

u64 prim_hash_45keys_45subset_63(u64 h1, u64 h2)
{
	ASSERT_TAG(h1, OTHER_TAG, "first argument to hash-ref must be a hash");

	if(HASH_OTHERTAG != (((my_hash*)DECODE_OTHER(h1))->tag))
		fatal_err("hash-ref not given a properly formed hash");

	ASSERT_TAG(h2, OTHER_TAG, "first argument to hash-ref must be a hash");

	if(HASH_OTHERTAG != (((my_hash*)DECODE_OTHER(h2))->tag))
		fatal_err("hash-ref not given a properly formed hash");

	my_hash *hash1 = (my_hash*)DECODE_OTHER(h1);
	//my_hash *hash2 = (my_hash*)DECODE_OTHER(h2);

	my_hash *cur, *tmp;

	HASH_ITER(hh, hash1, cur, tmp)
	{
		if(prim_hash_45has_45key_63(h2, cur->id) == V_FALSE)
			return V_FALSE;
	}

	return V_TRUE;
}

u64 prim_hash_45count(u64 h)
{
	ASSERT_TAG(h, OTHER_TAG, "first argument to hash-ref must be a hash");

	if(HASH_OTHERTAG != (((my_hash*)DECODE_OTHER(h))->tag))
		fatal_err("hash-ref not given a properly formed hash");

	my_hash *hash = (my_hash*)DECODE_OTHER(h);

	s64 cnt = HASH_CNT(hh, hash) - 1;

	return ENCODE_INT(cnt);
}

u64 prim_hash_45empty_63(u64 h)
{
	ASSERT_TAG(h, OTHER_TAG, "first argument to hash-ref must be a hash");

	if(HASH_OTHERTAG != (((my_hash*)DECODE_OTHER(h))->tag))
		fatal_err("hash-ref not given a properly formed hash");

	my_hash *hash = (my_hash*)DECODE_OTHER(h);

	s64 cnt = HASH_CNT(hh, hash);

	if(cnt == 1)
		return V_TRUE;
	else 
		return V_FALSE;
}

u64 prim_hash_45clear_33(u64 h)
{
	ASSERT_TAG(h, OTHER_TAG, "first argument to hash-ref must be a hash");

	if(HASH_OTHERTAG != (((my_hash*)DECODE_OTHER(h))->tag))
		fatal_err("hash-ref not given a properly formed hash");

	my_hash *hash = (my_hash*)DECODE_OTHER(h);

	my_hash *cur, *tmp;

	HASH_ITER(hh, hash, cur, tmp)
	{
		if(cur->id != V_HASH)
		{
			HASH_DELETE(hh, hash, cur);
			free(cur);
			curmem -= sizeof(my_hash);
		}
	}

	return V_VOID;
}

u64 prim_hash_45remove_33(u64 h, u64 key)
{
	ASSERT_TAG(h, OTHER_TAG, "first argument to hash-ref must be a hash");

	if(HASH_OTHERTAG != (((my_hash*)DECODE_OTHER(h))->tag))
		fatal_err("hash-ref not given a properly formed hash");

	my_hash *hash = (my_hash*)DECODE_OTHER(h);

	my_hash * s = get_val(&hash, key);

	if(s != NULL)
	{
		HASH_DELETE(hh, hash, s);
		free(s);
		curmem -= sizeof(my_hash);
	}

	return V_VOID;
}

u64 prim_make_45hash(u64 lst)
{
	my_hash *hash = NULL;

	//add default value to the hash to initialize it
	add_val(&hash, V_HASH, 0);

	u64* buffer = (u64*)malloc(512*sizeof(u64));
	u64 l = 0;
	while ((lst&7) == CONS_TAG && l < 512)
		buffer[l++] = expect_cons(lst, &lst);
	for (u64 i = 0; i < l; ++i)
	{
		u64 cur = buffer[i];
		u64* pair = expect_pair(cur);
		u64 key = pair[0];
		u64 val = pair[1];
		add_val(&hash, key, val);

	}
	
	hash->tag = HASH_OTHERTAG;	
	return ENCODE_OTHER(hash);
}

u64 prim_hash_45ref_45flag(u64 h, u64 key, u64 flag)
{
	ASSERT_TAG(h, OTHER_TAG, "first argument to hash-ref must be a hash");

	if(HASH_OTHERTAG != (((my_hash*)DECODE_OTHER(h))->tag))
		fatal_err("hash-ref not given a properly formed hash");

	my_hash *hash = (my_hash*)DECODE_OTHER(h);

	my_hash * s = get_val(&hash, key);

	if(s == NULL)
		return flag;

	return s->val;
}

u64 prim_hash_45ref(u64 h, u64 key)
{

	ASSERT_TAG(h, OTHER_TAG, "first argument to hash-ref must be a hash");

	if(HASH_OTHERTAG != (((my_hash*)DECODE_OTHER(h))->tag))
		fatal_err("hash-ref not given a properly formed hash");

	my_hash *hash = (my_hash*)DECODE_OTHER(h);

	my_hash * s = get_val(&hash, key);

	if(s == NULL)
		fatal_err("Key is not found in the hash");

	return s->val;

}

u64 prim_hash_45set_33(u64 h, u64 key, u64 val)
{
	my_hash *hash = NULL;

	ASSERT_TAG(h, OTHER_TAG, "first argument to hash-ref must be a hash");

	if(HASH_OTHERTAG != (((my_hash*)DECODE_OTHER(h))->tag))
		fatal_err("hash-ref not given a properly formed hash");

	hash = (my_hash*)DECODE_OTHER(h);
	
	add_val(&hash, key, val);

	return V_VOID;

}

u64 prim_make_45vector(u64 lenv, u64 iv)
{
    ASSERT_TAG(lenv, INT_TAG, "first argument to make-vector must be an integer")
    
    const u64 l = DECODE_INT(lenv);
    u64* vec = (u64*)alloc((l + 1) * sizeof(u64));
	check_mem((l+1) * sizeof(u64));
    vec[0] = (l << 3) | VECTOR_OTHERTAG;
    for (u64 i = 1; i <= l; ++i)
        vec[i] = iv;
    return ENCODE_OTHER(vec);
}
GEN_EXPECT2ARGLIST(applyprim_make_45vector, prim_make_45vector)


u64 prim_vector_45ref(u64 v, u64 i)
{
    ASSERT_TAG(i, INT_TAG, "second argument to vector-ref must be an integer")
    ASSERT_TAG(v, OTHER_TAG, "first argument to vector-ref must be a vector")

    if ((((u64*)DECODE_OTHER(v))[0]&7) != VECTOR_OTHERTAG)
        fatal_err("vector-ref not given a properly formed vector");

	const u64 l = (((u64*)DECODE_OTHER(v))[0] >> 3);

	if(DECODE_INT(i) >= l)
		fatal_err("Vector-ref: Index Out of bounds");


    return ((u64*)DECODE_OTHER(v))[1+(DECODE_INT(i))];
}
GEN_EXPECT2ARGLIST(applyprim_vector_45ref, prim_vector_45ref)


u64 prim_vector_45set_33(u64 a, u64 i, u64 v)
{
    ASSERT_TAG(i, INT_TAG, "second argument to vector-ref must be an integer")
    ASSERT_TAG(a, OTHER_TAG, "first argument to vector-ref must be an integer")

    if ((((u64*)DECODE_OTHER(a))[0]&7) != VECTOR_OTHERTAG)
        fatal_err("vector-ref not given a properly formed vector");
	
	const u64 l = (((u64*)DECODE_OTHER(a))[0] >> 3);

	if(DECODE_INT(i) >= l)
		fatal_err("Vector-set: Index Out of Bounds");
        
    ((u64*)(DECODE_OTHER(a)))[1+DECODE_INT(i)] = v;
        
    return V_VOID;
}
GEN_EXPECT3ARGLIST(applyprim_vector_45set_33, prim_vector_45set_33)


///// void, ...

    
u64 prim_void()
{
    return V_VOID;
}


    



///// eq?, eqv?, equal?

    
u64 prim_eq_63(u64 a, u64 b)
{
    if (a == b)
        return V_TRUE;
    else
        return V_FALSE;
}
GEN_EXPECT2ARGLIST(applyprim_eq_63, prim_eq_63)


u64 prim_eqv_63(u64 a, u64 b)
{
    if (a == b)
        return V_TRUE;
    //else if  // optional extra logic, see r7rs reference
    else
        return V_FALSE;
}
GEN_EXPECT2ARGLIST(applyprim_eqv_63, prim_eqv_63)

/*
u64 prim_equal_63(u64 a, u64 b)
{
    return 0;
}
GEN_EXPECT2ARGLIST(applyprim_equal_63, prim_equal_63)
*/


///// Other predicates


u64 prim_number_63(u64 a)
{
    // We assume that ints are the only number
    if ((a&7) == INT_TAG)
        return V_TRUE;
    else
        return V_FALSE;
}
GEN_EXPECT1ARGLIST(applyprim_number_63, prim_number_63)


u64 prim_integer_63(u64 a)
{
    if ((a&7) == INT_TAG)
        return V_TRUE;
    else
        return V_FALSE;
}
GEN_EXPECT1ARGLIST(applyprim_integer_63, prim_integer_63)


u64 prim_void_63(u64 a)
{
    if (a == V_VOID)
        return V_TRUE;
    else
        return V_FALSE;
}
GEN_EXPECT1ARGLIST(applyprim_void_63, prim_void_63)


u64 prim_procedure_63(u64 a)
{
    if ((a&7) == CLO_TAG)
        return V_TRUE;
    else
        return V_FALSE;
}
GEN_EXPECT1ARGLIST(applyprim_procedure_63, prim_procedure_63)


///// null?, cons?, cons, car, cdr


u64 prim_null_63(u64 p) // null?
{
    if (p == V_NULL)
        return V_TRUE;
    else
        return V_FALSE;
}
GEN_EXPECT1ARGLIST(applyprim_null_63, prim_null_63)    


u64 prim_cons_63(u64 p) // cons?
{
    if ((p&7) == CONS_TAG)
        return V_TRUE;
    else
        return V_FALSE;
}
GEN_EXPECT1ARGLIST(applyprim_cons_63, prim_cons_63)    


u64 prim_cons(u64 a, u64 b)
{
    u64* p = alloc(2*sizeof(u64));
    p[0] = a;
    p[1] = b;
    return ENCODE_CONS(p);
}
GEN_EXPECT2ARGLIST(applyprim_cons, prim_cons)


u64 prim_car(u64 p)
{
    u64 rest;
    u64 v0 = expect_cons(p,&rest);
    
    return v0;
}
GEN_EXPECT1ARGLIST(applyprim_car, prim_car)


u64 prim_cdr(u64 p)
{
    u64 rest;
    u64 v0 = expect_cons(p,&rest);
    
    return rest;
}
GEN_EXPECT1ARGLIST(applyprim_cdr, prim_cdr)


///// s32 prims, +, -, *, =, ...

    
u64 prim__43(u64 a, u64 b) // +
{
    ASSERT_TAG(a, INT_TAG, "(prim + a b); a is not an integer")
    ASSERT_TAG(b, INT_TAG, "(prim + a b); b is not an integer")

        //printf("sum: %d\n", DECODE_INT(a) + DECODE_INT(b));
    
    return ENCODE_INT(DECODE_INT(a) + DECODE_INT(b));
}

u64 applyprim__43(u64 p)
{
    if (p == V_NULL)
        return ENCODE_INT(0);
    else
    {
        ASSERT_TAG(p, CONS_TAG, "Tried to apply + on non list value.")
        u64* pp = DECODE_CONS(p);
        return ENCODE_INT(DECODE_INT(pp[0]) + DECODE_INT(applyprim__43(pp[1])));
    }
}
    
u64 prim__45(u64 a, u64 b) // -
{
    ASSERT_TAG(a, INT_TAG, "(prim + a b); a is not an integer")
    ASSERT_TAG(b, INT_TAG, "(prim - a b); b is not an integer")
    
    return ENCODE_INT(DECODE_INT(a) - DECODE_INT(b));
}

u64 applyprim__45(u64 p)
{
    if (p == V_NULL)
        return ENCODE_INT(0);
    else
    {
        ASSERT_TAG(p, CONS_TAG, "Tried to apply + on non list value.")
        u64* pp = DECODE_CONS(p);
        if (pp[1] == V_NULL)
            return ENCODE_INT(0 - DECODE_INT(pp[0]));
        else // ideally would be properly left-to-right
            return ENCODE_INT(DECODE_INT(pp[0]) - DECODE_INT(applyprim__43(pp[1])));
    }
}
    
u64 prim__42(u64 a, u64 b) // *
{
    ASSERT_TAG(a, INT_TAG, "(prim * a b); a is not an integer")
    ASSERT_TAG(b, INT_TAG, "(prim * a b); b is not an integer")
    
    return ENCODE_INT(DECODE_INT(a) * DECODE_INT(b));
}

u64 applyprim__42(u64 p)
{
    if (p == V_NULL)
        return ENCODE_INT(1);
    else
    {
        ASSERT_TAG(p, CONS_TAG, "Tried to apply + on non list value.")
        u64* pp = DECODE_CONS(p);
        return ENCODE_INT(DECODE_INT(pp[0]) * DECODE_INT(applyprim__42(pp[1])));
    }
}
    
u64 prim__47(u64 a, u64 b) // /
{
    ASSERT_TAG(a, INT_TAG, "(prim / a b); a is not an integer")
    ASSERT_TAG(b, INT_TAG, "(prim / a b); b is not an integer")
	ASSERT_VALUE_NOT(DECODE_INT(b), 0, "Divide By Zero Exception")
    
    return ENCODE_INT(DECODE_INT(a) / DECODE_INT(b));
}
    
u64 prim__61(u64 a, u64 b)  // =
{
    ASSERT_TAG(a, INT_TAG, "(prim = a b); a is not an integer")
    ASSERT_TAG(b, INT_TAG, "(prim = a b); b is not an integer")
        
    if ((s32)((a&(7ULL^MASK64)) >> 32) == (s32)((b&(7ULL^MASK64)) >> 32))
        return V_TRUE;
    else
        return V_FALSE;
}

u64 prim__60(u64 a, u64 b) // <
{
    ASSERT_TAG(a, INT_TAG, "(prim < a b); a is not an integer")
    ASSERT_TAG(b, INT_TAG, "(prim < a b); b is not an integer")
    
    if ((s32)((a&(7ULL^MASK64)) >> 32) < (s32)((b&(7ULL^MASK64)) >> 32))
        return V_TRUE;
    else
        return V_FALSE;
}
    
u64 prim__60_61(u64 a, u64 b) // <=
{
    ASSERT_TAG(a, INT_TAG, "(prim <= a b); a is not an integer")
    ASSERT_TAG(b, INT_TAG, "(prim <= a b); b is not an integer")
        
    if ((s32)((a&(7ULL^MASK64)) >> 32) <= (s32)((b&(7ULL^MASK64)) >> 32))
        return V_TRUE;
    else
        return V_FALSE;
}

u64 prim_not(u64 a) 
{
    if (a == V_FALSE)
        return V_TRUE;
    else
        return V_FALSE;
}
GEN_EXPECT1ARGLIST(applyprim_not, prim_not)



}




