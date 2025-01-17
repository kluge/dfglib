# dfglib

Experimental general purpose utility library for C++.

Note: this is *not* a mature library and is not intended or recommended for general use. Libraries such as [Abseil](https://abseil.io/) or [Boost](https://www.boost.org/) may provide many of the features in dfglib implemented in a more clear and professional manner. For a comprehensive list of alternatives, see [A list of open source C++ libraries at cppreference.com](https://en.cppreference.com/w/cpp/links/libs) 

## News

* 2019-09-10: dfgQtTableEditor [version 1.0.0.0](https://github.com/tc3t/dfglib/releases/tag/dfgQtTableEditor_1.0.0.0)

## Usage

Large proportion of features are 'header only' and can be used simply by including header without anything to link. When .cpp -files are needed, the intended usage is to include them in the build of the target project; there are no cmake-files etc. for building dfglib as a library.

### Building and running unit tests (dfgTest)

* Make sure that Boost is available in include path

1. With command line CMake
    * cd dfgTest
    * cmake CMakeLists.txt
    * make
    * ./dfgTest
2. Visual studio project:
    * Open dfgTest/dfgTest.sln
    * Build and run dfgTest-project with an available MSVC-compiler.

For a list of supported compilers, see "Build status"-section in this document.

## Features

The library consists of miscellaneous features such as algorithms, containers, math/numerics, streams, typed string and UTF-handling. Below are some notable features:

* Streams
    * [non-virtual streams](dfg/io/) with basic interface compatibility with standard streams.
    * [encoding-aware streams](dfg/io/) (i.e. streams that can read/write e.g. UTF-encoding (UTF8, UTF16BE/LE, UTF32BE/LE))

* CSV-file [reading/writing](dfg/io/) with [somewhat reasonable performance](misc/csvPerformanceRuns.md) (e.g. compared to some spreadsheet applications) and [Qt-widgets](dfg/qt/) for editing CSV-files.

* Simple CSV-editor as an example, for details see it's [readme](dfgExamples/dfgQtTableEditor/README.md/)

* Typed strings (i.e. strings that store encoded text instead of raw bytes)

* Algorithms such as
    * [median](dfg/numeric/median.hpp) & [percentile](dfg/numeric/percentile.hpp)
    * [polynomial evaluation](dfg/math/evalPolynomial.hpp) (Horner's method)
    * [spearman correlation](dfg/dataAnalysis/correlation.hpp)
    * [spreadsheet sorting](dfg/alg/sortMultiple.hpp)

* Containers such as [flat map](dfg/cont/MapVector.hpp) and [flat set](dfg/cont/SetVector.hpp)

## Third party code

Summary of 3rd party code in dfglib (last revised 2019-06-12).

| Library      | Usage      | License  | Comment |
| ------------- | ------------- | ----- | ------- |
| [Boost](http://www.boost.org/)  | i,m,ti (used in numerous places)          | [Boost software license](http://www.boost.org/LICENSE_1_0.txt) | Exact requirement for Boost version is unknown; unit tests have been successfully build and run with Boost versions 1.55, 1.61, 1.65.1 and 1.70.0 |
| [Colour Rendering of Spectra](dfg/colour/specRendJw.cpp) | m (used in colour handling tools) | [Public domain](dfg/colour/specRendJw.cpp) | 
| [cppcsv](https://github.com/paulharris/cppcsv) | c,t | [MIT](https://github.com/paulharris/cppcsv) | 
| [dlib](http://dlib.net/)    | m,ti (unit-aware integration and various tests)           | [Boost software license](http://www.boost.org/LICENSE_1_0.txt)  | Can be excluded from unit tests with option DFGTEST_BUILD_OPT_USE_DLIB
| [fast-csv-cpp-parser](https://github.com/ben-strasser/fast-cpp-csv-parser/) | c,t | [BSD-3](dfg/io/fast-cpp-csv-parser/csv.h) |
| [fmtlib](https://github.com/fmtlib/fmt) | m (string formatting)| [BSD-2](dfg/str/fmtlib/format.h) |
| [Google Test](https://github.com/google/googletest) (version 1.8.1) | t | [BSD-3](externals/gtest/gtest.h) |
| [LibQxt](https://bitbucket.org/libqxt/libqxt/wiki/Home) | c,t (QxtSpanSlider) | [BSD-3](dfg/qt/qxt/core/qxtglobal.h) |
| [Qt 5](https://www.qt.io/) | i (only for components in dfg/qt) | [Various](http://doc.qt.io/qt-5/licensing.html) |
| [UTF8-CPP](https://github.com/nemtrif/utfcpp) (version 3.1) | m (utf handling) | [Boost software license](dfg/utf/utf8_cpp/utf8.h) |

Usage types:
* c: All or some code from the library comes with dfglib (possibly modified), but is not directly used (i.e. related code can be removed without breaking any features in dfglib).
* i: Include dependency (i.e. some parts of dfglib may fail to compile if the 3rd party library is not available in include-path)
* m: Some code is integrated in dfglib itself possibly modified.
* t: Used in test code without (external) include dependency (i.e. the needed code comes with dfglib).
* ti: Used in test code with include dependency.

## Build status (as of 2019-06-10 commit [bdd9ffc](https://github.com/tc3t/dfglib/commit/bdd9ffc26cfb24509e420c56c82f3e21a4aa4937), with Boost 1.70.0 unless stated otherwise)

<!-- [![Build status](https://ci.appveyor.com/api/projects/status/89v23h19mvv9k5u3/branch/master?svg=true)](https://ci.appveyor.com/project/tc3t/dfglib/branch/master) -->

| Compiler      | Platform      | Config  | Tests (passed/all) | Comment |
| ------------- | ------------- | -----   | ------  | ------- |
| Clang 3.8.0   | x86           |         | 100 % (220/220) | Boost 1.61, Ubuntu 32-bit 16.04 |
| Clang 6.0.0   | x64           | Release | 100 % (220/220) | Boost 1.65.1, Ubuntu 64-bit 18.04 |
| GCC 5.4.0     | x86           |         | 100 % (220/220) | Boost 1.61, Ubuntu 32-bit 16.04 |
| GCC 7.4.0     | x64           | Release | 100 % (220/220) | Boost 1.65.1, Ubuntu 64-bit 18.04 |
| MinGW 4.8.0   | x86           | O2      | 100 % (223/223) | |
| VC2010        | x86           | Debug   | 100 % (223/223) | |
| VC2010        | x86           | Release | 100 % (223/223) | |
| VC2010        | x64           | Debug   | 100 % (223/223) | |
| VC2010        | x64           | Release | 100 % (223/223) | |
| VC2012        | x86           | Debug   | 100 % (226/226) | |
| VC2012        | x86           | Release | 100 % (226/226) | |
| VC2012        | x64           | Debug   | 100 % (226/226) | |
| VC2012        | x64           | Release | 100 % (226/226) | |
| VC2013        | x86           | Debug   | 100 % (226/226) | |
| VC2013        | x86           | Release | 100 % (226/226) | |
| VC2013        | x64           | Debug   | 100 % (226/226) | |
| VC2013        | x64           | Release | 100 % (226/226) | |
| VC2015        | x86           | Debug   | 100 % (226/226) | |
| VC2015        | x86           | Release | 99 % (225/226) | Numerical precision related failure in dfgNumeric.transform |
| VC2015        | x64           | Debug   | 100 % (226/226) | |
| VC2015        | x64           | Release | 99 % (225/226) | Numerical precision related failure in dfgNumeric.transform |
| VC2017        | x86           | Debug   | 100 % (226/226) | |
| VC2017        | x86           | Release | 99 % (225/226) | Numerical precision related failure in dfgNumeric.transform |
| VC2017        | x64           | Debug   | 100 % (226/226) | |
| VC2017        | x64           | Release | 99 % (225/226) | Numerical precision related failure in dfgNumeric.transform |
| VC2019        | x86           | Debug   | 100 % (225/225) | std:c++17 with Conformance mode |
| VC2019        | x86           | Release | 100 % (225/225) | std:c++17 with Conformance mode |
| VC2019        | x64           | Debug   | 100 % (225/225) | std:c++17 with Conformance mode |
| VC2019        | x64           | Release | 100 % (225/225) | std:c++17 with Conformance mode |
| Others        |               |         | Not tested |  |
||||||
