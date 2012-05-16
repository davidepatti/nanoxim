/*****************************************************************************

  TReservationTable.cpp -- TReservationTable implementation

 *****************************************************************************/
#include "nanoxim.h"
#include "TReservationTable.h"

//---------------------------------------------------------------------------

TReservationTable::TReservationTable()
{
  clear();
}

//---------------------------------------------------------------------------

void TReservationTable::clear()
{
  rtable.resize(DIRECTIONS+1);
  
  // note that NOT_VALID entries should remain untouched
  for (int i=0; i<DIRECTIONS+1; i++)
    if (rtable[i] != NOT_VALID) rtable[i] = NOT_RESERVED;
}

//---------------------------------------------------------------------------

bool TReservationTable::isAvailable(const int port_out) const
{
  assert(port_out >= 0 && port_out < DIRECTIONS+1);

  return ((rtable[port_out] == NOT_RESERVED));
}

//---------------------------------------------------------------------------

void TReservationTable::reserve(const int port_in, const int port_out)
{
    // reservation of reserved/not valid ports is illegal. Correctness
    // should be assured by TReservationTable users
    assert(isAvailable(port_out));

    // check for previous reservation to be released
  int port = getOutputPort(port_in);

  if (port!=NOT_RESERVED)
    release(port);

  rtable[port_out] = port_in;
}
//---------------------------------------------------------------------------

void TReservationTable::reserve(const int port_in, const vector<int>& port_out)
{
    // reservation of reserved/not valid ports is illegal. Correctness
    // should be assured by TReservationTable users

    for (int j=0;j<port_out.size();j++)
	assert(isAvailable(port_out[j]));

    // check for previous reservation to be released
  vector<int> ports = getMultiOutputPort(port_in);

  for (int j=0;j<ports.size();j++)
      if (ports[j]!=NOT_RESERVED) release(ports[j]);

  for (int j=0;j<port_out.size();j++)
      rtable[port_out[j]] = port_in;
}

//---------------------------------------------------------------------------

void TReservationTable::release(const int port_out)
{
  assert(port_out >= 0 && port_out < DIRECTIONS+1);
    // there is a valid reservation on port_out
  assert(rtable[port_out] >= 0 && rtable[port_out] < DIRECTIONS+1);

  rtable[port_out] = NOT_RESERVED;
}

//---------------------------------------------------------------------------

int TReservationTable::getOutputPort(const int port_in) const
{
  assert(port_in >= 0 && port_in < DIRECTIONS+1);

  for (int i=0; i<DIRECTIONS+1; i++)
    if (rtable[i] == port_in)
      return i; // port_in reserved outport i

  // semantic: port_in currently doesn't reserve any out port
  return NOT_RESERVED;
}
//---------------------------------------------------------------------------

vector<int> TReservationTable::getMultiOutputPort(const int port_in) const
{
    vector<int> dirs;
  assert(port_in >= 0 && port_in < DIRECTIONS+1);

  for (int i=0; i<DIRECTIONS+1; i++)
    if (rtable[i] == port_in)
       dirs.push_back(i);// port_in reserved outport i

  return dirs;

}
//---------------------------------------------------------------------------

// makes port_out no longer available for reservation/release
void TReservationTable::invalidate(const int port_out)
{
    rtable[port_out] = NOT_VALID;
}



