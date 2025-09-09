*******
CLogger
*******
A small, portable logging library for C that gives you structured, level-based 
logging without the usual headaches:

* Consistent **log levels** (DEBUG, INFO, WARNING, ERROR, CRITICAL)
* **Passable object** (``Logger*``) so you can inject logging into any function
* **Multiple sinks**: file, terminal, or both
* **Context-rich output**: timestamp, level, file:line:function
* **Thread-safe** by default (C11 threads with fallbacks to pthreads/Win32)
* **No heap required** (stack/static), with optional heap-based helpers if desired
* Sensible **stdio buffering** knobs for performance
* **MISRA C–friendly API**: macros optional, direct functions available

When to Use This Library
########################
All functionality is available from a single header/source pair: ``logger.h`` and ``logger.c``.

This library is particularly useful when you need:

* A simple, dependency-free logger for C11+ projects
* Log filtering by severity (e.g., suppress DEBUG in production)
* Colorized terminal logs (TTY only) and plain log files
* Injected logging (pass a ``Logger*`` into subsystems instead of using globals)
* Portable thread-safety across platforms (C11/pthreads/Win32)

The design avoids dynamic allocation by default (great for embedded or early-boot code), 
but you can add heap-based creation helpers if your app architecture prefers that style.

MISRA and Non-Macro Usage
#########################
By default, logging is done with convenience macros:

.. code-block:: c

   LOG_INFO(&lg, "Value is %d", x);

For MISRA C environments (where macros are discouraged), you can disable them 
at compile time by defining:

.. code-block:: c

   #define LOGGER_DISABLE_MACROS

Then use the explicit function:

.. code-block:: c

   logger_write(&lg, LOG_INFO, __FILE__, __LINE__, __func__, "Value is %d", x);

This provides the same functionality without macros, keeping the code MISRA-compliant.

Implementation Details
######################
Core Types
----------
**LogLevel**
   Enumerated severity: ``LOG_DEBUG``, ``LOG_INFO``, ``LOG_WARNING``, ``LOG_ERROR``, ``LOG_CRITICAL``.

**Logger**
   A small struct holding configuration and sinks:

* ``FILE* file`` (optional), ``FILE* stream`` (optional, e.g., ``stderr``)
* ``LogLevel level`` (minimum level to emit)
* ``const char* name`` (optional tag, not owned by default)
* ``timestamps`` and ``colors`` toggles
* Portable mutex (C11/pthreads/Win32) guarding concurrent emission

Threading & Portability
-----------------------
The header conditionally includes one of:

* C11 threads: ``<threads.h>`` (``mtx_t``)
* POSIX: ``pthread_mutex_t``
* Windows: ``CRITICAL_SECTION``

Buffering & Performance
-----------------------
The implementation uses ``setvbuf`` to reduce syscall overhead:

* Files: full buffering (e.g., 1 MiB) to batch writes
* Terminals: line buffering for prompt output (TTY only)

Error Handling
--------------
* **Initialization functions** return ``bool`` and set ``errno`` on failure.
* **Setters** set ``errno`` only on bad input (e.g., ``Logger* == NULL``).
* **Logging calls** set ``errno`` only on invalid parameters.

API Overview
------------
Initialization:

* ``bool logger_init_stream(Logger* lg, FILE* stream, LogLevel level);``
* ``bool logger_init_file(Logger* lg, const char* path, LogLevel level);``
* ``bool logger_init_dual(Logger* lg, const char* path, FILE* stream, LogLevel level);``
* ``void logger_close(Logger* lg);``

Configuration:

* ``void logger_set_level(Logger* lg, LogLevel level);``
* ``void logger_set_name(Logger* lg, const char* name);``  (NULL clears)
* ``void logger_enable_timestamps(Logger* lg, bool on);``
* ``void logger_enable_colors(Logger* lg, bool on);``
* ``void logger_enable_locking(Logger* lg, bool on);``

Logging:

* ``LOG_DEBUG/INFO/WARNING/ERROR/CRITICAL(lg, "fmt %d", x);`` (macros)
* ``logger_write(lg, level, __FILE__, __LINE__, __func__, "fmt %d", x);`` (MISRA-friendly)

