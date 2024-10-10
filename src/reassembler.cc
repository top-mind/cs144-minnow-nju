#include "reassembler.hh"
// #include <iostream>

using namespace std;
void Reassembler::insert( uint64_t __first_index, string data, bool __is_last_substring ) { if (output_.writer().is_closed())
    return;
  const uint64_t
    _1 = output_.writer().bytes_pushed(),
    _2 = output_.writer().available_capacity(),
    _3 = data.size();

  if (__is_last_substring) {
    end_pos_ = __first_index + _3;
    end_pos_known_ = true;

    if (output_.writer().bytes_pushed() == end_pos_)
      output_.writer().close();
  }

  uint64_t
    l = max(__first_index, _1),
    r = min(__first_index + _3, _1 + _2 ),
    _l = l - __first_index,
    len = r - l;

  if (l >= r) return;

  bool is_trunc = _l != 0 || len != _3;

  // insert
  // ----  ------
  //   ------
  // l r ?
  // + + cover?
  // - +
  // + -
  // - -
  
  auto _4 = mdata.lower_bound(l), _5 = _4;
  // cover case
  if (_4 != mdata.end() && _4->first == l && l + _4->second.size() >= r)
    return;

  // erase all datas covered by @__data
  for (; _5 != mdata.end() && _5->first + _5->second.size() <= r; _5++)
    size_ -= _5->second.size();
  mdata.erase(_4, _5);

  // update truncate
  _4 = _5;
  if (_4 != mdata.begin()) {
    --_4;
    uint64_t _4r = _4->first + _4->second.size();
    if (_4r >= r)
      return;
    if (l < _4r) {
      l = _4r;
      _l = l - __first_index;
      len = r - l;
      is_trunc = true;
    }
  }

  if (_5 != mdata.end()) {
    if (r > _5->first) {
      r = _5->first;
      len = r - l;
      is_trunc = true;
    }
  }

  mdata.emplace_hint(_5, l, is_trunc ? data.substr(_l, len) : move(data));
  size_ += len;

  // write
  while (!mdata.empty()) {
    auto i = mdata.begin();
    auto [idx, str] = *i;
    if (idx == output_.writer().bytes_pushed()) {
      size_ -= str.size();
      output_.writer().push(move(str));
      mdata.erase(i);
    } else {
      break;
    }
  }

  if (end_pos_known_ && output_.writer().bytes_pushed() == end_pos_)
    output_.writer().close();
}

uint64_t Reassembler::bytes_pending() const
{
  return size_;
}
