WSync readme

===== Used libs

== SHA1 lib
Files: SHA1.cpp, SHA1.h
Source: http://www.codeproject.com/KB/recipes/csha1.aspx
License: 100% free public domain implementation of the SHA-1 algorithm (see source code)

== boost ASIO
Source: www.boost.org/doc/libs/release/libs/asio/index.html
License: Boost License, compatible with closed source

== boost Filesystem
Source: www.boost.org/doc/libs/release/libs/filesystem/index.html
License: Boost License, compatible with closed source

===== How to install
1) get boost: 
 - for windows: http://www.boostpro.com/products/free
 - http://www.boost.org/users/download/
2) download and unzip boost somewhere
3) run cmake and set the WSYNC_BOOST_DIR variable to your boost path
 - for example to "C:\Program Files\boost\boost_1_36_0"
4) generate cmake buildsystem
5) build and have fun