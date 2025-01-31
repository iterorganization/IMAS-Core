Troubleshooting
===============

1. **Target Boost::log already has an imported location**
   This problem is known to occur with the 2020b toolchain on SDCC. Add the CMake configuration option -D Boost_NO_BOOST_CMAKE=ON to work around the problem.n.

