/*
 * des.{cc,hh} -- element implements IPsec encryption using DES
 * Alex Snoeren
 * contains code from other sources; see below
 *
 * Copyright (c) 1999-2000 Massachusetts Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * Further elaboration of this license, including a DISCLAIMER OF ANY
 * WARRANTY, EXPRESS OR IMPLIED, is provided in the LICENSE file, which is
 * also accessible at http://www.pdos.lcs.mit.edu/click/license.html
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#ifndef HAVE_IPSEC
# error "Must #define HAVE_IPSEC in config.h"
#endif
#include "des.hh"
#include "esp.hh"
#include <click/ipaddress.hh>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/glue.hh>

Des::Des()
  : Element(1, 1), _decrypt(-1)
{
}

Des::~Des()
{
}

Des::Des(int decrypt, unsigned char * key)
{
  add_input();
  add_output();
  _decrypt = decrypt;
  memcpy(_key, key, 8);
}

Des *
Des::clone() const
{
  return new Des(_decrypt, (unsigned char *)_key);
}

int
Des::configure(const Vector<String> &conf, ErrorHandler *errh)
{
  int dec_int;

  if (cp_va_parse(conf, this, errh,
		  cpInteger, "Encrypt/Decrypt (0/1)", &dec_int,
		  cpDesCblock, "8 byte IV", _iv,
		  cpDesCblock, "64-bit DES key", _key,
		  0) < 0)
    return -1;
  _decrypt = dec_int;
#ifdef DEBUG
  click_chatter("Key: %x%x%x%x%x%x%x%x",_key[0], _key[1], _key[2], _key[3],
	      _key[4], _key[5], _key[6], _key[7]);
#endif
  return 0;
}

int
Des::initialize(ErrorHandler *errh)
{
  if (_decrypt < 0)
    return errh->error("not configured");
  des_set_key(&_key, _ks);
  return 0;
}


Packet *
Des::simple_action(Packet *p_in)
{
  WritablePacket *p = p_in->uniqueify();
  unsigned char hold[8];
  unsigned char *idat = p->data();
  struct esp_new *esp = (struct esp_new *)p->data();

  unsigned char *ivp = esp->esp_iv;
  int i, plen = p->length() - sizeof(esp_new);
  idat = p->data() + sizeof(esp_new);
  
  // Copy in IV on encryption
  if(!_decrypt)
    memcpy(ivp, _iv, 8);

  // De/Encrypt the payload

  while (plen > 0) {

    if(_decrypt) {

      memcpy(hold, idat, 8);

      des_ecb_encrypt((des_cblock *)idat, (des_cblock *)idat,
		      _ks, DES_DECRYPT);

      /* CBC: XOR with the IV */

      for (i = 0; i < 8; i++) {
	idat[i] ^= ivp[i];
      }

      memcpy(ivp, hold, 8);
      
    } else {

      /* CBC: XOR with the IV */

      for (i = 0; i < 8; i++) {
	idat[i] ^= ivp[i];
      }
    
      des_ecb_encrypt((des_cblock *)idat, (des_cblock *)idat, 
		      _ks, DES_ENCRYPT);
      ivp = idat;
    }

    idat += 8;
    plen -= 8;
  
  }

  if(!_decrypt)
    //Save the last encrypted block, to be used as the next IV */
    memcpy(_iv, ivp, 8);

  return(p);
}

/* Copyright (C) 1992 Eric Young - see COPYING for more details */
/* Collected and modified by Werner Almesberger */

#define _	ks._

#define ITERATIONS 16

#define c2l(c,l)	(l =((unsigned long)(*((c)++)))    , \
			 l|=((unsigned long)(*((c)++)))<< 8, \
			 l|=((unsigned long)(*((c)++)))<<16, \
			 l|=((unsigned long)(*((c)++)))<<24)


#define l2c(l,c)	(*((c)++)=(unsigned char)(((l)    )&0xff), \
			 *((c)++)=(unsigned char)(((l)>> 8)&0xff), \
			 *((c)++)=(unsigned char)(((l)>>16)&0xff), \
			 *((c)++)=(unsigned char)(((l)>>24)&0xff))

/* from spr.h */

