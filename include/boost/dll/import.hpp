// Copyright 2014 Renato Tegon Forti, Antony Polukhin.
// Copyright 2015 Antony Polukhin.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_DLL_IMPORT_HPP
#define BOOST_DLL_IMPORT_HPP

#include <boost/config.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_object.hpp>
#include <boost/function.hpp>
#include <boost/make_shared.hpp>
#include <boost/dll/shared_library.hpp>
#include <boost/dll/detail/strip_calling_convention.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
# pragma once
#endif

/// \file boost/dll/import.hpp
/// \brief Contains all the boost::dll::import* reference counting
/// functions that hold a shared pointer to the instance of
/// boost::dll::shared_library.

namespace boost { namespace dll {

namespace detail {
    template <class T>
    class refc_function {
        boost::shared_ptr<shared_library>   lib_;
        T*                                  func_ptr_;

    public:
        refc_function(const boost::shared_ptr<shared_library>& lib, T* func_ptr) BOOST_NOEXCEPT
            : lib_(lib)
            , func_ptr_(func_ptr)
        {}

        operator T*() const BOOST_NOEXCEPT {
            return func_ptr_;
        }
    };

    template <class T, class = void>
    struct import_type;

    template <class T>
    struct import_type<T, typename boost::disable_if<boost::is_object<T> >::type> {
        typedef boost::dll::detail::refc_function<T> base_type;
        typedef boost::function<
            typename boost::dll::detail::strip_calling_convention<T>::type
        > type;
    };

