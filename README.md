# Server application library

[![Travis](https://img.shields.io/travis/svens/sal/master.svg?style=flat-square)](https://travis-ci.org/svens/sal)
[![AppVeyor](https://img.shields.io/appveyor/ci/svens/sal.svg?style=flat-square)](https://ci.appveyor.com/project/svens/sal/branch/master)
[![Coveralls](https://img.shields.io/coveralls/svens/sal.svg?style=flat-square)](https://coveralls.io/github/svens/sal?branch=master)
[![Coverity](https://img.shields.io/coverity/scan/10116.svg?style=flat-square)](https://scan.coverity.com/projects/svens-sal)

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
