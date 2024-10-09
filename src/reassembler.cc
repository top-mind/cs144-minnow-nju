#include "reassembler.hh"

using namespace std;


/* Reassembler
 * |<-readed zone->|<-ByteStream->|<-Reassembler->|
 *                  <-         capacity         ->
 * |<-Reassembler->|
 * = (<unknown><known>)*<unknown>?
 *
 */

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // Your code here.
  (void)first_index;
  (void)data;
  (void)is_last_substring;
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return {};
}
