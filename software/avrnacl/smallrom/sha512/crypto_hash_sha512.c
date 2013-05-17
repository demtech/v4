/*
 * File:   avrnacl-20130415/smallrom/sha512/crypto_hash_sha512.c
 * Author: Michael Hutter, Peter Schwabe
 * Public Domain
 */

#include <avr/pgmspace.h> 

#include "crypto_hash_sha512.h"
#include "bigint.h"

typedef struct{
  unsigned char v[8];
} myu64;

#define blocks crypto_hashblocks_sha512

extern void myu64_convert_bigendian(unsigned char *r, const unsigned char *x, unsigned char length);
extern void Ch(myu64 *r, const myu64 *x, const myu64 *y, const myu64 *z);
extern void Maj(myu64 *r, const myu64 *x, const myu64 *y, const myu64 *z);
extern void Sigma(myu64 *r, const myu64 *x, unsigned char c1, unsigned char c2, unsigned char c3);
extern void sigma(myu64 *r, const myu64 *x, unsigned char c1, unsigned char c2, unsigned char c3);
extern void M(myu64 *w0, const myu64 *w14, const myu64 *w9, const myu64 *w1);
extern void expand(myu64 *w);

const unsigned char iv[64] PROGMEM = {
  0x6a,0x09,0xe6,0x67,0xf3,0xbc,0xc9,0x08,
  0xbb,0x67,0xae,0x85,0x84,0xca,0xa7,0x3b,
  0x3c,0x6e,0xf3,0x72,0xfe,0x94,0xf8,0x2b,
  0xa5,0x4f,0xf5,0x3a,0x5f,0x1d,0x36,0xf1,
  0x51,0x0e,0x52,0x7f,0xad,0xe6,0x82,0xd1,
  0x9b,0x05,0x68,0x8c,0x2b,0x3e,0x6c,0x1f,
  0x1f,0x83,0xd9,0xab,0xfb,0x41,0xbd,0x6b,
  0x5b,0xe0,0xcd,0x19,0x13,0x7e,0x21,0x79
} ;

