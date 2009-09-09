/*****************************************************************************

  TBuffer.cpp -- Buffer implementation

 *****************************************************************************/
#include "TBuffer.h"

//---------------------------------------------------------------------------

TBuffer::TBuffer()
{
  SetMaxBufferSize(DEFAULT_BUFFER_DEPTH);
}

//---------------------------------------------------------------------------

void TBuffer::SetMaxBufferSize(const unsigned int bms)
{
  assert(bms > 0);

  max_buffer_size = bms;
}

//---------------------------------------------------------------------------

unsigned int TBuffer::GetMaxBufferSize() const
{
  return max_buffer_size;
}

//---------------------------------------------------------------------------

bool TBuffer::IsFull() const
{
  return buffer.size() == max_buffer_size;
}

//---------------------------------------------------------------------------

bool TBuffer::IsEmpty() const
{
  return buffer.size() == 0;
}

//---------------------------------------------------------------------------

void TBuffer::Drop(const TPacket& packet) const
{
  assert(false);
}

//---------------------------------------------------------------------------

void TBuffer::Empty() const
{
  assert(false);
}

//---------------------------------------------------------------------------

void TBuffer::Push(const TPacket& packet)
{
  if (IsFull())
    Drop(packet);
  else
    buffer.push(packet);
}

//---------------------------------------------------------------------------

TPacket TBuffer::Pop()
{
  TPacket f;

  if (IsEmpty())
    Empty();
  else
    {
      f = buffer.front();
      buffer.pop();
    }

  return f;
}

//---------------------------------------------------------------------------

TPacket TBuffer::Front() const
{
  TPacket f;

  if (IsEmpty())
    Empty();
  else
    f = buffer.front();

  return f;
}

//---------------------------------------------------------------------------

unsigned int TBuffer::Size() const
{
  return buffer.size();
}

//---------------------------------------------------------------------------
unsigned int TBuffer::getCurrentFreeSlots() const
{
    return (GetMaxBufferSize()-Size());
}
//---------------------------------------------------------------------------
