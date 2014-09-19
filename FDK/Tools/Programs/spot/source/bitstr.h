#ifndef  bitstr_H
#define  bitstr_H

#define _bit_charsize   8
#define _bit_type       unsigned long
#define _bit_intsiz     (sizeof(_bit_type) * _bit_charsize)
#define _bit_0s         ((_bit_type)0)
#define _bit_1s         ((_bit_type)~(_bit_type)0)
#define _bit_size(N) \
        (((N) / _bit_intsiz) + (((N) % _bit_intsiz) ? 1 : 0))
#define _bit_intn(N) \
        ((N) / _bit_intsiz)
#define _bit_mask(N) \
        ((_bit_type)1 << ((N) % _bit_intsiz))

#define bit_decl(Name, N) \
        _bit_type Name[_bit_size(N)]

#define bit_declp(Name) \
        _bit_type *Name;

#define bit_calloc(Name, N) \
        Name = (_bit_type *)calloc(_bit_size(N), sizeof (_bit_type))

#define bit_free(Name) \
        if (Name){free(Name);} Name = NULL

#define bit_ref(Name) \
        _bit_type Name[];

#define bit_test(Name, N) \
        ((Name)[_bit_intn(N)] & _bit_mask(N))
#define bit_set(Name, N) \
        { (Name)[_bit_intn(N)] |= _bit_mask(N); }
#define bit_clear(Name, N) \
        { (Name)[_bit_intn(N)] &= ~_bit_mask(N); }

#endif
