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


