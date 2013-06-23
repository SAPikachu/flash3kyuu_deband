How to compile the test project
===============================

1. Download and extract [Google Test 1.6](http://code.google.com/p/googletest/downloads/list). 
   In this note I will assume that it is extracted to `C:\gtest`.

2. Add a environment variable named `GTEST_ROOT` to your system, set its value to `C:\gtest`.

3. Open `C:\gtest\msvc\gtest.sln`, compile it in 4 configurations (Debug/Release x Win32/x64).
   
   Notes:

   * Make sure configuration type of all the subprojects is `Static library (.lib)`

   * You may need to add `_VARIADIC_MAX=10` to preprocessor definitions to make it compile

4. Open `f3kdb_test.vcxproj`. it should be ready to build now.