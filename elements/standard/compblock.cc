/*
 * compblock.{cc,hh} -- element blocks packets based on ratio of rate
 * annotations set by IPRateMonitor.
 * Benjie Chen
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
#include "compblock.hh"
#include <click/error.hh>
#include <click/confparse.hh>
#include <click/click_ip.h>

CompareBlock::CompareBlock()
  : Element(1, 2), _fwd_weight(0), _rev_weight(1)
{
}

CompareBlock *
CompareBlock::clone() const
{
  return new CompareBlock;
}

int
CompareBlock::configure(const Vector<String> &conf, ErrorHandler *errh)
{
  _bad = 0;
  return cp_va_parse(conf, this, errh,
		     cpInteger, "forward weight", &_fwd_weight,
		     cpInteger, "reverse weight", &_rev_weight,
		     cpInteger, "threshold", &_thresh,
		     0);
}

void
CompareBlock::push(int, Packet *p)
{
  // int network = *((unsigned char *)&p->ip_header()->ip_src);
  int fwd = p->fwd_rate_anno();
  if (fwd < 1) fwd = 1;
  int rev = p->rev_rate_anno();
  if (rev < 1) rev = 1;
  if ((fwd > _thresh || rev > _thresh) &&
      _fwd_weight * fwd > _rev_weight * rev) {
    output(1).push(p);
  } else
    output(0).push(p);
}


// HANDLERS
int
CompareBlock::fwd_weight_write_handler(const String &conf, Element *e, 
    				       void *, ErrorHandler *errh)
{
  Vector<String> args;
  cp_argvec(conf, args);
  CompareBlock* me = (CompareBlock *) e;

  if(args.size() != 1) {
    return errh->error("expecting one integer");
  }
  int weight;
  if(!cp_integer(args[0], &weight)) {
    return errh->error("not an integer");
  }
  me->_fwd_weight = weight;
  return 0;
}

int
CompareBlock::rev_weight_write_handler(const String &conf, Element *e, 
    				       void *, ErrorHandler *errh)
{
  Vector<String> args;
  cp_argvec(conf, args);
  CompareBlock* me = (CompareBlock *) e;

  if(args.size() != 1) {
    return errh->error("expecting one integer");
  }
  int weight;
  if(!cp_integer(args[0], &weight)) {
    return errh->error("not an integer");
  }
  me->_rev_weight = weight;
  return 0;
}

int
CompareBlock::thresh_write_handler(const String &conf, Element *e, 
    				   void *, ErrorHandler *errh)
{
  Vector<String> args;
  cp_argvec(conf, args);
  CompareBlock* me = (CompareBlock *) e;

  if(args.size() != 1) {
    return errh->error("expecting one integer");
  }
  int thresh;
  if(!cp_integer(args[0], &thresh)) {
    return errh->error("not an integer");
  }
  me->_thresh = thresh;
  return 0;
}

String
CompareBlock::fwd_weight_read_handler(Element *e, void *)
{
  CompareBlock *me = (CompareBlock *) e;
  return String(me->_fwd_weight) + "\n";
}

String
CompareBlock::rev_weight_read_handler(Element *e, void *)
{
  CompareBlock *me = (CompareBlock *) e;
  return String(me->_rev_weight) + "\n";
}

String
CompareBlock::thresh_read_handler(Element *e, void *)
{
  CompareBlock *me = (CompareBlock *) e;
  return String(me->_thresh) + "\n";
}

void
CompareBlock::add_handlers()
{
  add_read_handler("fwd_weight", fwd_weight_read_handler, 0);
  add_write_handler("fwd_weight", fwd_weight_write_handler, 0);
  add_read_handler("rev_weight", rev_weight_read_handler, 0);
  add_write_handler("rev_weight", rev_weight_write_handler, 0);
}

EXPORT_ELEMENT(CompareBlock)


