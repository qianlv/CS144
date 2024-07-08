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
