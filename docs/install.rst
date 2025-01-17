|Logo|

======================
Verilator Installation
======================

.. contents::
   :depth: 3

Introduction
============

This discusses how to install Verilator. For more general information
please see `verilator.org <https://verilator.org>`__.


Quick-start
===========


Install From a Package Manager
------------------------------

Using a distribution's package manager is the easiest way to get
started. (Note packages are unlikely to have the most recent version, so
Git, below, maybe a better alternative.) To install as a package:

::

   apt-get install verilator

If this works, skip down to `Running Verilator <#_running_verilator>`__.


Docker
------

Verilator is available in pre-built Docker containers. See
https://github.com/verilator/verilator/blob/master/ci/docker/run/README.rst


Git
---

Installing Verilator with Git provides the most flexibility. For
additional options and details see the additional sections below. In
brief:

::

   # Prerequisites:
   #sudo apt-get install git make autoconf g++ flex bison
   #sudo apt-get install libfl2  # Ubuntu only (ignore if gives error)
   #sudo apt-get install libfl-dev  # Ubuntu only (ignore if gives error)

   git clone https://github.com/verilator/verilator   # Only first time
   ## Note the URL above is not a page you can see with a browser, it's for git only

   # Every time you need to build:
   unsetenv VERILATOR_ROOT  # For csh; ignore error if on bash
   unset VERILATOR_ROOT  # For bash
   cd verilator
   git pull        # Make sure git repository is up-to-date
   git tag         # See what versions exist
   #git checkout master      # Use development branch (e.g. recent bug fixes)
   #git checkout stable      # Use most recent stable release
   #git checkout v{version}  # Switch to specified release version

   autoconf        # Create ./configure script
   ./configure
   make
   sudo make install
   # Now see "man verilator" or online verilator.pdf's for the example tutorials

If this works, skip down to `Running Verilator <#_running_verilator>`__.


Detailed Build Instructions
===========================

This section describes details of the build process, and assumes you are
building from Git or a tarball. For using a pre-built binary for your
Linux distribution, see instead `Install From a Package
Manager <#_install_from_a_package_manager>`__.


OS Requirements
---------------

Verilator is developed and has primary testing on Ubuntu, with additional
testing on FreeBSD and Apple OS-X. Versions have also built on Redhat
Linux, HPUX and Solaris. It should run with minor porting on any
GNU/Linux-ish platform. Verilator also works on Windows under Cygwin, and
Windows under MinGW (gcc -mno-cygwin). Verilated output (not Verilator
itself) compiles under all the options above, plus MSVC++.


Install Prerequisites
---------------------

To build or run Verilator you need these standard packages:

::

   sudo apt-get install perl python3 make
   sudo apt-get install g++  # Alternatively, clang
   sudo apt-get install libgz  # Non-Ubuntu (ignore if gives error)
   sudo apt-get install libfl2 libfl-dev zlibc zlib1g zlib1g-dev  # Ubuntu only (ignore if gives error)

To build or run the following are optional but should be installed for good
performance:

::

   sudo apt-get install ccache  # If present at build, needed for run
   sudo apt-get install libgoogle-perftools-dev numactl perl-doc

To build Verilator you will need to install these packages; these do not
need to be present to run Verilator:

::

   sudo apt-get install git autoconf flex bison

Those developing Verilator itself may also want these (see internals.rst):

::

   sudo apt-get install gdb asciidoctor graphviz cmake clang clang-format gprof lcov
   cpan install Pod::Perldoc
   cpan install Parallel::Forker


Install SystemC
~~~~~~~~~~~~~~~

If you will be using SystemC (vs straight C++ output), download `SystemC
<https://www.accellera.org/downloads/standards/systemc>`__.  Follow their
installation instructions. You will need to set ``SYSTEMC_INCLUDE`` to
point to the include directory with ``systemc.h`` in it, and
``SYSTEMC_LIBDIR`` to points to the directory with ``libsystemc.a`` in
it. (Older installations may set ``SYSTEMC`` and ``SYSTEMC_ARCH`` instead.)


Install GTKWave
~~~~~~~~~~~~~~~

To make use of Verilator FST tracing you will want `GTKwave
<http://gtkwave.sourceforge.net/>`__ installed, however this is not
required at Verilator build time.


Obtain Sources
--------------

You may use Git or a tarball for the sources. Git is the supported
option. (If using a historical build that uses a tarball, tarballs are
obtained from `Verilator Downloads
<https://www.veripool.org/projects/verilator/wiki/Download>`__; we presume
you know how to use it, and is not described here.)

Get the sources from the repository: (You need do this only once, ever.)

::

   git clone https://github.com/verilator/verilator   # Only first time
   ## Note the URL above is not a page you can see with a browser, it's for git only

Enter the checkout and determine what version/branch to use:

::

   cd verilator
   git pull        # Make sure we're up-to-date
   git tag         # See what versions exist
   #git checkout master      # Use development branch (e.g. recent bug fix)
   #git checkout stable      # Use most recent release
   #git checkout v{version}  # Switch to specified release version


Auto Configure
--------------

Create the configuration script:

::

   autoconf        # Create ./configure script


Eventual Installation Options
-----------------------------

Before configuring the build, you have to decide how you're going to
eventually install the kit. Verilator will be compiling the current value
of ``VERILATOR_ROOT``, ``SYSTEMC_INCLUDE``, and ``SYSTEMC_LIBDIR`` as
defaults into the executable, so they must be correct before configuring.

These are the options:


