/*
 * FILE : ChildSafe_wrap.c
 * 
 * This file was automatically generated by :
 * Simplified Wrapper and Interface Generator (SWIG)
 * Version 1.1 (Patch 5)
 * 
 * Portions Copyright (c) 1995-1998
 * The University of Utah and The Regents of the University of California.
 * Permission is granted to distribute this file in any manner provided
 * this notice remains intact.
 * 
 * Do not make changes to this file--changes will be lost!
 *
 */

#define SWIGCODE
/* Implementation : PERL 5 */

#define SWIGPERL
#define SWIGPERL5
#ifdef __cplusplus
#include <math.h>
#include <stdlib.h>
extern "C" {
#endif
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#undef free
#undef malloc
#include <string.h>
#ifdef __cplusplus
}
#endif

/*
 * HAND EDIT!
 * This version has been ported to perl5.006 by removing old (-DPERL_POLLUTE)
 * symbols. The code below is a hack for backwards compatibility with
 * 5.005 and below.
 */
#ifdef sv_undef
#undef PL_sv_undef
#define PL_sv_undef sv_undef
#undef PL_sv_yes
#define PL_sv_yes sv_yes
#undef PL_na
#define PL_na na
#endif

/* Definitions for compiling Perl extensions on a variety of machines */
#if defined(WIN32) || defined(__WIN32__)
#   if defined(_MSC_VER)
#	define SWIGEXPORT(a,b) __declspec(dllexport) a b
#   else
#	if defined(__BORLANDC__)
#	    define SWIGEXPORT(a,b) a _export b
#	else
#	    define SWIGEXPORT(a,b) a b
#	endif
#   endif
#else
#   define SWIGEXPORT(a,b) a b
#endif

#ifdef PERL_OBJECT
#define MAGIC_PPERL  CPerl *pPerl = (CPerl *) this;
#define MAGIC_CAST   (int (CPerl::*)(SV *, MAGIC *))
#define SWIGCLASS_STATIC 
#else
#define MAGIC_PPERL
#define MAGIC_CAST
#define SWIGCLASS_STATIC static
#endif


/*****************************************************************************
 * $Header: /vobs_fw/ChildSafe/ChildSafe_wrap.c /main/2 17-Nov-1999 14:55:24 dsb $
 *
 * perl5ptr.swg
 *
 * This file contains supporting code for the SWIG run-time type checking
 * mechanism.  The following functions are available :
 *
 * SWIG_RegisterMapping(char *origtype, char *newtype, void *(*cast)(void *));
 *
 *      Registers a new type-mapping with the type-checker.  origtype is the
 *      original datatype and newtype is an equivalent type.  cast is optional
 *      pointer to a function to cast pointer values between types (this
 *      is only used to cast pointers from derived classes to base classes in C++)
 *      
 * SWIG_MakePtr(char *buffer, void *ptr, char *typestring);
 *     
 *      Makes a pointer string from a pointer and typestring.  The result is returned
 *      in buffer.
 *
 * char * SWIG_GetPtr(SV *obj, void **ptr, char *type)
 *
 *      Gets a pointer value from a Perl5 scalar value.  If there is a 
 *      type-mismatch, returns a character string to the received type.  
 *      On success, returns NULL.
 *
 *
 * You can remap these functions by making a file called "swigptr.swg" in
 * your the same directory as the interface file you are wrapping.
 *
 * These functions are normally declared static, but this file can be
 * can be used in a multi-module environment by redefining the symbol
 * SWIGSTATIC.
 *
 * $Log:  $
 * Revision 1.1  1996/12/26 22:17:29  beazley
 * Initial revision
 *
 *****************************************************************************/

#include <stdlib.h>

#ifdef SWIG_GLOBAL
#ifdef __cplusplus
#define SWIGSTATIC extern "C"
#else
#define SWIGSTATIC
#endif
#endif

#ifndef SWIGSTATIC
#define SWIGSTATIC static
#endif

/* These are internal variables.   Should be static */

