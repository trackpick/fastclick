/*
 * counter.{cc,hh} -- element counts packets, measures packet rate
 * Eddie Kohler
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
#include "counter.hh"
#include <click/confparse.hh>
#include <click/error.hh>

static String counter_read_rate_handler(Element *, void *);

Counter::Counter()
  : Element(1, 1), _count(0)
{
}

void
Counter::reset()
{
  _count = 0;
  _rate.initialize();
}

int
Counter::configure(const Vector<String> &conf, ErrorHandler *errh) 
{ 
  _bytes = false;
  String b = "PACKETS";
  if (cp_va_parse(conf, this, errh, 
		  cpOptional,
	          cpString, "count bytes?", &b,
		  0) < 0)
    return -1;
  if (b.upper() == "BYTES")
    _bytes = true;
  else if (b.upper() == "PACKETS")
    _bytes = false;
  else 
    return errh->error("argument should be \"bytes\" or \"packets\"");
  return 0;
}


int
Counter::initialize(ErrorHandler *)
{
  reset();
  return 0;
}

Packet *
Counter::simple_action(Packet *p)
{
  if (!_bytes) {
    _count++;
    _rate.update(1);
  } else {
    _count += p->length();
    _rate.update(p->length());
  }
  return p;
}

/*
void
Counter::push(int, Packet *packet)
{
  _count++;
  _rate.update(1);
  output(0).push(packet);
}

Packet *
Counter::pull(int)
{
  Packet *p = input(0).pull();
  if (p) {
    _count++;
    _rate.update(1);
  }
  return p;
}
*/

static String
counter_read_count_handler(Element *e, void *)
{
  Counter *c = (Counter *)e;
  return String(c->count()) + "\n";
}

static String
counter_read_rate_handler(Element *e, void *)
{
  Counter *c = (Counter *)e;
  return cp_unparse_real(c->rate()*c->rate_freq(), c->rate_scale()) + "\n";
}

static int
counter_reset_write_handler(const String &, Element *e, void *, ErrorHandler *)
{
  Counter *c = (Counter *)e;
  c->reset();
  return 0;
}

void
Counter::add_handlers()
{
  add_read_handler("count", counter_read_count_handler, 0);
  add_read_handler("rate", counter_read_rate_handler, 0);
  add_write_handler("reset", counter_reset_write_handler, 0);
}


EXPORT_ELEMENT(Counter)
