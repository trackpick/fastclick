#ifndef SETIPCHECKSUM_HH
#define SETIPCHECKSUM_HH

/*
 * =c
 * SetIPChecksum()
 * =s sets IP packets' checksums
 * =d
 * Expects an IP packet as input.
 * Calculates the IP header's checksum and sets the checksum header field.
 *
 * You will not normally need SetIPChecksum. Most elements that modify an IP
 * header, like DecIPTTL, SetIPDSCP, and IPRewriter, already update the
 * checksum incrementally.
 *
 * =a CheckIPHeader, DecIPTTL, SetIPDSCP, IPRewriter */

#include <click/element.hh>
#include <click/glue.hh>

class SetIPChecksum : public Element {
public:
  SetIPChecksum();
  ~SetIPChecksum();
  
  const char *class_name() const		{ return "SetIPChecksum"; }
  const char *processing() const		{ return AGNOSTIC; }
  SetIPChecksum *clone() const;

  Packet *simple_action(Packet *);
};

#endif
