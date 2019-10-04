README
======

  This is a test of the C++ standard library.  At present a port of the uClibc++
  C++ library is available.  Due to licensing issues, the uClibc++ C++ library
  is not included in the NuttX source tree by default, but must be installed
  (see the README.txt file in the uClibc++ download package for installation).

  The uClibc++ test includes simple test of:

    - iostreams,
    - STL,
    - RTTI, and
    - Exceptions

  Example Configuration Options
  -----------------------------
    CONFIG_TESTING_CXXTEST=y - Eanbles the example
    CONFIG_TESTING_CXXTEST_CXXINITIALIZE=y - By default, if CONFIG_HAVE_CXX
      and CONFIG_HAVE_CXXINITIALIZE are defined, then this example
      will call the NuttX function to initialize static C++ constructors.
      This option may be disabled, however, if that static initialization
      was performed elsewhere.

  Other Required Configuration Settings
  -------------------------------------
  Other NuttX setting that are required include:

    CONFIG_HAVE_CXX=y
    CONFIG_HAVE_CXXINITIALIZE=y
    CONFIG_UCLIBCXX=y

  Additional uClibc++ settings may be required in your build environment.