typedef struct SwigPtrType {
  char               *name;
  int                 len;
  void               *(*cast)(void *);
  struct SwigPtrType *next;
} SwigPtrType;

/* Pointer cache structure */

typedef struct {
  int                 stat;               /* Status (valid) bit             */
  SwigPtrType        *tp;                 /* Pointer to type structure      */
  char                name[256];          /* Given datatype name            */
  char                mapped[256];        /* Equivalent name                */
} SwigCacheType;

static int SwigPtrMax  = 64;           /* Max entries that can be currently held */
static int SwigPtrN    = 0;            /* Current number of entries              */
static int SwigPtrSort = 0;            /* Status flag indicating sort            */
static SwigPtrType *SwigPtrTable = 0;  /* Table containing pointer equivalences  */
static int SwigStart[256];             /* Table containing starting positions    */

/* Cached values */

#define SWIG_CACHESIZE  8
#define SWIG_CACHEMASK  0x7
static SwigCacheType SwigCache[SWIG_CACHESIZE];  
static int SwigCacheIndex = 0;
static int SwigLastCache = 0;

/* Sort comparison function */
static int swigsort(const void *data1, const void *data2) {
	SwigPtrType *d1 = (SwigPtrType *) data1;
	SwigPtrType *d2 = (SwigPtrType *) data2;
	return strcmp(d1->name,d2->name);
}

/* Binary Search function */
static int swigcmp(const void *key, const void *data) {
  char *k = (char *) key;
  SwigPtrType *d = (SwigPtrType *) data;
  return strncmp(k,d->name,d->len);
}

/* Register a new datatype with the type-checker */

