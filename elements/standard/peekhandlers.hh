#ifndef PEEKHANDLERS_HH
#define PEEKHANDLERS_HH
#include <click/element.hh>
#include <click/timer.hh>

/*
 * =c
 * PeekHandlers([DELAY or HANDLER] ...)
 * =s
 * calls read handlers at specified times
 * V<debugging>
 * =io
 * None
 * =d
 *
 * Runs read handlers automatically at specified times. Each configuration
 * argument should be either `DELAY', a delay in seconds;
 * `ELEMENT.HANDLERNAME', a directive to call ELEMENT's HANDLERNAME read
 * handler and report the result; or `quit', a directive to stop the driver.
 * PeekHandlers processes its arguments in order, reading handlers as they
 * appear. A `DELAY' directive causes it to wait for DELAY seconds before
 * continuing.
 *
 * At user level, the results of any read handlers are printed to standard
 * error. In the Linux kernel module, they are printed to /var/log/messages
 * (accessible through dmesg(1)) and to /proc/click/errors.
 * =e
 *   PeekHandlers(q.length,
 *                1.5, // delay for 1.5 seconds
 *                q.length);
 * =a PokeHandlers, dmesg(1) */

class PeekHandlers : public Element {

  int _pos;
  Vector<Element *> _h_element;
  Vector<String> _h_handler;
  Vector<int> _h_timeout;
  Timer _timer;

  static void timer_hook(unsigned long);
  
 public:

  PeekHandlers();
  ~PeekHandlers();

  const char *class_name() const		{ return "PeekHandlers"; }
  PeekHandlers *clone() const			{ return new PeekHandlers; }
  int configure(const Vector<String> &, ErrorHandler *);
  int initialize(ErrorHandler *);
  void uninitialize();

};

#endif
