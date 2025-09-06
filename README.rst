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
* You can adjust these defaults or expose options (see below)

Error Handling
--------------
* **Initialization functions** return ``bool`` and set ``errno`` on failure (e.g., ``EINVAL`` for bad input, pass-through from ``fopen``).
* **Setters** are ``void`` and set ``errno`` only on bad input (e.g., ``Logger* == NULL``).
* **Logging calls** ignore ``errno`` on success and only set it on invalid parameters (never on level filtering).

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

* ``LOG_DEBUG/INFO/WARNING/ERROR/CRITICAL(lg, "fmt %d", x);`` — macros capturing file/line/function
* ``logger_log_impl`` / ``logger_vlog_impl`` — lower-level helpers if you need them

Usage Example
#############
.. code-block:: c

   #include "logger.h"
   #include <stdio.h>
   #include <stdlib.h>

   static void worker(Logger* log, int job_id) {
       LOG_DEBUG(log, "Starting job %d", job_id);
       if (job_id == 42) LOG_WARNING(log, "Special case");
       LOG_INFO(log, "Job %d done", job_id);
   }

   int main(void) {
       Logger log;
       if (!logger_init_dual(&log, "app.log", stderr, LOG_DEBUG)) {
           perror("logger_init_dual");
           return EXIT_FAILURE;
       }
       logger_set_name(&log, "demo");
       logger_enable_colors(&log, true);
       logger_enable_timestamps(&log, true);

       LOG_INFO(&log, "Application start");
       worker(&log, 1);
       worker(&log, 42);
       LOG_ERROR(&log, "Simulated error");
       LOG_CRITICAL(&log, "Critical condition, exiting");

       logger_close(&log);
       return EXIT_SUCCESS;
   }

Building
########
Single-Command Build (POSIX)
----------------------------
.. code-block:: bash

   cc -std=c11 -Wall -Wextra -O2 main.c logger.c -o demo -pthread
   ./demo

Windows (MSVC)
--------------
.. code-block:: batch

   cl /std:c11 /W4 /O2 main.c logger.c
   demo.exe

When to Adjust Buffering
########################
I/O can dominate runtime in chatty programs. The defaults already help, but you may want to tune:

* Increase file buffer size if disk writes are frequent:

  .. code-block:: c

     /* In logger_init_file after fopen succeeds */
     setvbuf(lg->file, NULL, _IOFBF, 1<<20);  /* 1 MiB */

* Use line buffering on terminals for prompt display while limiting syscalls:

  .. code-block:: c

     if (LOGGER_ISATTY(stream)) setvbuf(stream, NULL, _IOLBF, 0);

* Provide a ``logger_flush(lg)`` helper (optional) that calls ``fflush`` on sinks, to flush before ``fork/exec`` or after critical logs.

Design Choices & Extensions
###########################
Why object-style?
-----------------
Passing ``Logger*`` avoids globals, makes tests cleaner, and keeps lifetime explicit.

Static vs Dynamic Allocation
----------------------------
The library is designed for **stack/static** allocation by default. You can optionally add heap factories (``logger_new_*`` / ``logger_free``) if your architecture needs returnable/late-constructed loggers or opaque handles.

Optional Extensions (easy to add)
---------------------------------
* **Rotation** (size/time-based)
* **JSON/structured logs**
* **Buffered mode** (ring buffer) with ``logger_flush`` / ``dump_to_file``
* **Asynchronous writer thread** for ultra-low-latency hot paths
* **Allocator hooks** for custom arenas

Contributing
############
Pull requests are welcome. For major changes, open an issue first to discuss what you’d like to change. Please include/update tests and related docstrings. Keep portable behavior across C11/pthreads/Win32.

License
#######
MIT License.

Requirements
############
Developed and tested on macOS and Linux. Known-good toolchains:

* GCC 13+/Clang 16+ (POSIX) and MSVC (Windows)
* C standard: C11 or later (works with C17)
* Optional: ``cmocka`` for unit tests, ``valgrind`` on Linux for leak checks
* Optional: CMake 3.26+ if you prefer out-of-tree builds

Installation and Build Guide
############################
Getting the Code
----------------
.. code-block:: bash

   git clone https://github.com/<your-username>/clogger.git
   cd clogger

Debug Build (example with CMake)
--------------------------------
.. code-block:: bash

   cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug
   cmake --build build/debug -j

Run tests (if present):

.. code-block:: bash

   cd build/debug
   ctest --output-on-failure

Static Library Build
--------------------
.. code-block:: bash

   cmake -S . -B build/static -DLOGGER_BUILD_STATIC=ON -DCMAKE_BUILD_TYPE=Release
   cmake --build build/static -j

System Installation (optional)
------------------------------
.. code-block:: bash

   sudo cmake --install build/static

Usage in Projects
-----------------
1) **As system/installed library**: include headers in your C files and link the library if you built one.

.. code-block:: c

   #include <logger.h>

2) **As sources**: copy ``logger.h`` and ``logger.c`` into your project and compile them with your code.

Troubleshooting
---------------
* **Undefined references to pthreads**: ensure ``-pthread`` (GCC/Clang) is present on POSIX builds.
* **Colors not showing on Windows**: legacy consoles may need ANSI support enabled; or leave colors off for files.
* **No timestamps/colors**: verify that ``logger_enable_timestamps``/``logger_enable_colors`` are set to ``true``.
* **Nothing prints at DEBUG**: set the logger level to ``LOG_DEBUG``; levels below the threshold are suppressed.

Contribute to Documentation
---------------------------
If you maintain Sphinx/Doxygen docs:

1. Create a Python virtual environment and install requirements:

   .. code-block:: bash

      python -m venv .venv
      . .venv/bin/activate
      pip install -r docs/requirements.txt

2. Generate documentation (examples):

   .. code-block:: bash

      doxygen docs/doxygen/Doxyfile
      sphinx-build -b html docs/source docs/_build/html

Documentation
=============
Further documentation (API reference and examples) is planned for a Read the Docs site.  
Until then, the in-code Doxygen comments in ``logger.h``/``logger.c`` are the source of truth.

