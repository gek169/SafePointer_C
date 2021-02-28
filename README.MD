# Thread-safe, fast, platform-independent memory management for C

Public domain CC0, written by Gek, DMHSW

Tired of read-past-by-one errors?

Bored debugging mysterious memory leaks in spaghetti code?

Hate debugging use-after-free issues?

Hate segmentation faults?

Enjoy allocating memory without having to worry about freeing it later?

THIS IS THE LIBRARY FOR YOU!

### How can this be possible?

Simply put: This library forces you to do what you've never wanted to but always should have, and then some:

* Never store a pointer to memory without knowing its size.

* Always keep track of a pointer's lifetime (In this case, literally, it tracks the number of garbage collection invocations it is away from being reaped...)

* Guard memory accesses

If you don't go out of your way to avoid taking advantage of this library, you should never get memory errors again.



### What's the cost?

This implementation is not cost-free. The technology does exist on most platforms to make an (almost) cost-free version,
but it wouldn't be cross-platform anymore.

If you're only targetting a small handful of platforms, you should consider looking at safemem.h and figuring out how you could
implement the algorithm in a more efficient way for your platform(s).

safemem stores three arrays.

* The pointer array. Your actual malloc'd pointers are stored here.

* The "hash" array. Unique Identifiers for every malloc stored as two size_t's. It's basically just a count of all the mallocs that have ever happened. 
Two size_ts, even on a 32 bit machine, will last you basically forever (Hundreds of years of constantly running malloc() a million times per second...)

* The "Scheduled Deaths" array. The garbage collection iteration that pointer will be freed on (Within one size_t away)


It is entirely optional to use the garbage collecting feature of this library, but it's one of its strongest features, so I would recommend it.

The additional runtime cost of using safemem over raw C pointers is as such

* When allocating, safemem searches linearly for the first free available spot (From the last point where a free spot was found- for speed.) This has a non-zero cost.

* Allocating may require expanding the aforementioned three arrays by C_SAFEMEM_BLOCK_SIZE pointers. The larger the block size, the less often this happens.

* When dereferencing the safepointer to get the "real" pointer, safemem has to look up the index and compare the hashes.

* The above mentioned "lookup-and-compare" cost is also present when free-ing.

* All memory allocations are callocs, and so if there is an additional cost of calloc on your platform, then it is present.

* The arrays might get quite large if you have a lot of fragmented malloc'd memory.

### Why don't I just write better code and avoid the runtime cost of this library?

You say "I'll just write better code" but when I go through any standard linux system and try using different libraries or programs
under valgrind, I see loads of memory leaks.

The debian bug report area is CONSTANTLY seeing new "use after free" or "read-past-by-one" errors which allow arbitrary code execution.

The linux kernel and its many kernel modules regularly see new ACE bugs.

The largest impact at runtime on performance that this library will cause for you is probably the added cost of dereferencing pointers.

The data structure with probably the highest associated cost of using this library would be the linked list, then, since it requires
dereferencing pointers repeatedly to parse.

Standard vectors, arrays, framebuffers, large blocks of contiguous memory are the most efficient when working with this library,
you simply determine the largest access in your loop that will happen and guard it with a SAFEPTR_GUARD_IF

### How will I have to change my programming conventions?

First, you can't store any real pointers. Don't do it, or this whole library is pointless.

```
You get your real pointer with safepointer_deref, (with the resource lock if you're using threads)

You use your real pointer (With the SAME resource lock, if the other threads can be garbage collecting at the same time, otherwise unimportant)

You FORGET your real pointer (Let it go out of scope, set it to null)
```

Second, you guard all array accesses. You can do this before processing loops to test the maximum iteration of the loop.

This means you do have to successfully identify the largest access- Which is the biggest flaw with this library.

If you're in a situation where speed is vastly less important than not crashing (Database software, medical equipment...)
I would recommend guarding *every* access and writing an error handler.



```
a special macro SAFEPTR_GUARD_IF(type, safeptr, i) has been defined specifically to help you guard your memory accesses.
```

Third, you ALWAYS specify the *type* you're allocating when calling malloc.

```
Always use SAFEPTR_MALLOC(type, n, lifetime)
```


