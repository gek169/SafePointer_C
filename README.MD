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

### What's the cost?

This implementation is not cost-free. The technology does exist on most platforms to make a cost-free version,
but it wouldn't be cross-platform anymore.

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


### How will I have to change my programming conventions?

First, you can't store any real pointers. Don't do it, or this whole library is pointless.

```
You get your real pointer with safepointer_deref, (with the resource lock if you're using threads)

You use your real pointer (With the SAME resource lock, if the other threads can be garbage collecting at the same time, otherwise unimportant)

You FORGET your real pointer (Let it go out of scope, set it to null)
```

Second, you guard all array accesses. You can do this before for-loops to test the maximum iteration of the loop.

```
a special macro SAFEPTR_GUARD_IF(type, safeptr, i) has been defined specifically to help you guard your memory accesses.
```

Third, you have to tell the library how long you want to use the memory (If you use the garbage collector)

```
The garbage collector is entirely optional.
```

### Does this library require any sort of special initialization?
```
No. 
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

### There are better ways you could have done this.

I'd like to know, actually. Submit an issue! Thanks!

I spent quite a while trying to figure out the most efficient way I could write a memory safety system for C.

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