static unsigned long des_SPtrans[8][64]={
/* nibble 0 */
 {0x00820200, 0x00020000, 0x80800000, 0x80820200,
  0x00800000, 0x80020200, 0x80020000, 0x80800000,
  0x80020200, 0x00820200, 0x00820000, 0x80000200,
  0x80800200, 0x00800000, 0x00000000, 0x80020000,
  0x00020000, 0x80000000, 0x00800200, 0x00020200,
  0x80820200, 0x00820000, 0x80000200, 0x00800200,
  0x80000000, 0x00000200, 0x00020200, 0x80820000,
  0x00000200, 0x80800200, 0x80820000, 0x00000000,
  0x00000000, 0x80820200, 0x00800200, 0x80020000,
  0x00820200, 0x00020000, 0x80000200, 0x00800200,
  0x80820000, 0x00000200, 0x00020200, 0x80800000,
  0x80020200, 0x80000000, 0x80800000, 0x00820000,
  0x80820200, 0x00020200, 0x00820000, 0x80800200,
  0x00800000, 0x80000200, 0x80020000, 0x00000000,
  0x00020000, 0x00800000, 0x80800200, 0x00820200,
  0x80000000, 0x80820000, 0x00000200, 0x80020200},

/* nibble 1 */
 {0x10042004, 0x00000000, 0x00042000, 0x10040000,
  0x10000004, 0x00002004, 0x10002000, 0x00042000,
  0x00002000, 0x10040004, 0x00000004, 0x10002000,
  0x00040004, 0x10042000, 0x10040000, 0x00000004,
  0x00040000, 0x10002004, 0x10040004, 0x00002000,
  0x00042004, 0x10000000, 0x00000000, 0x00040004,
  0x10002004, 0x00042004, 0x10042000, 0x10000004,
  0x10000000, 0x00040000, 0x00002004, 0x10042004,
  0x00040004, 0x10042000, 0x10002000, 0x00042004,
  0x10042004, 0x00040004, 0x10000004, 0x00000000,
  0x10000000, 0x00002004, 0x00040000, 0x10040004,
  0x00002000, 0x10000000, 0x00042004, 0x10002004,
  0x10042000, 0x00002000, 0x00000000, 0x10000004,
  0x00000004, 0x10042004, 0x00042000, 0x10040000,
  0x10040004, 0x00040000, 0x00002004, 0x10002000,
  0x10002004, 0x00000004, 0x10040000, 0x00042000},

/* nibble 2 */
 {0x41000000, 0x01010040, 0x00000040, 0x41000040,
  0x40010000, 0x01000000, 0x41000040, 0x00010040,
  0x01000040, 0x00010000, 0x01010000, 0x40000000,
  0x41010040, 0x40000040, 0x40000000, 0x41010000,
  0x00000000, 0x40010000, 0x01010040, 0x00000040,
  0x40000040, 0x41010040, 0x00010000, 0x41000000,
  0x41010000, 0x01000040, 0x40010040, 0x01010000,
  0x00010040, 0x00000000, 0x01000000, 0x40010040,
  0x01010040, 0x00000040, 0x40000000, 0x00010000,
  0x40000040, 0x40010000, 0x01010000, 0x41000040,
  0x00000000, 0x01010040, 0x00010040, 0x41010000,
  0x40010000, 0x01000000, 0x41010040, 0x40000000,
  0x40010040, 0x41000000, 0x01000000, 0x41010040,
  0x00010000, 0x01000040, 0x41000040, 0x00010040,
  0x01000040, 0x00000000, 0x41010000, 0x40000040,
  0x41000000, 0x40010040, 0x00000040, 0x01010000},

/* nibble 3 */
 {0x00100402, 0x04000400, 0x00000002, 0x04100402,
  0x00000000, 0x04100000, 0x04000402, 0x00100002,
  0x04100400, 0x04000002, 0x04000000, 0x00000402,
  0x04000002, 0x00100402, 0x00100000, 0x04000000,
  0x04100002, 0x00100400, 0x00000400, 0x00000002,
  0x00100400, 0x04000402, 0x04100000, 0x00000400,
  0x00000402, 0x00000000, 0x00100002, 0x04100400,
  0x04000400, 0x04100002, 0x04100402, 0x00100000,
  0x04100002, 0x00000402, 0x00100000, 0x04000002,
  0x00100400, 0x04000400, 0x00000002, 0x04100000,
  0x04000402, 0x00000000, 0x00000400, 0x00100002,
  0x00000000, 0x04100002, 0x04100400, 0x00000400,
  0x04000000, 0x04100402, 0x00100402, 0x00100000,
  0x04100402, 0x00000002, 0x04000400, 0x00100402,
  0x00100002, 0x00100400, 0x04100000, 0x04000402,
  0x00000402, 0x04000000, 0x04000002, 0x04100400},

/* nibble 4 */
 {0x02000000, 0x00004000, 0x00000100, 0x02004108,
  0x02004008, 0x02000100, 0x00004108, 0x02004000,
  0x00004000, 0x00000008, 0x02000008, 0x00004100,
  0x02000108, 0x02004008, 0x02004100, 0x00000000,
  0x00004100, 0x02000000, 0x00004008, 0x00000108,
  0x02000100, 0x00004108, 0x00000000, 0x02000008,
  0x00000008, 0x02000108, 0x02004108, 0x00004008,
  0x02004000, 0x00000100, 0x00000108, 0x02004100,
  0x02004100, 0x02000108, 0x00004008, 0x02004000,
  0x00004000, 0x00000008, 0x02000008, 0x02000100,
  0x02000000, 0x00004100, 0x02004108, 0x00000000,
  0x00004108, 0x02000000, 0x00000100, 0x00004008,
  0x02000108, 0x00000100, 0x00000000, 0x02004108,
  0x02004008, 0x02004100, 0x00000108, 0x00004000,
  0x00004100, 0x02004008, 0x02000100, 0x00000108,
  0x00000008, 0x00004108, 0x02004000, 0x02000008},

/* nibble 5 */
 {0x20000010, 0x00080010, 0x00000000, 0x20080800,
  0x00080010, 0x00000800, 0x20000810, 0x00080000,
  0x00000810, 0x20080810, 0x00080800, 0x20000000,
  0x20000800, 0x20000010, 0x20080000, 0x00080810,
  0x00080000, 0x20000810, 0x20080010, 0x00000000,
  0x00000800, 0x00000010, 0x20080800, 0x20080010,
  0x20080810, 0x20080000, 0x20000000, 0x00000810,
  0x00000010, 0x00080800, 0x00080810, 0x20000800,
  0x00000810, 0x20000000, 0x20000800, 0x00080810,
  0x20080800, 0x00080010, 0x00000000, 0x20000800,
  0x20000000, 0x00000800, 0x20080010, 0x00080000,
  0x00080010, 0x20080810, 0x00080800, 0x00000010,
  0x20080810, 0x00080800, 0x00080000, 0x20000810,
  0x20000010, 0x20080000, 0x00080810, 0x00000000,
  0x00000800, 0x20000010, 0x20000810, 0x20080800,
  0x20080000, 0x00000810, 0x00000010, 0x20080010},

/* nibble 6 */
 {0x00001000, 0x00000080, 0x00400080, 0x00400001,
  0x00401081, 0x00001001, 0x00001080, 0x00000000,
  0x00400000, 0x00400081, 0x00000081, 0x00401000,
  0x00000001, 0x00401080, 0x00401000, 0x00000081,
  0x00400081, 0x00001000, 0x00001001, 0x00401081,
  0x00000000, 0x00400080, 0x00400001, 0x00001080,
  0x00401001, 0x00001081, 0x00401080, 0x00000001,
  0x00001081, 0x00401001, 0x00000080, 0x00400000,
  0x00001081, 0x00401000, 0x00401001, 0x00000081,
  0x00001000, 0x00000080, 0x00400000, 0x00401001,
  0x00400081, 0x00001081, 0x00001080, 0x00000000,
  0x00000080, 0x00400001, 0x00000001, 0x00400080,
  0x00000000, 0x00400081, 0x00400080, 0x00001080,
  0x00000081, 0x00001000, 0x00401081, 0x00400000,
  0x00401080, 0x00000001, 0x00001001, 0x00401081,
  0x00400001, 0x00401080, 0x00401000, 0x00001001},

/* nibble 7 */
 {0x08200020, 0x08208000, 0x00008020, 0x00000000,
  0x08008000, 0x00200020, 0x08200000, 0x08208020,
  0x00000020, 0x08000000, 0x00208000, 0x00008020,
  0x00208020, 0x08008020, 0x08000020, 0x08200000,
  0x00008000, 0x00208020, 0x00200020, 0x08008000,
  0x08208020, 0x08000020, 0x00000000, 0x00208000,
  0x08000000, 0x00200000, 0x08008020, 0x08200020,
  0x00200000, 0x00008000, 0x08208000, 0x00000020,
  0x00200000, 0x00008000, 0x08000020, 0x08208020,
  0x00008020, 0x08000000, 0x00000000, 0x00208000,
  0x08200020, 0x08008020, 0x08008000, 0x00200020,
  0x08208000, 0x00000020, 0x00200020, 0x08008000,
  0x08208020, 0x00200000, 0x08200000, 0x08000020,
  0x00208000, 0x00008020, 0x08008020, 0x08200000,
  0x00000020, 0x08208000, 0x00208020, 0x00000000,
  0x08000000, 0x08200020, 0x00008000, 0x00208020}};

