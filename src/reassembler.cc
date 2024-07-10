#include "reassembler.hh"
#include <iostream>

using namespace std;

auto Reassembler::split( uint64_t x )
{
  auto it = gap_strings_.lower_bound( { x, "" } );
  if ( it != gap_strings_.end() && it->first_index_ == x ) {
    return it;
  }

  if ( it == gap_strings_.begin() ) {
    return it;
  }

  --it;
  if ( x < it->end_index() ) {
    auto ret = gap_strings_.emplace_hint( it, x, it->data_.substr( x - it->first_index_ ) );
    it->data_.resize( x - it->first_index_ );
    return ret;
  }
  ++it;
  return it;
}

void Reassembler::debug()
{
  std::cerr << "-----\n";
  for ( const auto& gap : gap_strings_ ) {
    std::cerr << "[" << gap.first_index_ << " " << gap.end_index() << ")\n";
  }
  std::cerr << "-----\n";
  // uint64_t unassembled_index = writer().bytes_pushed();
  // uint64_t unaccentable_index = writer().bytes_pushed() + writer().available_capacity();
  // for (uint64_t i = unassembled_index; i < unaccentable_index; ++i) {
  //   auto it = gap_strings_.upper_bound({i, ""});
  //   if (it == gap_strings_.begin()) {
  //     std::cerr << "-";
  //     continue;
  //   }
  //   --it;
  //   if (i < it->end_index()) {
  //     std::cerr << it->data_[i - it->first_index_];
  //   } else {
  //     std::cerr << "-";
  //   }
  // }
  // std::cerr << std::endl;
}

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  uint64_t unaccentable_index = writer().bytes_pushed() + writer().available_capacity();
  uint64_t unassembled_index = writer().bytes_pushed();
  // std::cerr << "unassembled_index = " << unassembled_index << std::endl;
  // std::cerr << "unaccentable_index = " << unaccentable_index << std::endl;
  // std::cerr << "insert " << first_index << " " << data << " " << is_last_substring << std::endl;

  if ( first_index >= unaccentable_index ) {
    return;
  }

  if ( first_index < unassembled_index ) {
    if ( unassembled_index - first_index >= data.size() ) {
      return;
    }
    data = data.substr( unassembled_index - first_index );
    first_index = unassembled_index;
  }

  if ( first_index + data.size() >= unaccentable_index ) {
    data.erase( unaccentable_index - first_index );
    is_last_substring = false;
  }

  if ( is_last_substring && !is_exsited_last_byte_ ) {
    is_exsited_last_byte_ = true;
    last_index_ = first_index + data.size();
  }

  auto end = split( first_index + data.size() );
  auto begin = split( first_index );
  for ( auto it = begin; it != end; ++it ) {
    total_pending_bytes_ -= it->data_.size();
  }

  gap_strings_.erase( begin, end );
  gap_strings_.emplace( first_index, data );
  total_pending_bytes_ += data.size();

  if ( first_index == unassembled_index ) {
    auto it = gap_strings_.cbegin();
    while ( it != gap_strings_.cend() ) {
      std::string to_push_bytes;
      if ( it->first_index_ == first_index ) {
        to_push_bytes = std::move( it->data_ );
      } else {
        break;
      }

      total_pending_bytes_ -= to_push_bytes.size();
      first_index += to_push_bytes.size();
      output_.writer().push( std::move( to_push_bytes ) );
      if ( is_exsited_last_byte_ && first_index == last_index_ ) {
        output_.writer().close();
      }

      it = gap_strings_.erase( it );
    }
  }
}

uint64_t Reassembler::bytes_pending() const
{
  return total_pending_bytes_;
}
