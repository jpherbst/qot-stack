# Content description #

This is a folder of unit tests for the QoT stack. In order to run these tests you will need CMake 2.8+ and the GTest framework installed and compiled.

    #> sudo apt-get install libgtest-dev
    #> cd /usr/src/gtest
    #> sudo cmake CMakeLists.txt
    #> sudo make
    #> cd /usr/lib
    #> sudo ln -s /usr/src/gtest/libgtest.a 
    #> sudo ln -s /usr/src/gtest/libgtest_main.a 

Then, you will be able to make code:

    #> make

And, run the tests

    #> make test

If you want lots of information, try

    #> ctest -V