/* from sk.h */

static unsigned long des_skb[8][64]={
/* for C bits (numbered as per FIPS 46) 1 2 3 4 5 6 */
 {0x00000000,0x00000010,0x20000000,0x20000010,
  0x00010000,0x00010010,0x20010000,0x20010010,
  0x00000800,0x00000810,0x20000800,0x20000810,
  0x00010800,0x00010810,0x20010800,0x20010810,
  0x00000020,0x00000030,0x20000020,0x20000030,
  0x00010020,0x00010030,0x20010020,0x20010030,
  0x00000820,0x00000830,0x20000820,0x20000830,
  0x00010820,0x00010830,0x20010820,0x20010830,
  0x00080000,0x00080010,0x20080000,0x20080010,
  0x00090000,0x00090010,0x20090000,0x20090010,
  0x00080800,0x00080810,0x20080800,0x20080810,
  0x00090800,0x00090810,0x20090800,0x20090810,
  0x00080020,0x00080030,0x20080020,0x20080030,
  0x00090020,0x00090030,0x20090020,0x20090030,
  0x00080820,0x00080830,0x20080820,0x20080830,
  0x00090820,0x00090830,0x20090820,0x20090830},
/* for C bits (numbered as per FIPS 46) 7 8 10 11 12 13 */
 {0x00000000,0x02000000,0x00002000,0x02002000,
  0x00200000,0x02200000,0x00202000,0x02202000,
  0x00000004,0x02000004,0x00002004,0x02002004,
  0x00200004,0x02200004,0x00202004,0x02202004,
  0x00000400,0x02000400,0x00002400,0x02002400,
  0x00200400,0x02200400,0x00202400,0x02202400,
  0x00000404,0x02000404,0x00002404,0x02002404,
  0x00200404,0x02200404,0x00202404,0x02202404,
  0x10000000,0x12000000,0x10002000,0x12002000,
  0x10200000,0x12200000,0x10202000,0x12202000,
  0x10000004,0x12000004,0x10002004,0x12002004,
  0x10200004,0x12200004,0x10202004,0x12202004,
  0x10000400,0x12000400,0x10002400,0x12002400,
  0x10200400,0x12200400,0x10202400,0x12202400,
  0x10000404,0x12000404,0x10002404,0x12002404,
  0x10200404,0x12200404,0x10202404,0x12202404},
/* for C bits (numbered as per FIPS 46) 14 15 16 17 19 20 */
 {0x00000000,0x00000001,0x00040000,0x00040001,
  0x01000000,0x01000001,0x01040000,0x01040001,
  0x00000002,0x00000003,0x00040002,0x00040003,
  0x01000002,0x01000003,0x01040002,0x01040003,
  0x00000200,0x00000201,0x00040200,0x00040201,
  0x01000200,0x01000201,0x01040200,0x01040201,
  0x00000202,0x00000203,0x00040202,0x00040203,
  0x01000202,0x01000203,0x01040202,0x01040203,
  0x08000000,0x08000001,0x08040000,0x08040001,
  0x09000000,0x09000001,0x09040000,0x09040001,
  0x08000002,0x08000003,0x08040002,0x08040003,
  0x09000002,0x09000003,0x09040002,0x09040003,
  0x08000200,0x08000201,0x08040200,0x08040201,
  0x09000200,0x09000201,0x09040200,0x09040201,
  0x08000202,0x08000203,0x08040202,0x08040203,
  0x09000202,0x09000203,0x09040202,0x09040203},
/* for C bits (numbered as per FIPS 46) 21 23 24 26 27 28 */
 {0x00000000,0x00100000,0x00000100,0x00100100,
  0x00000008,0x00100008,0x00000108,0x00100108,
  0x00001000,0x00101000,0x00001100,0x00101100,
  0x00001008,0x00101008,0x00001108,0x00101108,
  0x04000000,0x04100000,0x04000100,0x04100100,
  0x04000008,0x04100008,0x04000108,0x04100108,
  0x04001000,0x04101000,0x04001100,0x04101100,
  0x04001008,0x04101008,0x04001108,0x04101108,
  0x00020000,0x00120000,0x00020100,0x00120100,
  0x00020008,0x00120008,0x00020108,0x00120108,
  0x00021000,0x00121000,0x00021100,0x00121100,
  0x00021008,0x00121008,0x00021108,0x00121108,
  0x04020000,0x04120000,0x04020100,0x04120100,
  0x04020008,0x04120008,0x04020108,0x04120108,
  0x04021000,0x04121000,0x04021100,0x04121100,
  0x04021008,0x04121008,0x04021108,0x04121108},
/* for D bits (numbered as per FIPS 46) 1 2 3 4 5 6 */
 {0x00000000,0x10000000,0x00010000,0x10010000,
  0x00000004,0x10000004,0x00010004,0x10010004,
  0x20000000,0x30000000,0x20010000,0x30010000,
  0x20000004,0x30000004,0x20010004,0x30010004,
  0x00100000,0x10100000,0x00110000,0x10110000,
  0x00100004,0x10100004,0x00110004,0x10110004,
  0x20100000,0x30100000,0x20110000,0x30110000,
  0x20100004,0x30100004,0x20110004,0x30110004,
  0x00001000,0x10001000,0x00011000,0x10011000,
  0x00001004,0x10001004,0x00011004,0x10011004,
  0x20001000,0x30001000,0x20011000,0x30011000,
  0x20001004,0x30001004,0x20011004,0x30011004,
  0x00101000,0x10101000,0x00111000,0x10111000,
  0x00101004,0x10101004,0x00111004,0x10111004,
  0x20101000,0x30101000,0x20111000,0x30111000,
  0x20101004,0x30101004,0x20111004,0x30111004},
/* for D bits (numbered as per FIPS 46) 8 9 11 12 13 14 */
 {0x00000000,0x08000000,0x00000008,0x08000008,
  0x00000400,0x08000400,0x00000408,0x08000408,
  0x00020000,0x08020000,0x00020008,0x08020008,
  0x00020400,0x08020400,0x00020408,0x08020408,
  0x00000001,0x08000001,0x00000009,0x08000009,
  0x00000401,0x08000401,0x00000409,0x08000409,
  0x00020001,0x08020001,0x00020009,0x08020009,
  0x00020401,0x08020401,0x00020409,0x08020409,
  0x02000000,0x0A000000,0x02000008,0x0A000008,
  0x02000400,0x0A000400,0x02000408,0x0A000408,
  0x02020000,0x0A020000,0x02020008,0x0A020008,
  0x02020400,0x0A020400,0x02020408,0x0A020408,
  0x02000001,0x0A000001,0x02000009,0x0A000009,
  0x02000401,0x0A000401,0x02000409,0x0A000409,
  0x02020001,0x0A020001,0x02020009,0x0A020009,
  0x02020401,0x0A020401,0x02020409,0x0A020409},
/* for D bits (numbered as per FIPS 46) 16 17 18 19 20 21 */
 {0x00000000,0x00000100,0x00080000,0x00080100,
  0x01000000,0x01000100,0x01080000,0x01080100,
  0x00000010,0x00000110,0x00080010,0x00080110,
  0x01000010,0x01000110,0x01080010,0x01080110,
  0x00200000,0x00200100,0x00280000,0x00280100,
  0x01200000,0x01200100,0x01280000,0x01280100,
  0x00200010,0x00200110,0x00280010,0x00280110,
  0x01200010,0x01200110,0x01280010,0x01280110,
  0x00000200,0x00000300,0x00080200,0x00080300,
  0x01000200,0x01000300,0x01080200,0x01080300,
  0x00000210,0x00000310,0x00080210,0x00080310,
  0x01000210,0x01000310,0x01080210,0x01080310,
  0x00200200,0x00200300,0x00280200,0x00280300,
  0x01200200,0x01200300,0x01280200,0x01280300,
  0x00200210,0x00200310,0x00280210,0x00280310,
  0x01200210,0x01200310,0x01280210,0x01280310},
/* for D bits (numbered as per FIPS 46) 22 23 24 25 27 28 */
 {0x00000000,0x04000000,0x00040000,0x04040000,
  0x00000002,0x04000002,0x00040002,0x04040002,
  0x00002000,0x04002000,0x00042000,0x04042000,
  0x00002002,0x04002002,0x00042002,0x04042002,
  0x00000020,0x04000020,0x00040020,0x04040020,
  0x00000022,0x04000022,0x00040022,0x04040022,
  0x00002020,0x04002020,0x00042020,0x04042020,
  0x00002022,0x04002022,0x00042022,0x04042022,
  0x00000800,0x04000800,0x00040800,0x04040800,
  0x00000802,0x04000802,0x00040802,0x04040802,
  0x00002800,0x04002800,0x00042800,0x04042800,
  0x00002802,0x04002802,0x00042802,0x04042802,
  0x00000820,0x04000820,0x00040820,0x04040820,
  0x00000822,0x04000822,0x00040822,0x04040822,
  0x00002820,0x04002820,0x00042820,0x04042820,
  0x00002822,0x04002822,0x00042822,0x04042822}
};

