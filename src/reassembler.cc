#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  uint64_t unaccentable_index = writer().bytes_pushed() + writer().available_capacity();
  uint64_t unassembled_index = writer().bytes_pushed();

  if (first_index >= unaccentable_index) {
    return;
  }
  if (first_index < unassembled_index) {
    if (unassembled_index - first_index >= data.size()) {
      return;
    }
    data = data.substr(unassembled_index - first_index);
    first_index = unassembled_index;
  }

  if (first_index + data.size() >= unaccentable_index) {
    data.erase(unaccentable_index - first_index);
    is_last_substring = false;
  }

  is_exsited_last_byte_ |= is_last_substring;

  if (first_index != unassembled_index) {
    // the first iterator >= range_string{first_index, ""};
    // auto it = gap_strings_.lower_bound({first_index, {}});
    // if (it == gap_strings_.end()) {
    // }
  }

  while (first_index == unassembled_index) {
    output_.writer().push(std::move(data));
    first_index += data.size();
    auto it = gap_strings_.cbegin();
    while (it != gap_strings_.cend()) {
      std::string to_push_bytes;
      if (it->first_index_ == first_index) {
        to_push_bytes = std::move(it->data_);
      } else if (it->first_index_ < first_index) {
        to_push_bytes = it->data_.substr(first_index - it->first_index_);
      } else {
        break;
      }
      first_index += to_push_bytes.size();
      output_.writer().push(std::move(to_push_bytes));
      it = gap_strings_.erase(it);
    }
  }
}

uint64_t Reassembler::bytes_pending() const
{
  return total_pending_bytes_;
}
