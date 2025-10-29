EADME.rst

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
at compile time by defining the compile time float ``NO_FUNCTION_MACROS``.

.. code-block:: c

    gcc -DNO_FUNCTION_MACROS -o myprogram main.c logger.c

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

Usage Example
#############
.. code-block:: c

   #include "logger.h"
   #include <stdio.h>
   #include <stdlib.h>

   int main(void) {
       Logger log;
       if (!logger_init_dual(&log, "app.log", stderr, LOG_DEBUG)) {
           perror("logger_init_dual");
           return EXIT_FAILURE;
       }
       logger_set_name(&log, "demo");

       LOG_INFO(&log, "Application start");
       logger_write(&log, LOG_ERROR, __FILE__, __LINE__, __func__, "Error with MISRA-safe call");

       logger_close(&log);
       return EXIT_SUCCESS;
   }

Building
########
Compile directly (POSIX)
------------------------
.. code-block:: bash

   cc -std=c11 -Wall -Wextra -O2 main.c logger.c -o demo -pthread
   ./demo

Compile directly (Windows MSVC)
-------------------------------
.. code-block:: batch

   cl /std:c11 /W4 /O2 main.c logger.c
   demo.exe

CMake Builds
------------
The project provides a ``CMakeLists.txt`` to build static or shared libraries 
and unit tests with ``cmocka``:

.. code-block:: bash

   cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug -DLOGGER_BUILD_TESTS=ON
   cmake --build build/debug -j
   ctest --test-dir build/debug --output-on-failure

Helper Scripts
##############
Convenience scripts are included in ``scripts``:

**zsh**
--------------
* ``scripts/zsh/debug.zsh`` – Debug build with tests
* ``scripts/zsh/static.zsh`` – Build static library
* ``scripts/zsh/install.zsh`` – Install to system prefix

**bash**
--------------
* ``scripts/bash/debug.sh`` – Debug build with tests
* ``scripts/bash/static.sh`` – Build static library
* ``scripts/bash/install.sh`` – Install to system prefix

**Windows (.bat)**
------------------
* ``scripts/Windows/debug.bat`` – Debug build with tests
* ``scripts/Windows/static.bat`` – Build static library
* ``scripts/Windows/install.bat`` – Install to a given prefix

Run them from the repo root. Edit hardcoded paths in the script if needed.

System Installation
-------------------
.. code-block:: bash

   sudo cmake --install build/static

or on Windows:

.. code-block:: batch

   cmake --install build\static --config Release

Requirements
############

* C11 compiler (GCC, Clang, or MSVC)
* CMake 3.26+
* Optional: cmocka (for unit testing)
* Optional: valgrind (Linux, leak checking)

License
#######
MIT License.

Documentation
#############
Further documentation (API reference and examples) is planned for a Read the Docs site.  
Until then, see the in-code Doxygen comments in ``logger.h`` and ``logger.c``.