/* from ecb_encrypt.c */

#define PERM_OP(a,b,t,n,m) ((t)=((((a)>>(n))^(b))&(m)),\
	(b)^=(t),\
	(a)=((a)^((t)<<(n))))

#define HPERM_OP(a,t,n,m) ((t)=((((a)<<(16-(n)))^(a))&(m)),\
	(a)=(a)^(t)^(t>>(16-(n))))

static char shifts2[16]={0,0,1,1,1,1,1,1,0,1,1,1,1,1,1,0};


#define D_ENCRYPT(L,R,S)	\
	u=(R^s[S  ]); \
	t=R^s[S+1]; \
	t=((t>>4)+(t<<28)); \
	L^=	des_SPtrans[1][(t    )&0x3f]| \
		des_SPtrans[3][(t>> 8)&0x3f]| \
		des_SPtrans[5][(t>>16)&0x3f]| \
		des_SPtrans[7][(t>>24)&0x3f]| \
		des_SPtrans[0][(u    )&0x3f]| \
		des_SPtrans[2][(u>> 8)&0x3f]| \
		des_SPtrans[4][(u>>16)&0x3f]| \
		des_SPtrans[6][(u>>24)&0x3f];


void
Des::des_encrypt(unsigned long *input,unsigned long *output,
		 des_key_schedule ks,int encrypt)
{
  register unsigned long l,r,t,u;
  register int i;
  register unsigned long *s;
  
  l=input[0];
  r=input[1];
  
  /* do IP */
  PERM_OP(r,l,t, 4,0x0f0f0f0f);
  PERM_OP(l,r,t,16,0x0000ffff);
  PERM_OP(r,l,t, 2,0x33333333);
  PERM_OP(l,r,t, 8,0x00ff00ff);
  PERM_OP(r,l,t, 1,0x55555555);
  /* r and l are reversed - remember that :-) - fix
   * it in the next step */
  
  /* Things have been modified so that the initial rotate is
   * done outside the loop.  This required the 
   * des_SPtrans values in sp.h to be rotated 1 bit to the right.
   * One perl script later and things have a 5% speed up on a sparc2.
   * Thanks to Richard Outerbridge <71755.204@CompuServe.COM>
   * for pointing this out. */
  t=(r<<1)|(r>>31);
  r=(l<<1)|(l>>31); 
  l=t;
  
  /* clear the top bits on machines with 8byte longs */
  l&=0xffffffff;
  r&=0xffffffff;
  
  s=(unsigned long *)ks;
  /* I don't know if it is worth the effort of loop unrolling the
   * inner loop */
  if (encrypt)
    {
      for (i=0; i<32; i+=4)
	{
	  D_ENCRYPT(l,r,i+0); /*  1 */
	  D_ENCRYPT(r,l,i+2); /*  2 */
	}
    }
  else
    {
      for (i=30; i>0; i-=4)
	{
	  D_ENCRYPT(l,r,i-0); /* 16 */
	  D_ENCRYPT(r,l,i-2); /* 15 */
	}
    }
  l=(l>>1)|(l<<31);
  r=(r>>1)|(r<<31);
  /* clear the top bits on machines with 8byte longs */
  l&=0xffffffff;
  r&=0xffffffff;
  
  /* swap l and r
   * we will not do the swap so just remember they are
   * reversed for the rest of the subroutine
   * luckily FP fixes this problem :-) */
  
  PERM_OP(r,l,t, 1,0x55555555);
  PERM_OP(l,r,t, 8,0x00ff00ff);
  PERM_OP(r,l,t, 2,0x33333333);
  PERM_OP(l,r,t,16,0x0000ffff);
  PERM_OP(r,l,t, 4,0x0f0f0f0f);

  output[0]=l;
  output[1]=r;
}

