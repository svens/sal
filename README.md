# Server application library

[![Build Status](https://travis-ci.org/svens/sal.svg?branch=master)](https://travis-ci.org/svens/sal)
[![Build status](https://ci.appveyor.com/api/projects/status/2kign69ypgoy6pam/branch/master?svg=true)](https://ci.appveyor.com/project/svens/sal/branch/master)
[![Coverage](https://coveralls.io/repos/github/svens/sal/badge.svg?branch=master)](https://coveralls.io/github/svens/sal?branch=master)
[![Coverity](https://scan.coverity.com/projects/10116/badge.svg)](https://scan.coverity.com/projects/svens-sal)

SAL is portable simple application library providing functionality for
threaded networking applications (configuring, logging, etc). Goal is to cover
Linux, Windows and OSX platforms.


## Compiling and installing

    $ mkdir build && cd build
    $ cmake .. [-DSAL_UNITTESTS=yes|no] [-DSAL_BENCH=yes|no] [-DSAL_DOCS=yes|no]
    $ make && make test && make install


## Source tree

The source tree is organised as follows:

    .               Root of source tree
    |- sal          Library ...
    |  `- module    ... per module headers/sources/tests
    |- cmake        CMake helper stuff
    |- bench        Benchmarking application
    `- scripts      Helper scripts