Fourth, you have to tell the library how long you want to use the memory (If you use the garbage collector)

```
The garbage collector is entirely optional.
```

### Is this GCC/Clang only (using attribute cleanup or the like)?

No. It should work in any C99 compatible C compiler.

Note that the included example/test program uses pthreads. Just replace the lock/unlock macros with your platform's mutex implementation
if you want to test multithreading.



### Does this library require any sort of special initialization?
```
The only thing you'd need to initialize is a mutex at program start if you're writing a multithreaded application.

Otherwise, it works out-of-the-box.
```

### How do I prevent my memory from being deleted while still using the garbage collector

```
using safepointer_keepalive(safeptr, lifetime) which updates the pointer's lifetime.
```

### just write rust/go/c++(unique_ptr) etcetera

You can do that, but there are reasons why you'd want to do it in C.

### This library is against the whole philosophy of C!

no u

### Can I use this on embedded platforms?

Your C library needs calloc, size_t, and SIZE_MAX.

You have to have stddef, stdint, and stdlib

It's recommended you go in and disable the scheduled deaths array if you're not going to use it, which you probably won't want to.

### This doesn't actually *prevent* memory problems

You're right. if you go out of the way to avoid what this library tries to force you to do, you *can* still make mistakes.

The main benefits of this library are as such

1. Mistakes are more difficult to make

2. When you identify that there is a mistake, debugging it is a lot easier

3. Code reviewers can more easily identify mistakes (In fact, you could probably write a static analyzer...).

4. Good programming conventions for source code security and maintainability are a lot easier to write AND debug.



### Here's a list of honest mistakes I foresee happening, and how code reviewers/static analyzers will spot them instantly:

* If you forget to guard pointer_deref then an expansion of safepointer's arrays might happen in another thread causing an invalid state of the library. 

Code reviewer sees lack of RESOURCE_LOCK() around safepointer_deref

* If you forget to guard the usage of memory and invoke the garbage collector on another thread, then your memory might get free'd 

Code reviewer sees lack of RESOURCE_LOCK() around the entire lifetime of a raw pointer

* You might incorrectly determine the maximum memory access of a for loop you're guarding with SAFEPTR_GUARD_IF 

Code reviewer sees lack of or incorrect placement of SAFEPTR_GUARD_IF

* You stupidly store a raw pointer without thinking about it 

Code reviewer sees raw pointer type in struct/globals

* You fail to guard memory access altogether 

Code reviewer sees pointer_deref or raw pointer deref without SAFEPTR_GUARD_IF

* You don't handle getting a NULL from safepointer_deref 

Code reviewer sees SAFEPTR_GUARD_IF without else error handling

* You use different types when guarding and when allocating 

Code reviewer sees mismatching types in SAFEPTR_GUARD_IF and SAFEPTR_MALLOC

* Programmer dereferences non-initialized safepointer

Most dereference-before-initialization errors will result in the error handler triggering in SAFEPTR_GUARD_IF's.

Code reviewer will see error handler triggering, or...
Code reviewer will see lack of error handler.

* Programmer artificially creates safepointer struct

Code reviewer will see safepointer struct having its members assigned or the struct itself being overwritten.



You can see now how this library makes debugging issues and independently verifying code a lot easier.

This library is *specifically* targeted at large organizations which want to reduce the number of man-hours needed to verify the integrity
of a programmer's code.

99% of errors can be caught just by hitting Ctrl+F in the codebase instead of meticulous verification of algorithms.

Even horrible spaghetti code should be easily verifiable.

### There are better ways you could have done this.

I'd like to know, actually. Submit an issue! Thanks!

I spent quite a while trying to figure out the most efficient, effective way I could write a memory safety system for C.

* I could put some data at the beginning of all memory allocations, like the size.

```
This doesn't prevent you from using after free or using an invalid pointer.
```

* I could hook into SIGSEG somehow and return valid pointers

```
Not cross platform, and probably quite slow.
```

* unix/linux mmap magic

```
NOT CROSS PLATFORM
```

* msync in linux

```
What did I just tell you? Also, that's gonna be pretty slow. A lot slower than a lookup table.
```

* Define a pre-made container which holds the size and automatically checks for dereference errors

```
Doesn't stop you from using after free
```