int
Des::des_ecb_encrypt(des_cblock *input,des_cblock *output,
		     des_key_schedule ks, int encrypt)
	{
	register unsigned long l0,l1;
	register unsigned char *in,*out;
	unsigned long ll[2];

	in=(unsigned char *)input;
	out=(unsigned char *)output;
	c2l(in,l0);
	c2l(in,l1);
	ll[0]=l0;
	ll[1]=l1;
	des_encrypt(ll,ll,ks,encrypt);
	l0=ll[0];
	l1=ll[1];
	l2c(l0,out);
	l2c(l1,out);
	return(0);
	}

/* from set_key.c */

void
Des::des_set_key(des_cblock *key,des_key_schedule schedule)
	{
	register unsigned long c,d,t,s;
	register unsigned char *in;
	register unsigned long *k;
	register int i;

	k=(unsigned long *)schedule;
	in=(unsigned char *)key;

	c2l(in,c);
	c2l(in,d);

	/* I now do it in 47 simple operations :-)
	 * Thanks to John Fletcher (john_fletcher@lccmail.ocf.llnl.gov)
	 * for the inspiration. :-) */
	PERM_OP (d,c,t,4,0x0f0f0f0f);
	HPERM_OP(c,t,-2,0xcccc0000);
	HPERM_OP(d,t,-2,0xcccc0000);
	PERM_OP (d,c,t,1,0x55555555);
	PERM_OP (c,d,t,8,0x00ff00ff);
	PERM_OP (d,c,t,1,0x55555555);
	d=	(((d&0x000000ff)<<16)| (d&0x0000ff00)     |
		 ((d&0x00ff0000)>>16)|((c&0xf0000000)>>4));
	c&=0x0fffffff;

	for (i=0; i<ITERATIONS; i++)
		{
		if (shifts2[i])
			{ c=((c>>2)|(c<<26)); d=((d>>2)|(d<<26)); }
		else
			{ c=((c>>1)|(c<<27)); d=((d>>1)|(d<<27)); }
		c&=0x0fffffff;
		d&=0x0fffffff;
		/* could be a few less shifts but I am to lazy at this
		 * point in time to investigate */
		s=	des_skb[0][ (c    )&0x3f                ]|
			des_skb[1][((c>> 6)&0x03)|((c>> 7)&0x3c)]|
			des_skb[2][((c>>13)&0x0f)|((c>>14)&0x30)]|
			des_skb[3][((c>>20)&0x01)|((c>>21)&0x06) |
						  ((c>>22)&0x38)];
		t=	des_skb[4][ (d    )&0x3f                ]|
			des_skb[5][((d>> 7)&0x03)|((d>> 8)&0x3c)]|
			des_skb[6][ (d>>15)&0x3f                ]|
			des_skb[7][((d>>21)&0x0f)|((d>>22)&0x30)];

		/* table contained 0213 4657 */
		*(k++)=((t<<16)|(s&0x0000ffff))&0xffffffff;
		s=     ((s>>16)|(t&0xffff0000));
		
		s=(s<<4)|(s>>28);
		*(k++)=s&0xffffffff;
		}
	}


ELEMENT_REQUIRES(false)
EXPORT_ELEMENT(Des)
