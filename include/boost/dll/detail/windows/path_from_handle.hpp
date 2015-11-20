// Copyright 2014 Renato Tegon Forti, Antony Polukhin.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_DLL_DETAIL_WINDOWS_PATH_FROM_HANDLE_HPP
#define BOOST_DLL_DETAIL_WINDOWS_PATH_FROM_HANDLE_HPP

#include <boost/config.hpp>
#include <boost/dll/detail/system_error.hpp>
#include <boost/detail/winapi/dll.hpp>
#include <boost/detail/winapi/GetLastError.hpp>
#include <boost/filesystem/path.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
# pragma once
#endif

namespace boost { namespace dll { namespace detail {


    static inline boost::system::error_code last_error_code() BOOST_NOEXCEPT {
        return boost::system::error_code(
            boost::detail::winapi::GetLastError(),
            boost::system::system_category()
        );
    }

    inline boost::filesystem::path path_from_handle(boost::detail::winapi::HMODULE_ handle, boost::system::error_code &ec) {
        boost::filesystem::path ret;
        
        BOOST_STATIC_CONSTANT(boost::detail::winapi::DWORD_, ERROR_INSUFFICIENT_BUFFER_ = 0x7A);
        BOOST_STATIC_CONSTANT(boost::detail::winapi::DWORD_, DEFAULT_PATH_SIZE_ = 260);

        // A handle to the loaded module whose path is being requested.
        // If this parameter is NULL, GetModuleFileName retrieves the path of the
        // executable file of the current process.
        boost::detail::winapi::WCHAR_ path_hldr[DEFAULT_PATH_SIZE_];
        boost::detail::winapi::LPWSTR_ path = path_hldr;
        boost::detail::winapi::GetModuleFileNameW(handle, path, DEFAULT_PATH_SIZE_);
        ec = last_error_code();

        // In case of ERROR_INSUFFICIENT_BUFFER_ trying to get buffer big enough to store the whole path
        for (unsigned i = 2; i < 1025 && ec.value() == ERROR_INSUFFICIENT_BUFFER_; i *= 2) {
            path = new boost::detail::winapi::WCHAR_[DEFAULT_PATH_SIZE_ * i];

            boost::detail::winapi::GetModuleFileNameW(handle, path, DEFAULT_PATH_SIZE_ * i);
            ec = last_error_code();

            if (ec) {
                delete[] path;
            }
        }
        
        if (ec) {
            // Error other than ERROR_INSUFFICIENT_BUFFER_ occurred or failed to allocate buffer big enough
            return boost::filesystem::path();
        }
        
        ret = path;
        if (path != path_hldr) {
            delete[] path;
        }
        
        return ret;
    }

}}} // namespace boost::dll::detail

#endif // BOOST_DLL_DETAIL_WINDOWS_PATH_FROM_HANDLE_HPP