const myu64 roundconstants_pgm[80] PROGMEM = {
{{0x22, 0xae, 0x28, 0xd7, 0x98, 0x2f, 0x8a, 0x42}},
{{0xcd, 0x65, 0xef, 0x23, 0x91, 0x44, 0x37, 0x71}},
{{0x2f, 0x3b, 0x4d, 0xec, 0xcf, 0xfb, 0xc0, 0xb5}},
{{0xbc, 0xdb, 0x89, 0x81, 0xa5, 0xdb, 0xb5, 0xe9}},
{{0x38, 0xb5, 0x48, 0xf3, 0x5b, 0xc2, 0x56, 0x39}},
{{0x19, 0xd0, 0x05, 0xb6, 0xf1, 0x11, 0xf1, 0x59}},
{{0x9b, 0x4f, 0x19, 0xaf, 0xa4, 0x82, 0x3f, 0x92}},
{{0x18, 0x81, 0x6d, 0xda, 0xd5, 0x5e, 0x1c, 0xab}},
{{0x42, 0x02, 0x03, 0xa3, 0x98, 0xaa, 0x07, 0xd8}},
{{0xbe, 0x6f, 0x70, 0x45, 0x01, 0x5b, 0x83, 0x12}},
{{0x8c, 0xb2, 0xe4, 0x4e, 0xbe, 0x85, 0x31, 0x24}},
{{0xe2, 0xb4, 0xff, 0xd5, 0xc3, 0x7d, 0x0c, 0x55}},
{{0x6f, 0x89, 0x7b, 0xf2, 0x74, 0x5d, 0xbe, 0x72}},
{{0xb1, 0x96, 0x16, 0x3b, 0xfe, 0xb1, 0xde, 0x80}},
{{0x35, 0x12, 0xc7, 0x25, 0xa7, 0x06, 0xdc, 0x9b}},
{{0x94, 0x26, 0x69, 0xcf, 0x74, 0xf1, 0x9b, 0xc1}},
{{0xd2, 0x4a, 0xf1, 0x9e, 0xc1, 0x69, 0x9b, 0xe4}},
{{0xe3, 0x25, 0x4f, 0x38, 0x86, 0x47, 0xbe, 0xef}},
{{0xb5, 0xd5, 0x8c, 0x8b, 0xc6, 0x9d, 0xc1, 0x0f}},
{{0x65, 0x9c, 0xac, 0x77, 0xcc, 0xa1, 0x0c, 0x24}},
{{0x75, 0x02, 0x2b, 0x59, 0x6f, 0x2c, 0xe9, 0x2d}},
{{0x83, 0xe4, 0xa6, 0x6e, 0xaa, 0x84, 0x74, 0x4a}},
{{0xd4, 0xfb, 0x41, 0xbd, 0xdc, 0xa9, 0xb0, 0x5c}},
{{0xb5, 0x53, 0x11, 0x83, 0xda, 0x88, 0xf9, 0x76}},
{{0xab, 0xdf, 0x66, 0xee, 0x52, 0x51, 0x3e, 0x98}},
{{0x10, 0x32, 0xb4, 0x2d, 0x6d, 0xc6, 0x31, 0xa8}},
{{0x3f, 0x21, 0xfb, 0x98, 0xc8, 0x27, 0x03, 0xb0}},
{{0xe4, 0x0e, 0xef, 0xbe, 0xc7, 0x7f, 0x59, 0xbf}},
{{0xc2, 0x8f, 0xa8, 0x3d, 0xf3, 0x0b, 0xe0, 0xc6}},
{{0x25, 0xa7, 0x0a, 0x93, 0x47, 0x91, 0xa7, 0xd5}},
{{0x6f, 0x82, 0x03, 0xe0, 0x51, 0x63, 0xca, 0x06}},
{{0x70, 0x6e, 0x0e, 0x0a, 0x67, 0x29, 0x29, 0x14}},
{{0xfc, 0x2f, 0xd2, 0x46, 0x85, 0x0a, 0xb7, 0x27}},
{{0x26, 0xc9, 0x26, 0x5c, 0x38, 0x21, 0x1b, 0x2e}},
{{0xed, 0x2a, 0xc4, 0x5a, 0xfc, 0x6d, 0x2c, 0x4d}},
{{0xdf, 0xb3, 0x95, 0x9d, 0x13, 0x0d, 0x38, 0x53}},
{{0xde, 0x63, 0xaf, 0x8b, 0x54, 0x73, 0x0a, 0x65}},
{{0xa8, 0xb2, 0x77, 0x3c, 0xbb, 0x0a, 0x6a, 0x76}},
{{0xe6, 0xae, 0xed, 0x47, 0x2e, 0xc9, 0xc2, 0x81}},
{{0x3b, 0x35, 0x82, 0x14, 0x85, 0x2c, 0x72, 0x92}},
{{0x64, 0x03, 0xf1, 0x4c, 0xa1, 0xe8, 0xbf, 0xa2}},
{{0x01, 0x30, 0x42, 0xbc, 0x4b, 0x66, 0x1a, 0xa8}},
{{0x91, 0x97, 0xf8, 0xd0, 0x70, 0x8b, 0x4b, 0xc2}},
{{0x30, 0xbe, 0x54, 0x06, 0xa3, 0x51, 0x6c, 0xc7}},
{{0x18, 0x52, 0xef, 0xd6, 0x19, 0xe8, 0x92, 0xd1}},
{{0x10, 0xa9, 0x65, 0x55, 0x24, 0x06, 0x99, 0xd6}},
{{0x2a, 0x20, 0x71, 0x57, 0x85, 0x35, 0x0e, 0xf4}},
{{0xb8, 0xd1, 0xbb, 0x32, 0x70, 0xa0, 0x6a, 0x10}},
{{0xc8, 0xd0, 0xd2, 0xb8, 0x16, 0xc1, 0xa4, 0x19}},
{{0x53, 0xab, 0x41, 0x51, 0x08, 0x6c, 0x37, 0x1e}},
{{0x99, 0xeb, 0x8e, 0xdf, 0x4c, 0x77, 0x48, 0x27}},
{{0xa8, 0x48, 0x9b, 0xe1, 0xb5, 0xbc, 0xb0, 0x34}},
{{0x63, 0x5a, 0xc9, 0xc5, 0xb3, 0x0c, 0x1c, 0x39}},
{{0xcb, 0x8a, 0x41, 0xe3, 0x4a, 0xaa, 0xd8, 0x4e}},
{{0x73, 0xe3, 0x63, 0x77, 0x4f, 0xca, 0x9c, 0x5b}},
{{0xa3, 0xb8, 0xb2, 0xd6, 0xf3, 0x6f, 0x2e, 0x68}},
{{0xfc, 0xb2, 0xef, 0x5d, 0xee, 0x82, 0x8f, 0x74}},
{{0x60, 0x2f, 0x17, 0x43, 0x6f, 0x63, 0xa5, 0x78}},
{{0x72, 0xab, 0xf0, 0xa1, 0x14, 0x78, 0xc8, 0x84}},
{{0xec, 0x39, 0x64, 0x1a, 0x08, 0x02, 0xc7, 0x8c}},
{{0x28, 0x1e, 0x63, 0x23, 0xfa, 0xff, 0xbe, 0x90}},
{{0xe9, 0xbd, 0x82, 0xde, 0xeb, 0x6c, 0x50, 0xa4}},
{{0x15, 0x79, 0xc6, 0xb2, 0xf7, 0xa3, 0xf9, 0xbe}},
{{0x2b, 0x53, 0x72, 0xe3, 0xf2, 0x78, 0x71, 0xc6}},
{{0x9c, 0x61, 0x26, 0xea, 0xce, 0x3e, 0x27, 0xca}},
{{0x07, 0xc2, 0xc0, 0x21, 0xc7, 0xb8, 0x86, 0xd1}},
{{0x1e, 0xeb, 0xe0, 0xcd, 0xd6, 0x7d, 0xda, 0xea}},
{{0x78, 0xd1, 0x6e, 0xee, 0x7f, 0x4f, 0x7d, 0xf5}},
{{0xba, 0x6f, 0x17, 0x72, 0xaa, 0x67, 0xf0, 0x06}},
{{0xa6, 0x98, 0xc8, 0xa2, 0xc5, 0x7d, 0x63, 0x0a}},
{{0xae, 0x0d, 0xf9, 0xbe, 0x04, 0x98, 0x3f, 0x11}},
{{0x1b, 0x47, 0x1c, 0x13, 0x35, 0x0b, 0x71, 0x1b}},
{{0x84, 0x7d, 0x04, 0x23, 0xf5, 0x77, 0xdb, 0x28}},
{{0x93, 0x24, 0xc7, 0x40, 0x7b, 0xab, 0xca, 0x32}},
{{0xbc, 0xbe, 0xc9, 0x15, 0x0a, 0xbe, 0x9e, 0x3c}},
{{0x4c, 0x0d, 0x10, 0x9c, 0xc4, 0x67, 0x1d, 0x43}},
{{0xb6, 0x42, 0x3e, 0xcb, 0xbe, 0xd4, 0xc5, 0x4c}},
{{0x2a, 0x7e, 0x65, 0xfc, 0x9c, 0x29, 0x7f, 0x59}},
{{0xec, 0xfa, 0xd6, 0x3a, 0xab, 0x6f, 0xcb, 0x5f}},
{{0x17, 0x58, 0x47, 0x4a, 0x8c, 0x19, 0x44, 0x6c}}};


