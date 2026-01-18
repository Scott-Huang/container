# VexDB template library
Header file container lib without RAII. Under development.

But all headers not ending with .hpp should be usable. Set the include path to vtl to use it.

TBA

## Memory Containers
All containers need to be manually released as PG which uses setjmp is incompatible with RAII,
call `.destroy()` to release them. Read <vtl/allocator> for memory allocation setting, which is
basically a wrapper of `MemoryContext`.
