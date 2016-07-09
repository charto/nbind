/*
Copyright (c) 2010-2014 Emscripten authors, see EMSCRIPTEN-AUTHORS file.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#pragma once

namespace emscripten {
    namespace internal {

        // TypeList<>

        template<typename...>
        struct TypeList {};

        // Cons :: T, TypeList<types...> -> Cons<T, types...>

        template<typename First, typename TypeList>
        struct Cons;

        template<typename First, typename... Rest>
        struct Cons<First, TypeList<Rest...>> {
            typedef TypeList<First, Rest...> type;
        };

        // Apply :: T, TypeList<types...> -> T<types...>

        template<template<typename...> class Output, typename TypeList>
        struct Apply;

        template<template<typename...> class Output, typename... Types>
        struct Apply<Output, TypeList<Types...>> {
            typedef Output<Types...> type;
        };

        // MapWithIndex_

        template<typename PolicyList, template<typename, size_t, typename> class Mapper, size_t CurrentIndex, typename... Args>
        struct MapWithIndex_;

        template<typename PolicyList, template<typename, size_t, typename> class Mapper, size_t CurrentIndex, typename First, typename... Rest>
        struct MapWithIndex_<PolicyList, Mapper, CurrentIndex, First, Rest...> {
            typedef typename Cons<
                Mapper<PolicyList, CurrentIndex, First>,
                typename MapWithIndex_<PolicyList, Mapper, CurrentIndex + 1, Rest...>::type
                >::type type;
        };

        template<typename PolicyList, template<typename, size_t, typename> class Mapper, size_t CurrentIndex>
        struct MapWithIndex_<PolicyList, Mapper, CurrentIndex> {
            typedef TypeList<> type;
        };

        template<typename PolicyList, template<typename...> class Output, template<typename, size_t, typename> class Mapper, typename... Args>
        struct MapWithIndex {
            typedef typename internal::Apply<
                Output,
                typename MapWithIndex_<PolicyList, Mapper, 0, Args...>::type
            >::type type;
        };
    }
}