1. Run-in-Place from VERILATOR_ROOT
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Our personal favorite is to always run Verilator in-place from its Git
directory. This allows the easiest experimentation and upgrading, and
allows many versions of Verilator to co-exist on a system.

::

   export VERILATOR_ROOT=`pwd`   # if your shell is bash
   setenv VERILATOR_ROOT `pwd`   # if your shell is csh
   ./configure
   # Running will use files from $VERILATOR_ROOT, so no install needed

Note after installing (below steps), a calling program or shell must set
the environment variable ``VERILATOR_ROOT`` to point to this Git directory,
then execute ``$VERILATOR_ROOT/bin/verilator``, which will find the path to
all needed files.


2. Install into a CAD Disk
~~~~~~~~~~~~~~~~~~~~~~~~~~

You may eventually be installing onto a project/company-wide "CAD" tools
disk that may support multiple versions of every tool. Target the build to
a destination directory name that includes the Verilator version name:

::

   unset VERILATOR_ROOT      # if your shell is bash
   unsetenv VERILATOR_ROOT   # if your shell is csh
   # For the tarball, use the version number instead of git describe
   ./configure --prefix /CAD_DISK/verilator/`git describe | sed "s/verilator_//"`

Note after installing (below steps), if you use `modulecmd
<http://modules.sourceforge.net/>`__, you'll want a module file like the
following:

**modulecmd's verilator/version file.**

::

   set install_root /CAD_DISK/verilator/{version-number-used-above}
   unsetenv VERILATOR_ROOT
   prepend-path PATH $install_root/bin
   prepend-path MANPATH $install_root/man
   prepend-path PKG_CONFIG_PATH $install_root/share/pkgconfig


3. Install into a Specific Path
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You may eventually install Verilator into a specific installation prefix,
as most GNU tools support:

::

   unset VERILATOR_ROOT      # if your shell is bash
   unsetenv VERILATOR_ROOT   # if your shell is csh
   ./configure --prefix /opt/verilator-VERSION

Then after installing (below steps) you will need to add
``/opt/verilator-VERSION/bin`` to ``$PATH``.


4. Install System Globally
~~~~~~~~~~~~~~~~~~~~~~~~~~

The final option is to eventually install Verilator globally, using the
normal system paths:

::

   unset VERILATOR_ROOT      # if your shell is bash
   unsetenv VERILATOR_ROOT   # if your shell is csh
   ./configure

Then after installing (below) the binary directories should already be
in your ``$PATH``.


Configure
---------

The command to configure the package was described in the previous step.
Developers should configure to have more complete developer tests.
Additional packages may be required for these tests.

::

   export VERILATOR_AUTHOR_SITE=1    # Put in your .bashrc
   ./configure --enable-longtests  ...above options...


Compile
-------

Compile Verilator:

::

   make -j


Test
----

Check the compilation by running self-tests:

::

   make test


Install
-------

If you used any but the `1. Run-in-Place from VERILATOR_ROOT
<#_1_run_in_place_from_verilator_root>`__ scheme, install to the
OS-standard place:

::

   make install


Running Verilator
=================

To run Verilator, see the example sections in the `Verilator manual (HTML)
<https://verilator.org/verilator_doc.html>`__, or `Verilator manual (PDF)
<https://verilator.org/verilator_doc.pdf>`__.

Also see the ``examples/`` directory that is part of the kit, and is
installed (in a OS-specific place, often in e.g.
``/usr/local/share/verilator/examples``).

::

   cd examples/make_hello_c
   make

Note if you did a ``make install`` above you should not have
``VERILATOR_ROOT`` set in your environment; it is built into the
executable.


Announcements
=============

To get notified of new releases, go to `Verilator announcement repository
<https://github.com/verilator/verilator-announce>`__ and follow the
instructions there.


Directory Structure
===================

Some relevant files and directories in this package are as follows:

::

   Changes                     => Version history
   README.rst                 => This document
   bin/verilator               => Compiler wrapper invoked to Verilate code
   docs/                       => Additional documentation
   examples/make_hello_c       => Example GNU-make simple Verilog->C++ conversion
   examples/make_hello_sc      => Example GNU-make simple Verilog->SystemC conversion
   examples/make_tracing_c     => Example GNU-make Verilog->C++ with tracing
   examples/make_tracing_sc    => Example GNU-make Verilog->SystemC with tracing
   examples/make_protect_lib   => Example using --protect-lib
   examples/cmake_hello_c      => Example building make_hello_c with CMake
   examples/cmake_hello_sc     => Example building make_hello_sc with CMake
   examples/cmake_tracing_c    => Example building make_tracing_c with CMake
   examples/cmake_tracing_sc   => Example building make_tracing_sc with CMake
   examples/cmake_protect_lib  => Example building make_protect_lib with CMake
   include/                    => Files that should be in your -I compiler path
   include/verilated*.cpp      => Global routines to link into your simulator
   include/verilated*.h        => Global headers
   include/verilated.mk        => Common Makefile
   src/                        => Translator source code
   test_regress                => Internal tests

For files created after a design is Verilated, see the `Verilator manual
(HTML) <https://verilator.org/verilator_doc.html>`__, or `Verilator
manual (PDF) <https://verilator.org/verilator_doc.pdf>`__.


License
=======

Copyright 2008-2021 by Wilson Snyder. Verilator is free software; you can
redistribute it and/or modify it under the terms of either the GNU Lesser
General Public License Version 3 or the Perl Artistic License Version 2.0.

.. |Logo| image:: https://www.veripool.org/img/verilator_256_200_min.png