#ifndef PERL_OBJECT
SWIGSTATIC 
void SWIG_RegisterMapping(char *origtype, char *newtype, void *(*cast)(void *)) {
#else
SWIGSTATIC
#define SWIG_RegisterMapping(a,b,c) _SWIG_RegisterMapping(pPerl, a,b,c)
void _SWIG_RegisterMapping(CPerl *pPerl, char *origtype, char *newtype, void *(*cast)(void *)) {
#endif

  int i;
  SwigPtrType *t = 0, *t1;

  if (!SwigPtrTable) {     
    SwigPtrTable = (SwigPtrType *) malloc(SwigPtrMax*sizeof(SwigPtrType));
    SwigPtrN = 0;
  }
  if (SwigPtrN >= SwigPtrMax) {
    SwigPtrMax = 2*SwigPtrMax;
    SwigPtrTable = (SwigPtrType *) realloc(SwigPtrTable,SwigPtrMax*sizeof(SwigPtrType));
  }
  for (i = 0; i < SwigPtrN; i++)
    if (strcmp(SwigPtrTable[i].name,origtype) == 0) {
      t = &SwigPtrTable[i];
      break;
    }
  if (!t) {
    t = &SwigPtrTable[SwigPtrN];
    t->name = origtype;
    t->len = strlen(t->name);
    t->cast = 0;
    t->next = 0;
    SwigPtrN++;
  }
  while (t->next) {
    if (strcmp(t->name,newtype) == 0) {
      if (cast) t->cast = cast;
      return;
    }
    t = t->next;
  }
  t1 = (SwigPtrType *) malloc(sizeof(SwigPtrType));
  t1->name = newtype;
  t1->len = strlen(t1->name);
  t1->cast = cast;
  t1->next = 0;
  t->next = t1;
  SwigPtrSort = 0;
}

/* Make a pointer value string */

SWIGSTATIC 
void SWIG_MakePtr(char *_c, const void *_ptr, char *type) {
  static char _hex[16] =
  {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
   'a', 'b', 'c', 'd', 'e', 'f'};
  unsigned long _p, _s;
  char _result[20], *_r;    /* Note : a 64-bit hex number = 16 digits */
  _r = _result;
  _p = (unsigned long) _ptr;
  if (_p > 0) {
    while (_p > 0) {
      _s = _p & 0xf;
      *(_r++) = _hex[_s];
      _p = _p >> 4;
    }
    *_r = '_';
    while (_r >= _result)
      *(_c++) = *(_r--);
  } else {
    strcpy (_c, "NULL");
  }
  if (_ptr)
    strcpy (_c, type);
}

/* Define for backwards compatibility */

#define _swig_make_hex   SWIG_MakePtr 

/* Function for getting a pointer value */

#ifndef PERL_OBJECT
SWIGSTATIC 
char *SWIG_GetPtr(SV *sv, void **ptr, char *_t)
#else
SWIGSTATIC
#define SWIG_GetPtr(a,b,c) _SWIG_GetPtr(pPerl,a,b,c)
char *_SWIG_GetPtr(CPerl *pPerl, SV *sv, void **ptr, char *_t)
#endif
{
  char temp_type[256];
  char *name,*_c;
  int  len,i,start,end;
  IV   tmp;
  SwigPtrType *sp,*tp;
  SwigCacheType *cache;

  /* If magical, apply more magic */

  if (SvGMAGICAL(sv))
    mg_get(sv);

  /* Check to see if this is an object */
  if (sv_isobject(sv)) {
    SV *tsv = (SV*) SvRV(sv);
    if ((SvTYPE(tsv) == SVt_PVHV)) {
      MAGIC *mg;
      if (SvMAGICAL(tsv)) {
	mg = mg_find(tsv,'P');
	if (mg) {
	  SV *rsv = mg->mg_obj;
	  if (sv_isobject(rsv)) {
	    tmp = SvIV((SV*)SvRV(rsv));
	  }
	}
      } else {
	return "Not a valid pointer value";
      }
    } else {
      tmp = SvIV((SV*)SvRV(sv));
    }
    if (!_t) {
      *(ptr) = (void *) tmp;
      return (char *) 0;
    }
  } else if (sv == &PL_sv_undef) {            /* Check for undef */
    *(ptr) = (void *) 0;
    return (char *) 0;
  } else if (SvTYPE(sv) == SVt_RV) {       /* Check for NULL pointer */
    *(ptr) = (void *) 0;
    if (!SvROK(sv)) 
      return (char *) 0;
    else
      return "Not a valid pointer value";
  } else {                                 /* Don't know what it is */
      *(ptr) = (void *) 0;
      return "Not a valid pointer value";
  }
  if (_t) {
    /* Now see if the types match */      

    if (!sv_isa(sv,_t)) {
      _c = HvNAME(SvSTASH(SvRV(sv)));
      if (!SwigPtrSort) {
	qsort((void *) SwigPtrTable, SwigPtrN, sizeof(SwigPtrType), swigsort);  
	for (i = 0; i < 256; i++) {
	  SwigStart[i] = SwigPtrN;
	}
	for (i = SwigPtrN-1; i >= 0; i--) {
	  SwigStart[SwigPtrTable[i].name[0]] = i;
	}
	for (i = 255; i >= 1; i--) {
	  if (SwigStart[i-1] > SwigStart[i])
	    SwigStart[i-1] = SwigStart[i];
	}
	SwigPtrSort = 1;
	for (i = 0; i < SWIG_CACHESIZE; i++)  
	  SwigCache[i].stat = 0;
      }
      /* First check cache for matches.  Uses last cache value as starting point */
      cache = &SwigCache[SwigLastCache];
      for (i = 0; i < SWIG_CACHESIZE; i++) {
	if (cache->stat) {
	  if (strcmp(_t,cache->name) == 0) {
	    if (strcmp(_c,cache->mapped) == 0) {
	      cache->stat++;
	      *ptr = (void *) tmp;
	      if (cache->tp->cast) *ptr = (*(cache->tp->cast))(*ptr);
	      return (char *) 0;
	    }
	  }
	}
	SwigLastCache = (SwigLastCache+1) & SWIG_CACHEMASK;
	if (!SwigLastCache) cache = SwigCache;
	else cache++;
      }

      start = SwigStart[_t[0]];
      end = SwigStart[_t[0]+1];
      sp = &SwigPtrTable[start];
      while (start < end) {
	if (swigcmp(_t,sp) == 0) break;
	sp++;
	start++;
      }
      if (start >= end) sp = 0;
      if (sp) {
	while (swigcmp(_t,sp) == 0) {
	  name = sp->name;
	  len = sp->len;
	  tp = sp->next;
	  while(tp) {
	    if (tp->len >= 255) {
	      return _c;
	    }
	    strcpy(temp_type,tp->name);
	    strncat(temp_type,_t+len,255-tp->len);
	    if (sv_isa(sv,temp_type)) {
	      /* Get pointer value */
	      *ptr = (void *) tmp;
	      if (tp->cast) *ptr = (*(tp->cast))(*ptr);

	      strcpy(SwigCache[SwigCacheIndex].mapped,_c);
	      strcpy(SwigCache[SwigCacheIndex].name,_t);
	      SwigCache[SwigCacheIndex].stat = 1;
	      SwigCache[SwigCacheIndex].tp = tp;
	      SwigCacheIndex = SwigCacheIndex & SWIG_CACHEMASK;
	      return (char *) 0;
	    }
	    tp = tp->next;
	  } 
	  /* Hmmm. Didn't find it this time */
 	  sp++;
	}
      }
      /* Didn't find any sort of match for this data.  
	 Get the pointer value and return the received type */
      *ptr = (void *) tmp;
      return _c;
    } else {
      /* Found a match on the first try.  Return pointer value */
      *ptr = (void *) tmp;
      return (char *) 0;
    }
  } 
  *ptr = (void *) tmp;
  return (char *) 0;
}

/* Compatibility mode */

#define _swig_get_hex  SWIG_GetPtr
/* Magic variable code */
#ifndef PERL_OBJECT
#define swig_create_magic(s,a,b,c) _swig_create_magic(s,a,b,c)
static void _swig_create_magic(SV *sv, char *name, int (*set)(SV *, MAGIC *), int (*get)(SV *,MAGIC *)) {
#else
#define swig_create_magic(s,a,b,c) _swig_create_magic(pPerl,s,a,b,c)
static void _swig_create_magic(CPerl *pPerl, SV *sv, char *name, int (CPerl::*set)(SV *, MAGIC *), int (CPerl::*get)(SV *, MAGIC *)) {
#endif
  MAGIC *mg;
  sv_magic(sv,sv,'U',name,strlen(name));
  mg = mg_find(sv,'U');
  mg->mg_virtual = (MGVTBL *) malloc(sizeof(MGVTBL));
  mg->mg_virtual->svt_get = get;
  mg->mg_virtual->svt_set = set;
  mg->mg_virtual->svt_len = 0;
  mg->mg_virtual->svt_clear = 0;
  mg->mg_virtual->svt_free = 0;
}

#define SWIG_init    boot_IPC__ChildSafe

#define SWIG_name   "IPC::ChildSafe::boot_IPC__ChildSafe"
#define SWIG_varinit "IPC::ChildSafe::var_ChildSafe_init();"
#ifdef __cplusplus
extern "C"
#endif
#ifndef PERL_OBJECT
SWIGEXPORT(void,boot_IPC__ChildSafe)(CV* cv);
#else
SWIGEXPORT(void,boot_IPC__ChildSafe)(CPerl *, CV *cv);
#endif

#include "childsafe.h"
#ifdef PERL_OBJECT
#define MAGIC_CLASS _wrap_ChildSafe_var::
class _wrap_ChildSafe_var : public CPerl {
public:
#else
#define MAGIC_CLASS
#endif
SWIGCLASS_STATIC int swig_magic_readonly(SV *sv, MAGIC *mg) {
    MAGIC_PPERL
    sv = sv; mg = mg;
    croak("Value is read-only.");
    return 0;
}
SWIGCLASS_STATIC int _wrap_set_Debug_Level(SV* sv, MAGIC *mg) {


    MAGIC_PPERL
    mg = mg;
    Debug_Level = (int ) SvIV(sv);
    return 1;
}

SWIGCLASS_STATIC int _wrap_val_Debug_Level(SV *sv, MAGIC *mg) {


    MAGIC_PPERL
    mg = mg;
    sv_setiv(sv, (IV) Debug_Level);
    return 1;
}

SWIGCLASS_STATIC int _wrap_set_Alarm_Wait(SV* sv, MAGIC *mg) {


    MAGIC_PPERL
    mg = mg;
    Alarm_Wait = (int ) SvIV(sv);
    return 1;
}

SWIGCLASS_STATIC int _wrap_val_Alarm_Wait(SV *sv, MAGIC *mg) {


    MAGIC_PPERL
    mg = mg;
    sv_setiv(sv, (IV) Alarm_Wait);
    return 1;
}



#ifdef PERL_OBJECT
};
#endif

XS(_wrap_child_open) {

    CHILD * _result;
    char * _arg0;
    char * _arg1;
    char * _arg2;
    int argvi = 0;
    dXSARGS ;

    cv = cv;
    if ((items < 3) || (items > 3)) 
        croak("Usage: child_open(char *,char *,char *);");
    _arg0 = (char *) SvPV(ST(0),PL_na);
    _arg1 = (char *) SvPV(ST(1),PL_na);
    _arg2 = (char *) SvPV(ST(2),PL_na);
    _result = (CHILD *)child_open(_arg0,_arg1,_arg2);
    ST(argvi) = sv_newmortal();
    sv_setref_pv(ST(argvi++),"CHILDPtr", (void *) _result);
    XSRETURN(argvi);
}

XS(_wrap_child_puts) {

    int  _result;
    char * _arg0;
    CHILD * _arg1;
    int argvi = 0;
    dXSARGS ;

    cv = cv;
    if ((items < 2) || (items > 2)) 
        croak("Usage: child_puts(char *,CHILD *);");
    _arg0 = (char *) SvPV(ST(0),PL_na);
    if (SWIG_GetPtr(ST(1),(void **) &_arg1,"CHILDPtr")) {
        croak("Type error in argument 2 of child_puts. Expected CHILDPtr.");
        XSRETURN(1);
    }
    _result = (int )child_puts(_arg0,_arg1);
    ST(argvi) = sv_newmortal();
    sv_setiv(ST(argvi++),(IV) _result);
    XSRETURN(argvi);
}

XS(_wrap_child_get_stdout_perl) {

    char * _result;
    CHILD * _arg0;
    int argvi = 0;
    dXSARGS ;

    cv = cv;
    if ((items < 1) || (items > 1)) 
        croak("Usage: child_get_stdout_perl(CHILD *);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"CHILDPtr")) {
        croak("Type error in argument 1 of child_get_stdout_perl. Expected CHILDPtr.");
        XSRETURN(1);
    }
    _result = (char *)child_get_stdout_perl(_arg0);
    ST(argvi) = sv_newmortal();
    sv_setpv((SV*)ST(argvi++),(char *) _result);
free(_result);

    XSRETURN(argvi);
}

XS(_wrap_child_get_stderr_perl) {

    char * _result;
    CHILD * _arg0;
    int argvi = 0;
    dXSARGS ;

    cv = cv;
    if ((items < 1) || (items > 1)) 
        croak("Usage: child_get_stderr_perl(CHILD *);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"CHILDPtr")) {
        croak("Type error in argument 1 of child_get_stderr_perl. Expected CHILDPtr.");
        XSRETURN(1);
    }
    _result = (char *)child_get_stderr_perl(_arg0);
    ST(argvi) = sv_newmortal();
    sv_setpv((SV*)ST(argvi++),(char *) _result);
free(_result);

    XSRETURN(argvi);
}

XS(_wrap_child_close) {

    int  _result;
    CHILD * _arg0;
    int argvi = 0;
    dXSARGS ;

    cv = cv;
    if ((items < 1) || (items > 1)) 
        croak("Usage: child_close(CHILD *);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"CHILDPtr")) {
        croak("Type error in argument 1 of child_close. Expected CHILDPtr.");
        XSRETURN(1);
    }
    _result = (int )child_close(_arg0);
    ST(argvi) = sv_newmortal();
    sv_setiv(ST(argvi++),(IV) _result);
    XSRETURN(argvi);
}

XS(_wrap_child_kill) {

    int  _result;
    CHILD * _arg0;
    int  _arg1;
    int argvi = 0;
    dXSARGS ;

    cv = cv;
    if ((items < 2) || (items > 2)) 
        croak("Usage: child_kill(CHILD *,int );");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"CHILDPtr")) {
        croak("Type error in argument 1 of child_kill. Expected CHILDPtr.");
        XSRETURN(1);
    }
    _arg1 = (int )SvIV(ST(1));
    _result = (int )child_kill(_arg0,_arg1);
    ST(argvi) = sv_newmortal();
    sv_setiv(ST(argvi++),(IV) _result);
    XSRETURN(argvi);
}

