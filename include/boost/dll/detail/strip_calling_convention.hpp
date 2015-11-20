// Copyright 2015 Antony Polukhin.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_DLL_DETAIL_STRIP_CALLING_CONVENTION_HPP
#define BOOST_DLL_DETAIL_STRIP_CALLING_CONVENTION_HPP

#include <boost/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
# pragma once
#endif

namespace boost { namespace dll { namespace detail {

template <class T>
struct strip_calling_convention {
    typedef T type;
};

#ifdef _MSC_VER

// stripping __fastcall

template <class Ret, class Arg1>
struct strip_calling_convention<Ret __fastcall (Arg1)> {
    typedef Ret(type)(Arg1);
};

template <class Ret, class Arg1, class Arg2>
struct strip_calling_convention<Ret __fastcall (Arg1, Arg2)> {
    typedef Ret(type)(Arg1, Arg2);
};

template <class Ret, class Arg1, class Arg2, class Arg3>
struct strip_calling_convention<Ret __fastcall (Arg1, Arg2, Arg3)> {
    typedef Ret(type)(Arg1, Arg2, Arg3);
};

template <class Ret, class Arg1, class Arg2, class Arg3, class Arg4>
struct strip_calling_convention<Ret __fastcall (Arg1, Arg2, Arg3, Arg4)> {
    typedef Ret(type)(Arg1, Arg2, Arg3, Arg4);
};

template <class Ret, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
struct strip_calling_convention<Ret __fastcall (Arg1, Arg2, Arg3, Arg4, Arg5)> {
    typedef Ret(type)(Arg1, Arg2, Arg3, Arg4, Arg5);
};

template <class Ret, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6>
struct strip_calling_convention<Ret __fastcall (Arg1, Arg2, Arg3, Arg4, Arg5, Arg6)> {
    typedef Ret(type)(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6);
};

template <class Ret, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7>
struct strip_calling_convention<Ret __fastcall (Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7)> {
    typedef Ret(type)(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7);
};

template <class Ret, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7, class Arg8>
struct strip_calling_convention<Ret __fastcall (Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8)> {
    typedef Ret(type)(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8);
};

template <class Ret, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7, class Arg8, class Arg9>
struct strip_calling_convention<Ret __fastcall (Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8, Arg9)> {
    typedef Ret(type)(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8, Arg9);
};

// stripping __stdall
template <class Ret, class Arg1>
struct strip_calling_convention<Ret __stdcall (Arg1)> {
    typedef Ret(type)(Arg1);
};

template <class Ret, class Arg1, class Arg2>
struct strip_calling_convention<Ret __stdcall (Arg1, Arg2)> {
    typedef Ret(type)(Arg1, Arg2);
};

template <class Ret, class Arg1, class Arg2, class Arg3>
struct strip_calling_convention<Ret __stdcall (Arg1, Arg2, Arg3)> {
    typedef Ret(type)(Arg1, Arg2, Arg3);
};

template <class Ret, class Arg1, class Arg2, class Arg3, class Arg4>
struct strip_calling_convention<Ret __stdcall (Arg1, Arg2, Arg3, Arg4)> {
    typedef Ret(type)(Arg1, Arg2, Arg3, Arg4);
};

template <class Ret, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
struct strip_calling_convention<Ret __stdcall (Arg1, Arg2, Arg3, Arg4, Arg5)> {
    typedef Ret(type)(Arg1, Arg2, Arg3, Arg4, Arg5);
};

template <class Ret, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6>
struct strip_calling_convention<Ret __stdcall (Arg1, Arg2, Arg3, Arg4, Arg5, Arg6)> {
    typedef Ret(type)(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6);
};

template <class Ret, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7>
struct strip_calling_convention<Ret __stdcall (Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7)> {
    typedef Ret(type)(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7);
};

template <class Ret, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7, class Arg8>
struct strip_calling_convention<Ret __stdcall (Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8)> {
    typedef Ret(type)(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8);
};

template <class Ret, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7, class Arg8, class Arg9>
struct strip_calling_convention<Ret __stdcall (Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8, Arg9)> {
    typedef Ret(type)(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8, Arg9);
};

#ifndef BOOST_NO_CXX11_VARIADIC_TEMPLATES

// Some of the MSVC versions have issues in variadic templates implementation.
// So we leave previous specializations visible even if variadic templates are allowed

template <class Ret, class... Args>
struct strip_calling_convention<Ret __fastcall (Args...)> {
    typedef Ret(type)(Args...);
};

template <class Ret, class... Args>
struct strip_calling_convention<Ret __stdcall (Args...)> {
    typedef Ret(type)(Args...);
};
#endif // #ifndef BOOST_NO_CXX11_VARIADIC_TEMPLATES

#endif // #ifdef _MSC_VER


}}} // namespace boost::dll::detail


#endif // #ifndef BOOST_DLL_DETAIL_STRIP_CALLING_CONVENTION_HPP
