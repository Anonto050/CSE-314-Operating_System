# Implementing Threading and Synchronization in xv6

This README provides comprehensive instructions on implementing threading and synchronization primitives in the xv6 operating system. It covers creating new system calls for threads, handling thread exits, implementing synchronization mechanisms such as spinlocks, mutexes, condition variables, and semaphores, and solving synchronization problems like the producer-consumer problem.

## Threading Implementation

### Creating a New Thread

To add threading support, we need to implement three new system calls: `thread_create()`, `thread_join()`, and `thread_exit()`. These functions will enable creating, managing, and terminating threads within xv6.

#### Steps to Implement `thread_create()`

1. **Base Functionality**: Start by duplicating the `fork()` function's contents. This provides a foundation since threads are similar to processes but share the same address space.
   
2. **Replace `uvmcopy()` with `uvmmirror()`**:
   - **uvmmirror Function**: Write a function `uvmmirror()` that mimics `uvmcopy()` but omits the `mem` variable.
   - **Modification**: Replace the reference to `mem` in the call to `mappages()` with `pa`. This change ensures that the threads share the same physical memory pages.

3. **Set Entry Point**:
   - Set `np->trapframe->epc` to the function pointer `fcn` provided by the user. This sets the entry point for the new thread, ensuring that the scheduler starts execution at the specified function.

4. **Initialize Stack Pointer**:
   - Set `np->trapframe->sp` to `stack + 4096`. This initialization is crucial as the stack grows downwards; hence, it should point to the end of the allocated page (4096 bytes).
   - Adjust the stack pointer with `sp - (sp % 16)` to maintain 16-byte alignment, as seen in `exec.c`.

5. **Pass Arguments**:
   - Set `np->trapframe->a0` to `arg`. In RISC-V architecture, function arguments are stored in registers `a0`, `a1`, etc.

6. **Thread Identification**:
   - Set `np->is_thread = 1` to distinguish threads from processes. This flag will be used in various parts of the codebase, particularly in `freeproc()`.

#### Exiting from a Thread

To handle thread termination, implement the `thread_exit()` system call, similar to `exit()`, but with considerations for shared resources.

1. **Assumptions**:
   - Assume that the parent thread will wait for its children to exit. This prevents issues with resource deallocation timing.

2. **Resource Management in `freeproc()`**:
   - **Threads vs. Processes**: If `p->is_thread == 0`, call `proc_freepagetable()` as usual.
   - **Shared Memory Management**: When clearing a thread (`p->is_thread == 1`), modify the last parameter of `uvmunmap()` to `0` to prevent deallocation of shared physical memory. This ensures that only the parent process handles memory deallocation.

#### Joining a Thread

The `thread_join()` system call, similar to `wait()`, allows a parent process to wait for a specific thread.

1. **Implementation Details**:
   - Add `pp->pid == pid` in the if-statement loop to ensure it waits for the specific thread.
   - Remove unnecessary parameters like `addr`, as threads share the same address space.

### Setting Up proc.h

To manage threads, additional fields and structures are necessary in `proc.h`.

1. **New Fields**:
   - **is_thread**: A flag indicating if the process is a thread.
   - **mem_id**: An identifier for the shared physical memory. Threads created by the same parent share the same `mem_id`.
   - **memlock**: A lock to prevent concurrent memory size changes, avoiding race conditions.

2. **Handling `mem_id`**:
   - Assign `mem_id` in a similar manner to how `pid` is allocated in `allocproc()`.
   - Ensure that child threads inherit the parent's `mem_id`.

3. **Managing `memlock`**:
   - **Option 1**: Use a shared spinlock pointer for all processes with the same `mem_id`. This setup simplifies synchronization at the cost of initial complexity.
   - **Option 2**: Allocate a separate `memlock` for each process. This method requires careful synchronization across all related processes but is simpler to set up initially.

### Modifying growproc()

When threads request memory expansion, the `growproc()` function needs to handle it correctly.

1. **Locking**:
   - Acquire `memlock(s)` at the beginning of `growproc()` and release it at the end. This ensures that no two threads can modify the memory size concurrently.

2. **Memory Adjustment**:
   - For positive `n` (memory growth): Extend the mapping from the old size to the new size using a modified `uvmmirror()`, called `uvmrangemirror()`.
   - For negative `n` (memory reduction): Use `uvmunmap()` to unmap the memory from the new size to the old size.

3. **Consistency Across Threads**:
   - Iterate over all processes with the same `mem_id` to apply the memory size change consistently.

## Implementing Synchronization Primitives

Synchronization primitives ensure that multiple threads can safely access shared resources.

### Spinlocks and Mutexes

1. **Spinlocks**:
   - Use the kernel's spinlock implementation as a base.
   - Change the `locked` variable type to `uint8` for user-space compatibility.
   - Remove kernel-specific functions like `push_off()` and `pop_off()`.

2. **Mutexes**:
   - Use `sleep(1)` instead of a custom `yield()` to prevent busy waiting. This avoids the issue where the same process might be scheduled immediately after yielding.

### Condition Variables

Condition variables are used for waiting on certain conditions to be met.

1. **Queue Management**:
   - Use a queue to manage waiting threads, similar to the one in `producer_consumer.c`.

2. **Atomicity**:
   - Ensure that releasing a mutex and going to sleep is atomic to avoid race conditions.

3. **Custom Sleep Function**:
   - Implement `new_sleep()` to handle sleeping without race conditions. It must be able to release the mutex and put the thread to sleep atomically.

4. **Modifications in copyout()**:
   - Modify `copyout()` to release the lock atomically in the `new_sleep()` implementation. This involves setting the lock in a way that is atomic and avoids race conditions.

### Semaphores

Semaphores are implemented using conditional variables to manage access to shared resources.

1. **Implementation**:
   - Refer to page 405 of the OSTEP book for a detailed description and implementation guidelines.

2. **Example**:
   - Implement a producer-consumer problem using semaphores to manage the buffer and mutexes for mutual exclusion.

