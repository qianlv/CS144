# Stanford CS 144 Networking Lab

These labs are open to the public under the (friendly) request that to
preserve their value as a teaching tool, solutions not be posted
publicly by anybody.

Website: https://cs144.stanford.edu

To set up the build system: `cmake -S . -B build`

To compile: `cmake --build build`

To run tests: `cmake --build build --target test`

To run speed benchmarks: `cmake --build build --target speed`

To run clang-tidy (which suggests improvements): `cmake --build build --target tidy`

To format code: `cmake --build build --target format`

## Checkpoint 0

Use the structure of `std::string buffer_[2]` to cyclically store the data. Both buffer* 2 strings are capacity.
In `Reader::pop`, when buffer*[0] is empty, `std::string::swap(buffer_[0], buffer_[1])` to reuse buffer*[0]
In `Writer::push`, need to check if buffer*[0] is enough space to store new data, otherness you need split the data into buffer*[0] and buffer*[1]

reference: using `std::queue<std::string>` to implement，[Checkpoint0](https://zhuanlan.zhihu.com/p/683162988)

## Checkpoint 1

1. keep the insert data in range [unassembled_index, unaccepted_index),
   unassembled_index equal to push bytes at bytes buffer(`bytes_pushed`)
   unaccepted_index = unassembled_index + bytes buffer available capacity(`available_capacity`)
   to keep the window size is bytes buffer capacity
2. using `std::set<string_range>`, `string_range` is the range of bytes with the start index.
   keeping `std::set<string_range> gap_string_` all `string_range` not overlap and order.
   Insert [l, r] string range, just split at r + 1 and l two position, then erase [l, r] all string range, then
   insert [l, r] string range. For example:
   [1, 4], [6, 8], [10, 20] now insert [3, 6], we split `gap_string_` at position (6 + 1) = 7 and 3 to get
   [1, 2], [3, 4], [6, 6], [7, 8], [10, 20], then erase [3, 4], [6, 6] from `gap_string_` then insert [3, 6]

reference: [odt tree](https://oi-wiki.org/misc/odt/)

## Checkpoint 2

1. Convert absolute seqno to relative seqno, just (zero point + absolute seqno) % 2^32
2. Convert seqno to absolute seqno.
   2.1 Get the distance between seqno and zero point as offset
   2.2 use checkpoint to the multile of 2^32 as nwrap, now offset = (offset + nwrap * 2^32),
       then if offset < checkpoint, choose from (offset, offset + 2^32) that min distance to checkpoint
       else choose from (offset - 2^32, offset) that min distance to checkpoint

3. `TCPReceiver::receive`, attention to first_index is absolute seqno - 1
   `TCPReceiver::send`, ackno is attention to closed status for stream