static void myF(myu64 *state, const myu64 *w, int k_index)
{
  myu64 t, t1, t2;
  unsigned char i;
  Ch(&t, state+4, state+5, state+6);
  Sigma(&t1, state+4, 14, 18, 23);
  bigint_add64(t1.v, t1.v, (state+7)->v);  
  bigint_add64(t1.v, t1.v, t.v);  
  for(i=0;i<8;i++)
    t.v[i] = pgm_read_byte((unsigned char *)roundconstants_pgm+k_index*8+i);
  bigint_add64(t1.v, t1.v, t.v);  
  bigint_add64(t1.v, t1.v, w->v);  
  Sigma(&t2, state+0, 28, 34, 5);
  Maj(&t,state+0,state+1,state+2);
  bigint_add64(t2.v, t2.v, t.v);    
  *(state+7) = *(state+6);
  *(state+6) = *(state+5);
  *(state+5) = *(state+4);
  bigint_add64((state+4)->v, (state+3)->v, t1.v);  
  *(state+3) = *(state+2);
  *(state+2) = *(state+1);
  *(state+1) = *(state+0);
  bigint_add64((state+0)->v, t1.v, t2.v);
}

int crypto_hashblocks_sha512(unsigned char *statebytes,const unsigned char *in,crypto_uint16 inlen)
{
  myu64 state[8];
  myu64 state_safe[8];  
  myu64 w[16];
  unsigned char i;

  myu64_convert_bigendian(state[0].v, statebytes + 0, 8);
  for (i=0;i<8;i++)
    state_safe[i] = state[i];
	
  while (inlen >= 128) 
  {
    for(i=0;i<16;i++)
      myu64_convert_bigendian(w[i].v, in + 8*i, 1);
	 
    for(i=0;i<16;i++)
      myF(state, w+i, i);

    expand(w);

    for(i=0;i<16;i++)
      myF(state, w+i, i+16);

    expand(w);

    for(i=0;i<16;i++)
      myF(state, w+i, i+32);

    expand(w);

    for(i=0;i<16;i++)
      myF(state, w+i, i+48);

    expand(w);

    for(i=0;i<16;i++)
      myF(state, w+i, i+64);
	
	for (i=0;i<8;i++) {
		bigint_add64(state[i].v, state[i].v, state_safe[i].v);
		state_safe[i] = state[i];
	}

    in += 128;
    inlen -= 128;
  }
 
  myu64_convert_bigendian(statebytes+0, state_safe[0].v, 8);

  return 0;
}

int crypto_hash_sha512(unsigned char *out,const unsigned char *in,crypto_uint16 inlen)
{
  unsigned char h[64];
  unsigned char padded[256];
  crypto_uint16 i;
  
  for (i = 0;i < 64;++i) h[i] = pgm_read_byte(iv+i);

  blocks(h,in,inlen);
  in += inlen;
  inlen &= 127;
  in -= inlen;

  for (i = 0;i < inlen;++i) padded[i] = in[i];
  padded[inlen] = 0x80;

  if (inlen < 112) {
	for (i = inlen + 1;i < 125;++i) padded[i] = 0;
	padded[125] = inlen >> 13;
	padded[126] = inlen >> 5;
	padded[127] = inlen << 3;	
    blocks(h,padded,128);
  } else {
	for (i = inlen + 1;i < 253;++i) padded[i] = 0;
	padded[253] = inlen >> 13;
	padded[254] = inlen >> 5;
	padded[255] = inlen << 3;	
    blocks(h,padded,256);
  }

  for (i = 0;i < 64;++i) out[i] = h[i];

  return 0;
}
