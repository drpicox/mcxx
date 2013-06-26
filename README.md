Mercurium C/C++ Compiler
========================

This is the Mercurium C/C++ compiler source package. 
This software is licensed under GPL v3 and you
should have a file named `COPYING' containing the license itself.

## Target environment

The compiler is meant for GNU/Linux. It has not been tested in other *NIXes and
it is much more than possible that it will arise portability issues in these
environments.

## Installation from source tarball

You'll need only a fairly recent version of gcc and g++ (4.1 or newer). Just do the typical

    $ ./configure
    $ make
    $ make install

It is recommended that you use --prefix on the configure in order to set the base directory
of the whole installation. In this latter case you'll need to adjust the PATH and the LD_LIBRARY_PATH
to the prefix/bin and prefix/lib respectively. 

Provided you use a Bourne alike shell the following would suffice.

    $ ./configure --prefix=/my/path
    $ make
    $ make install
    $ export PATH=$PATH:/my/path/bin
    $ export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/my/path/bin

## Installation from source repository

You will need

  flex 2.5.x
  gperf 3.0.x
  automake-1.9 (or newer)
  autoconf-2.59 (or newer)
  libtool-1.5.22 (or newer)
  gcc and g++ (4.1 or newer)

Provided you use a Bourne alike shell you must bootstrap

    $ aclocal-1.9
    $ autoconf  # Ensure that this invokes the proper version of autoconf
    $ autoheader # Ensure that this invokes the version of autoheader of the autoconf you used
    $ libtoolize --force
    $ automake-1.9 -a -c

## Configuration of the compiler

The compiler is driven by the file located at prefix/share/mcxx/config.mcxx. The file has
the following structure

    [section_name]
    option_name=value

"section_name" denotes the name of the executable. By default 'mcc' and 'mcxx' are
installed configured for C and C++, respectively. If you want to create a
special version of the compiler just soft-link or copy (the former being
better if you want to rebuild the compiler). For instance, if you want
a special compiler for MPI and C, you could create 'mpicc' in the prefix/bin
and then create a section named 'mpicc'

    [mpicc]
    options

## Phases of the compiler

The idea, but it is not still made possible, is that the compiler will provide a sort of SDK
to develop further compiler phases that are dynamically loaded. This phases are located under
src/tl where you will find several example (namely OpenMP and instrumentation with Mintaka).
Obviously, more documentation about this is needed :)


Building Mercurium from GIT
===========================

First check that all requirements are fulfilled, note that there are extra requirements when building from git. You can get the latest git snapshot by running the following command:

    $ git clone http://pm.bsc.es/git/mcxx.git

This will create a git repository in your local machine. The  web interface can also be used to get the source code of Mercurium. Once the code has been fetched from the git, the first step is to run autoreconf to generate the configure script and the rest of the initial files. To do this cd into the new Nanos++ directory and run:

    $ autoreconf -vfi

This process is somewhat fragile: some warnings will appear in several Makefile.am (due to GNU Make extensions used in Mercurium makefiles) and some m4 warnings might or might not appear depending on your precise environments (although this is now rare in Linux it might happen in some versions of Solaris). However, no errors should happen.

It may happen that autoreconf does complain about some Libtool macros not recognized. It usually happens if the Libtool used is not 2.2.6 or it is not installed system-wide. In the latter case, adjust your PATH variable to use a 2.2.6 (or better) Libtool. In either case, run the following command (make sure it comes from a 2.2.6 Libtool installation directory!)

    $ libtoolize --version
    libtoolize (GNU libtool) 2.2.6
    Written by Gary V. Vaughan <gary@gnu.org>, 2003
    
    $ libtoolize -fi

and then run again:

    $ autoreconf -vfi

This should do. There is an obscure bug with autoreconf not discovering that libtool is being used which seems only triggered in environments where the Libtool being used is not system-wide installed, so it might not be a problem in your environment.

If autoreconf ends successfully you can follow the usual installation steps.

Build requirements when building from the GIT repository
If you are building Nanos++ from the git repository you will also need:

- Automake 1.10 or better
- Autoconf 2.63 or better
- Libtool 2.2.6a or better
- git 1.7.0 or better (using https with git requires a HTTPS-enabled libcurl in your system