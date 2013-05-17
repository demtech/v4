/*
 * File:   avrnacl-20130415/smallrom/ed25519/ge25519.c
 * Author: Michael Hutter, Peter Schwabe
 * Public Domain
 */

#include "fe25519.h"
#include "sc25519.h"
#include "ge25519.h"
#include "print.h" //DEBUG

#include <avr/pgmspace.h>

/*
 * Arithmetic on the twisted Edwards curve -x^2 + y^2 = 1 + dx^2y^2
 * with d = -(121665/121666) = 37095705934669439343138083508754565189542113879843219016388785533085940283555
 * Base point: (15112221349535400772501151409588531511454012693041857206046113283949847762202,46316835694926478169428394003475163141307993866256225615783033603165251855960);
 */

/* d */
static const fe25519 ge25519_ecd = {{0xA3, 0x78, 0x59, 0x13, 0xCA, 0x4D, 0xEB, 0x75, 0xAB, 0xD8, 0x41, 0x41, 0x4D, 0x0A, 0x70, 0x00,
                                     0x98, 0xE8, 0x79, 0x77, 0x79, 0x40, 0xC7, 0x8C, 0x73, 0xFE, 0x6F, 0x2B, 0xEE, 0x6C, 0x03, 0x52}};
/* 2*d */
static const fe25519 ge25519_ec2d = {{0x59, 0xF1, 0xB2, 0x26, 0x94, 0x9B, 0xD6, 0xEB, 0x56, 0xB1, 0x83, 0x82, 0x9A, 0x14, 0xE0, 0x00,
                                      0x30, 0xD1, 0xF3, 0xEE, 0xF2, 0x80, 0x8E, 0x19, 0xE7, 0xFC, 0xDF, 0x56, 0xDC, 0xD9, 0x06, 0x24}};

  /* sqrt(-1) */
static const fe25519 ge25519_sqrtm1 = {{0xB0, 0xA0, 0x0E, 0x4A, 0x27, 0x1B, 0xEE, 0xC4, 0x78, 0xE4, 0x2F, 0xAD, 0x06, 0x18, 0x43, 0x2F,
	                                    0xA7, 0xD7, 0xFB, 0x3D, 0x99, 0x00, 0x4D, 0x2B, 0x0B, 0xDF, 0xC1, 0x4F, 0x80, 0x24, 0x83, 0x2B}};


#define ge25519_p3 ge25519

typedef struct
{
  fe25519 x;
  fe25519 z;
  fe25519 y;
  fe25519 t;
} ge25519_p1p1;

typedef struct
{
  fe25519 x;
  fe25519 y;
  fe25519 z;
} ge25519_p2;

typedef struct
{
  fe25519 x;
  fe25519 y;
} ge25519_aff;

