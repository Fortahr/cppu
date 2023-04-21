# CPPUtilities
Header-only library that offers cross platform tools and more useful functions to develop programs and games.
Crossplatform C++ library (c++17)


## Function wrapper, alternative to std::function
* Faster and smaller than `std::function`, see [BENCHMARK.md](BENCHMARK.md)
* Size == `2 * sizeof(void*)`
* Embedding of `sizeof(T) <= sizeof(void*)` objects/lambdas, i.e.: no heap allocation
* Supports lambda, static, and member functions
  ```cpp
  cppu::function<void(int)>(&StaticFunction);
  cppu::function<void(int)>(&T::MemberFunction, &object); // pointer to object
  cppu::function<void(int)>(&T::MemberFunction, object); // copy object, may embed
  cppu::function<void(int)>([](int x) { ... });
  cppu::function<void(int)>([this](int x) { ... }); // embeds the lambda
  ```
* No `std::bind` with placeholder types, just the member functions and a pointer
* Define `CPPU_FUNCTION_ENABLE_JUMP_RESOLVE` to resolve jmp tables, like those with incremental linking
* Uses pointer tagging (top 2 bits, masked out before invocation)

## Garbage collected containers with smart pointers
  - Strong and Weak pointer types for any of the containers,
  - Containers are dedicated to a single object type,
  - Destruction of objects can be done manually, automatically in a background thread, or directly when the last references goes away (this can be set per container),
  - Cleaned up positions for containers like arrays will be reused on the next construction of an object.
  - Arrays still store objects like normal arrays (thread-safe, lock-free),
  - Arrays can have an encapsulating version that will automatically add more arrays (like deques/buckets),
  - Arrays are thread-safe, no locks, uses compare and swap (CAS) instead,
  - Maps (using std unordered or ordered map for now),
  - these collections do not move objects around so it does not break/invalidate any references or pointers.

## Serializer
  - Serializes to binary data, usable for saving data and network packets,
  - VTable to enable backward and forward compatibility,
  - Holds version info so the developer can do different logic per different version of packets,
  - Allows nested archives, ideal for a hierarchy of objects (including member objects),
  - Does not offer type checking to keep performance (only correct use of the vtable can help with type correctness),
  - Class, inheritance, and pointer support (no overhead for other type (de-)serializations),
  - Option to directly (fast) serialize and deserialize (no vtable usage, only advised for internal usage),
  - Goal is to keep it as fast and preferably as small as simply doing a mem dump per object in a stream.

## Others
  - CPU & Memory (physical + Virtual) monitoring,
  - Offers TLS sockets (asio client and server sockets, using the bearssl library for the TLS handshake and encryption),
  - Has a stack tracer (Windows only right now, though unstable at the moment),
  - Extra functions like showing a console screen and checking if the program is already running.

CPPUtilities has been released under the MIT license, I do appreciate acknowledgement from whoever uses it.
