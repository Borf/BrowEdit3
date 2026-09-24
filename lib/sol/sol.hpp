// The MIT License (MIT)

// Copyright (c) 2013-2022 Rapptz, ThePhD and contributors

// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef SOL_HPP
#define SOL_HPP

#include <lib/sol/version.hpp>

#if SOL_IS_ON(SOL_INSIDE_UNREAL_ENGINE)
#ifdef check
#pragma push_macro("check")
#undef check
#endif
#endif // Unreal Engine 4 Bullshit

#if SOL_IS_ON(SOL_COMPILER_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wconversion"
#if __GNUC__ > 6
#pragma GCC diagnostic ignored "-Wnoexcept-type"
#endif
#elif SOL_IS_ON(SOL_COMPILER_CLANG)
// we'll just let this alone for now
#elif SOL_IS_ON(SOL_COMPILER_VCXX)
#pragma warning(push)
#pragma warning(disable : 4505) // unreferenced local function has been removed GEE THANKS
#endif                          // clang++ vs. g++ vs. VC++

#include <lib/sol/forward.hpp>
#include <lib/sol/forward_detail.hpp>
#include <lib/sol/assert.hpp>
#include <lib/sol/bytecode.hpp>
#include <lib/sol/stack.hpp>
#include <lib/sol/object.hpp>
#include <lib/sol/function.hpp>
#include <lib/sol/protected_function.hpp>
#include <lib/sol/usertype.hpp>
#include <lib/sol/table.hpp>
#include <lib/sol/state.hpp>
#include <lib/sol/coroutine.hpp>
#include <lib/sol/thread.hpp>
#include <lib/sol/userdata.hpp>
#include <lib/sol/metatable.hpp>
#include <lib/sol/as_args.hpp>
#include <lib/sol/variadic_args.hpp>
#include <lib/sol/variadic_results.hpp>
#include <lib/sol/lua_value.hpp>

#if SOL_IS_ON(SOL_COMPILER_GCC)
#pragma GCC diagnostic pop
#elif SOL_IS_ON(SOL_COMPILER_CLANG)
// we'll just let this alone for now
#elif SOL_IS_ON(SOL_COMPILER_VCXX)
#pragma warning(pop)
#endif // g++

#if SOL_IS_ON(SOL_INSIDE_UNREAL_ENGINE)
#undef check
#pragma pop_macro("check")
#endif // Unreal Engine 4 Bullshit

#endif // SOL_HPP