const ge25519 ge25519_base PROGMEM = {{{0x1A, 0xD5, 0x25, 0x8F, 0x60, 0x2D, 0x56, 0xC9, 0xB2, 0xA7, 0x25, 0x95, 0x60, 0xC7, 0x2C, 0x69,
                                0x5C, 0xDC, 0xD6, 0xFD, 0x31, 0xE2, 0xA4, 0xC0, 0xFE, 0x53, 0x6E, 0xCD, 0xD3, 0x36, 0x69, 0x21}},
                              {{0x58, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
                                0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66}},
                              {{0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
                              {{0xA3, 0xDD, 0xB7, 0xA5, 0xB3, 0x8A, 0xDE, 0x6D, 0xF5, 0x52, 0x51, 0x77, 0x80, 0x9F, 0xF0, 0x20,
                                0x7D, 0xE3, 0xAB, 0x64, 0x8E, 0x4E, 0xEA, 0x66, 0x65, 0x76, 0x8B, 0xD7, 0x0F, 0x5F, 0x87, 0x67}}};


/* Multiples of the base point in affine representation */
static const ge25519_aff ge25519_base_multiples_affine[3] PROGMEM = {
#include "ge25519_base.data"
};

static void p1p1_to_p2(ge25519_p2 *r, const ge25519_p1p1 *p)
{
  fe25519_mul(&r->x,&p->x,&p->t);
  fe25519_mul(&r->y,&p->y,&p->z);
  fe25519_mul(&r->z,&p->z,&p->t);
}

static void p1p1_to_p3(ge25519_p3 *r, const ge25519_p1p1 *p)
{
  p1p1_to_p2((ge25519_p2 *)r, p);
  fe25519_mul(&r->t,&p->x,&p->y);
}

static void ge25519_mixadd2(ge25519_p3 *r, const ge25519_aff *q)
{
  fe25519 a,b,e,f,h;
  fe25519_mul(&f,&q->x,&q->y);
  fe25519_sub(&a,&r->y,&r->x);
  fe25519_add(&b,&r->y,&r->x);	
  fe25519_sub(&h,&q->y,&q->x);
  fe25519_add(&e,&q->y,&q->x);
  fe25519_mul(&a,&a,&h);
  fe25519_mul(&b,&b,&e);
  fe25519_sub(&e,&b,&a);
  fe25519_add(&h,&b,&a);
  fe25519_mul(&a,&r->t,&f);
  fe25519_mul(&a,&a,&ge25519_ec2d);
  fe25519_add(&b,&r->z,&r->z);
  fe25519_sub(&f,&b,&a);
  fe25519_add(&a,&b,&a);
  fe25519_mul(&r->x,&e,&f);
  fe25519_mul(&r->y,&h,&a);
  fe25519_mul(&r->z,&a,&f);
  /* fe25519_mul(&r->t,&e,&h); // Can remove this multiplication, as t is not used in doubling */
}

static void add_p1p1(ge25519_p1p1 *r, const ge25519_p3 *p, const ge25519_p3 *q)
{
  fe25519 a, b, c, d;
  fe25519_sub(&a,&p->y,&p->x);
  fe25519_sub(&c,&q->y,&q->x);
  fe25519_mul(&a,&a,&c);
  fe25519_add(&b,&p->x,&p->y);
  fe25519_add(&c,&q->x,&q->y);
  fe25519_mul(&b,&b,&c);
  fe25519_mul(&c,&p->t,&q->t);
  fe25519_mul(&c,&c,&ge25519_ec2d);
  fe25519_mul(&d,&p->z,&q->z);
  fe25519_add(&d,&d,&d);
  fe25519_sub(&r->x,&b,&a);
  fe25519_sub(&r->t,&d,&c);
  fe25519_add(&r->z,&d,&c);
  fe25519_add(&r->y,&b,&a);
}

/* See http://www.hyperelliptic.org/EFD/g1p/auto-twisted-extended-1.html#doubling-dbl-2008-hwcd */
static void dbl_p1p1(ge25519_p1p1 *r, const ge25519_p2 *p)
{
  fe25519 a,b,c,d;
  fe25519_mul(&a,&p->x,&p->x);
  fe25519_mul(&b,&p->y,&p->y);
  fe25519_mul(&c,&p->z,&p->z);
  fe25519_add(&c,&c,&c);
  fe25519_neg(&d, &a);
  fe25519_add(&r->x,&p->x,&p->y);
  fe25519_mul(&r->x,&r->x,&r->x);
  fe25519_sub(&r->x,&r->x,&a);
  fe25519_sub(&r->x,&r->x,&b);
  fe25519_add(&r->z,&d,&b);
  fe25519_sub(&r->t,&r->z,&c);
  fe25519_sub(&r->y,&d,&b);
}

static unsigned char negative(signed char b)
{
  unsigned long long x = b; /* 18446744073709551361..18446744073709551615: yes; 0..255: no */
  x >>= 63; /* 1: yes; 0: no */
  return x;
}

static void choose_t(ge25519_aff *t, ge25519_aff *base_multiples, signed char b)
{
  fe25519 v;
  unsigned char u;
  signed char mask = (*(unsigned char *)&b) >> 7;
  mask = -mask;
  //int i;
  u = (b + mask) ^ mask;
  *t = base_multiples[u];

  //*t = ge25519_base_multiples_affine[u];
  /*
  for (i=0;i<32;i++) {
	 (&t->x)->v[i] = pgm_read_byte(&(ge25519_base_multiples_affine[u].x.v[i]));
	 (&t->y)->v[i] = pgm_read_byte(&(ge25519_base_multiples_affine[u].y.v[i]));
  }
  */

  fe25519_neg(&v, &t->x);
  fe25519_cmov(&t->x, &v, negative(b));
}

static void setneutral(ge25519 *r)
{
  fe25519_setzero(&r->x);
  fe25519_setone(&r->y);
  fe25519_setone(&r->z);
  fe25519_setzero(&r->t);
}


/* ********************************************************************
 *                    EXPORTED FUNCTIONS
 ******************************************************************** */

/* return 0 on success, -1 otherwise */
int ge25519_unpackneg_vartime(ge25519_p3 *r, const unsigned char p[32])
{
  unsigned char par = p[31] >> 7;
  fe25519 t, chk, num, den, den2, den4, den6;

  fe25519_setone(&r->z);
  fe25519_unpack(&r->y, p);
  fe25519_mul(&num,&r->y,&r->y);
  fe25519_mul(&den,&num,&ge25519_ecd);

  fe25519_sub(&num,&num,&r->z);
  fe25519_add(&den,&r->z,&den);

  /* Computation of sqrt(num/den) */
  /* 1.: computation of num^((p-5)/8)*den^((7p-35)/8) = (num*den^7)^((p-5)/8) */
  fe25519_mul(&den2,&den,&den);
  fe25519_mul(&den4,&den2,&den2);
  fe25519_mul(&den6,&den4,&den2);
  fe25519_mul(&t,&den6,&num);
  fe25519_mul(&t,&t,&den);

  fe25519_pow2523(&t, &t);
  /* 2. computation of r->x = t * num * den^3 */
  fe25519_mul(&t,&t,&num);
  fe25519_mul(&t,&t,&den);
  fe25519_mul(&t,&t,&den);
  fe25519_mul(&r->x,&t,&den);

  /* 3. Check whether sqrt computation gave correct result, multiply by sqrt(-1) if not: */
  fe25519_mul(&chk,&r->x,&r->x);
  fe25519_mul(&chk,&chk,&den);
  if (!fe25519_iseq_vartime(&chk, &num))
    fe25519_mul(&r->x,&r->x,&ge25519_sqrtm1);

  /* 4. Now we have one of the two square roots, except if input was not a square */
  fe25519_mul(&chk,&r->x,&r->x);
  fe25519_mul(&chk,&chk,&den);
  if (!fe25519_iseq_vartime(&chk, &num))
    return -1;

  /* 5. Choose the desired square root according to parity: */
  if(fe25519_getparity(&r->x) != (1-par))
    fe25519_neg(&r->x, &r->x);

  fe25519_mul(&r->t,&r->x,&r->y);
  return 0;
}

void ge25519_pack(unsigned char r[32], const ge25519_p3 *p)
{

  fe25519 tx, ty, zi;
  fe25519_invert(&zi, &p->z);
  fe25519_mul(&tx,&p->x,&zi);
  fe25519_mul(&ty,&p->y,&zi);
  fe25519_pack(r, &ty);
  r[31] ^= fe25519_getparity(&tx) << 7;
}

int ge25519_isneutral_vartime(const ge25519_p3 *p)
{
  int ret = 1;
  if(!fe25519_iszero(&p->x)) ret = 0;
  if(!fe25519_iseq_vartime(&p->y, &p->z)) ret = 0;
  return ret;
}

/* computes [s1]p1 + [s2]p2 */
void ge25519_double_scalarmult_vartime(ge25519_p3 *r, const ge25519_p3 *p1, const sc25519 *s1, const sc25519 *s2)
{
  ge25519_p1p1 tp1p1;
  ge25519_p3 pre;
  unsigned char b[255];
  signed int i;
  ge25519 p2;

  for (i=0;i<32; i++) {
    p2.x.v[i]=pgm_read_byte(&(ge25519_base.x.v[i]));
    p2.y.v[i]=pgm_read_byte(&(ge25519_base.y.v[i]));
    p2.z.v[i]=pgm_read_byte(&(ge25519_base.z.v[i]));
    p2.t.v[i]=pgm_read_byte(&(ge25519_base.t.v[i]));
  }

  add_p1p1(&tp1p1,p1,&p2);
  p1p1_to_p3(&pre, &tp1p1);

  sc25519_2interleave1(b,s1,s2);

  if(b[254]==0)       setneutral(r);
  else if(b[254]==1)  *r = *p1;
  else if(b[254]==2)  *r = p2;
  else if(b[254]==3)  *r = pre;

  /* scalar multiplication */
  for(i=253;i>=0;i--)
  {
    dbl_p1p1(&tp1p1, (ge25519_p2 *)r);
    if(b[i]==1)
    {
      p1p1_to_p3(r, &tp1p1);
      add_p1p1(&tp1p1, r, p1);
    }
    else if(b[i]==2)
    {
      p1p1_to_p3(r, &tp1p1);
      add_p1p1(&tp1p1, r, &p2);
    }
    else if(b[i]==3)
    {
      p1p1_to_p3(r, &tp1p1);
      add_p1p1(&tp1p1, r, &pre);
    }

    if(i != 0) p1p1_to_p2((ge25519_p2 *)r, &tp1p1);
    else p1p1_to_p3(r, &tp1p1);
  }
}

void ge25519_scalarmult_base(ge25519_p3 *r, const sc25519 *s)
{
  signed char b[128];
  signed int i;       //2 byte
  ge25519_aff t;	   //64 bytes
  ge25519_p1p1 tp1p1;  //128 bytes
  sc25519_window2(b,s);

  ge25519_aff base_multiples[3];

  for(i=0;i<sizeof(base_multiples);i++)
    i[(char *)base_multiples] = pgm_read_byte((char *)ge25519_base_multiples_affine+i);

  choose_t((ge25519_aff *)r, base_multiples, b[127]);
  fe25519_setone(&r->z);
  fe25519_mul(&r->t,&r->x,&r->y);

  for(i=126;i>=0;i--)
  {
    /*
    dbl_p1p1(&tp1p1, (ge25519_p2 *)r);
    p1p1_to_p2((ge25519_p2 *)r, &tp1p1);
    dbl_p1p1(&tp1p1, (ge25519_p2 *)r);
    p1p1_to_p2((ge25519_p2 *)r, &tp1p1);
    */
    dbl_p1p1(&tp1p1, (ge25519_p2 *)r);
    p1p1_to_p2((ge25519_p2 *)r, &tp1p1);
    dbl_p1p1(&tp1p1, (ge25519_p2 *)r);
    p1p1_to_p3(r, &tp1p1);

    choose_t(&t, base_multiples, b[i]);
    ge25519_mixadd2(r, &t);
  }
}
