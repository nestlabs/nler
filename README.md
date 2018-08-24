[![Build Status][nler-travis-svg]][nler-travis]
[![Coverage Status][nler-codecov-svg]][nler-codecov]

Nest Labs Embedded Runtime (NLER)
=================================

# Introduction

NLER is designed to supply the minimal building blocks necessary to create event passing 
applications in deeply embedded environments.

NLER provides abstractions for:

* atomic operations
* (event) queues
* locks
* logging
* threads
* timers

NLER currently supports the above abstractions against the following build platforms:

* FreeRTOS
* Netscape Portable Runtime (NSPR)
* POSIX Threads (pthreads)

[nler-travis]: https://travis-ci.org/nestlabs/nler
[nler-travis-svg]: https://travis-ci.org/nestlabs/nler.svg?branch=master
[nler-codecov]: https://codecov.io/gh/nestlabs/nler
[nler-codecov-svg]: https://codecov.io/gh/nestlabs/nler/branch/master/graph/badge.svg

# Interact

There are numerous avenues for nler support:

  * Bugs and feature requests — [submit to the Issue Tracker](https://github.com/nestlabs/nler/issues)
  * Google Groups — discussion and announcements
    * [nler-announce](https://groups.google.com/forum/#!forum/nler-announce) — release notes and new updates on nler
    * [nler-users](https://groups.google.com/forum/#!forum/nler-users) — discuss use of and enhancements to nler

# Versioning

nler follows the [Semantic Versioning guidelines](http://semver.org/) 
for release cycle transparency and to maintain backwards compatibility.

# License

nler is released under the [Apache License, Version 2.0 license](https://opensource.org/licenses/Apache-2.0). 
See the `LICENSE` file for more information.