XS(_wrap_perl5_ChildSafe_var_init) {
    dXSARGS;
    SV *sv;
    cv = cv; items = items;
    sv = perl_get_sv("Debug_Level",TRUE | 0x2);
    sv_setiv(sv,(IV)Debug_Level);
    swig_create_magic(sv,"Debug_Level", MAGIC_CAST MAGIC_CLASS _wrap_set_Debug_Level, MAGIC_CAST MAGIC_CLASS _wrap_val_Debug_Level);
    sv = perl_get_sv("Alarm_Wait",TRUE | 0x2);
    sv_setiv(sv,(IV)Alarm_Wait);
    swig_create_magic(sv,"Alarm_Wait", MAGIC_CAST MAGIC_CLASS _wrap_set_Alarm_Wait, MAGIC_CAST MAGIC_CLASS _wrap_val_Alarm_Wait);
    XSRETURN(1);
}
#ifdef __cplusplus
extern "C"
#endif
XS(boot_IPC__ChildSafe) {
	 dXSARGS;
	 char *file = __FILE__;
	 cv = cv; items = items;
	 newXS("IPC::ChildSafe::var_ChildSafe_init", _wrap_perl5_ChildSafe_var_init, file);
	 newXS("IPC::ChildSafe::child_open", _wrap_child_open, file);
	 newXS("IPC::ChildSafe::child_puts", _wrap_child_puts, file);
	 newXS("IPC::ChildSafe::child_get_stdout_perl", _wrap_child_get_stdout_perl, file);
	 newXS("IPC::ChildSafe::child_get_stderr_perl", _wrap_child_get_stderr_perl, file);
	 newXS("IPC::ChildSafe::child_close", _wrap_child_close, file);
	 newXS("IPC::ChildSafe::child_kill", _wrap_child_kill, file);
/*
 * These are the pointer type-equivalency mappings. 
 * (Used by the SWIG pointer type-checker).
 */
	 SWIG_RegisterMapping("unsigned short","short",0);
	 SWIG_RegisterMapping("long","unsigned long",0);
	 SWIG_RegisterMapping("long","signed long",0);
	 SWIG_RegisterMapping("signed short","short",0);
	 SWIG_RegisterMapping("signed int","int",0);
	 SWIG_RegisterMapping("short","unsigned short",0);
	 SWIG_RegisterMapping("short","signed short",0);
	 SWIG_RegisterMapping("unsigned long","long",0);
	 SWIG_RegisterMapping("int","unsigned int",0);
	 SWIG_RegisterMapping("int","signed int",0);
	 SWIG_RegisterMapping("unsigned int","int",0);
	 SWIG_RegisterMapping("signed long","long",0);
	 ST(0) = &PL_sv_yes;
	 XSRETURN(1);
}
