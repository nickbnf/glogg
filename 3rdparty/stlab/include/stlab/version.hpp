/*
    Copyright 2015 Adobe
    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

//  See http://www.stlab.cc for documentation

#ifndef STLAB_VERSION_HPP
#define STLAB_VERSION_HPP

//
//  Caution: this is the only stlab header that is guaranteed
//  to change with every stlab release. Including this header
//  will cause a recompile every time a new stlab version is
//  used.
//
//  STLAB_VERSION % 100 is the patch level
//  STLAB_VERSION / 100 % 1000 is the minor version
//  STLAB_VERSION / 100000 is the major version

#define STLAB_VERSION 100200

//
//  STLAB_LIB_VERSION must be defined to be the same as STLAB_VERSION
//  but as a *string* in the form "x_y[_z]" where x is the major version
//  number, y is the minor version number, and z is the patch level if not 0.

#define STLAB_LIB_VERSION "1_2_0"

#endif