    template <class T>
    struct import_type<T, typename boost::enable_if<boost::is_object<T> >::type> {
        typedef boost::shared_ptr<T> base_type;
        typedef boost::shared_ptr<T> type;
    };
} // namespace detail


#ifndef BOOST_DLL_DOXYGEN
#   define BOOST_DLL_IMPORT_RESULT_TYPE inline typename boost::dll::detail::import_type<T>::type
#endif


/*!
* Returns boost::function<T> or boost::shared_ptr<T> that holds an imported function or variable
* from the loaded library and refcounts usage
* of the loaded shared library, so that it won't get unload until all copies of return value
* are not destroyed.
*
* This call will succeed if call to \forcedlink{shared_library}`::has(const char* )`
* function with the same symbol name returned `true`.
*
* For importing symbols by \b alias names use \forcedlink{import_alias} method.
*
* \b Examples:
* \code
* boost::function<int(int)> f = import<int(int)>(
*           boost::make_shared<shared_library>("test_lib.so"),
*           "integer_func_name"
*       );
* \endcode
*
* \code
* boost::function<int(int)> f = import<int(int)>("test_lib.so", "integer_func_name");
* \endcode
*
* \code
* boost::shared_ptr<int> i = import<int>("test_lib.so", "integer_name");
* \endcode
*
* \b Template \b parameter \b T:    Type of the symbol that we are going to import. Must be explicitly specified.
*
* \param lib Path or shared pointer to library to load function from.
* \param name Null-terminated C or C++ mangled name of the function to import. Can handle std::string, char*, const char*.
* \param mode An mode that will be used on library load.
*
* \return boost::function<T> if T is a function type, or boost::shared_ptr<T> if T is an object type.
*
* \throw boost::system::system_error if symbol does not exist or if the DLL/DSO was not loaded.
*       Overload that accepts path also throws std::bad_alloc in case of insufficient memory.
*/
template <class T>
BOOST_DLL_IMPORT_RESULT_TYPE import(const boost::filesystem::path& lib, const char* name,
    load_mode::type mode = load_mode::default_mode);

//! \overload boost::dll::import(const boost::filesystem::path& lib, const char* name, load_mode::type mode)
template <class T>
BOOST_DLL_IMPORT_RESULT_TYPE import(const boost::filesystem::path& lib, const std::string& name,
    load_mode::type mode = load_mode::default_mode)
{
    return boost::dll::import<T>(
        boost::make_shared<boost::dll::shared_library>(lib, mode),
        name.c_str()
    );
}

//! \overload boost::dll::import(const boost::filesystem::path& lib, const char* name, load_mode::type mode)
template <class T>
BOOST_DLL_IMPORT_RESULT_TYPE import(const boost::shared_ptr<shared_library>& lib, const char* name) {
    typedef typename boost::dll::detail::import_type<T>::base_type type;
    return type(lib, &lib->get<T>(name));
}

//! \overload boost::dll::import(const boost::filesystem::path& lib, const char* name, load_mode::type mode)
template <class T>
BOOST_DLL_IMPORT_RESULT_TYPE import(const boost::shared_ptr<shared_library>& lib, const std::string& name) {
    return boost::dll::import<T>(lib, name.c_str());
}

template <class T>
BOOST_DLL_IMPORT_RESULT_TYPE import(const boost::filesystem::path& lib, const char* name, load_mode::type mode) {
    return boost::dll::import<T>(
        boost::make_shared<boost::dll::shared_library>(lib, mode),
        name
    );
}




/*!
* Returns boost::function<T> or boost::shared_ptr<T> that holds an imported function or variable
* from the loaded library and refcounts usage
* of the loaded shared library, so that it won't get unload until all copies of return value
* are not destroyed.
*
* This call will succeed if call to \forcedlink{shared_library}`::has(const char* )`
* function with the same symbol name returned `true`.
*
* For importing symbols by \b non \b alias names use \forcedlink{import} method.
*
* \b Examples:
* \code
* boost::function<int(int)> f = import_alias<int(int)>(
*           boost::make_shared<shared_library>("test_lib.so"),
*           "integer_func_alias_name"
*       );
* \endcode
*
* \code
* boost::function<int(int)> f = import_alias<int(int)>("test_lib.so", "integer_func_alias_name");
* \endcode
*
* \code
* boost::shared_ptr<int> i = import_alias<int>("test_lib.so", "integer_alias_name");
* \endcode
*
* \b Template \b parameter \b T:    Type of the symbol alias that we are going to import. Must be explicitly specified.
*
* \param lib Path or shared pointer to library to load function from.
* \param name Null-terminated C or C++ mangled name of the function or variable to import. Can handle std::string, char*, const char*.
* \param mode An mode that will be used on library load.
*
* \return boost::function<T> if T is a function type, or boost::shared_ptr<T> if T is an object type.
*
* \throw boost::system::system_error if symbol does not exist or if the DLL/DSO was not loaded.
*       Overload that accepts path also throws std::bad_alloc in case of insufficient memory.
*/
template <class T>
BOOST_DLL_IMPORT_RESULT_TYPE import_alias(const boost::filesystem::path& lib, const char* name,
    load_mode::type mode = load_mode::default_mode);

//! \overload boost::dll::import_alias(const boost::filesystem::path& lib, const char* name, load_mode::type mode)
template <class T>
BOOST_DLL_IMPORT_RESULT_TYPE import_alias(const boost::filesystem::path& lib, const std::string& name,
    load_mode::type mode = load_mode::default_mode)
{
    return boost::dll::import_alias<T>(
        boost::make_shared<boost::dll::shared_library>(lib, mode),
        name.c_str()
    );
}

//! \overload boost::dll::import_alias(const boost::filesystem::path& lib, const char* name, load_mode::type mode)
template <class T>
BOOST_DLL_IMPORT_RESULT_TYPE import_alias(const boost::shared_ptr<shared_library>& lib, const std::string& name) {
    return boost::dll::import_alias<T>(lib, name.c_str());
}

//! \overload boost::dll::import_alias(const boost::filesystem::path& lib, const char* name, load_mode::type mode)
template <class T>
BOOST_DLL_IMPORT_RESULT_TYPE import_alias(const boost::shared_ptr<shared_library>& lib, const char* name) {
    typedef typename boost::dll::detail::import_type<T>::base_type type;
    return type(lib, lib->get<T*>(name));
}

template <class T>
BOOST_DLL_IMPORT_RESULT_TYPE import_alias(const boost::filesystem::path& lib, const char* name, load_mode::type mode) {
    return boost::dll::import_alias<T>(
        boost::make_shared<boost::dll::shared_library>(lib, mode),
        name
    );
}
#undef BOOST_DLL_IMPORT_RESULT_TYPE


}} // boost::dll

#endif // BOOST_DLL_IMPORT_HPP

