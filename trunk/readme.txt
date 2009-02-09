This is Rigs of Rods

Copyright 2005,2006,2007,2008,2009 Pierre-Michel Ricordel
Copyright 2007,2008,2009 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as 
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.

DIRECTORY CONTENTS

build: build environment

test: test environment (generated from build)

deploy: upload environment (generated from test)

tools: building tools (build->test, test->deploy)


HOWTO BUILD (new style)
1. Use CMake with source code in RoRdev\build, and build directory RoRdev\build\build
2. Open project in RoRdev\build\build\RoR.sln
3. Build target INSTALL
4. Build contents     : RoRdev\build\contents\build_contents.py
5. Build streams      : RoRdev\streams\build_streams.py
6. Switch to test mode: RoRdev\build2test.py
7. go to RoRdev\test\current to find the release directory

HOWTO BUILD (new style) under linux
1. make a new directory and change to it: RoRdev\build\build
2. execute: ccmake ..
3. execute: make -j <your number of CPU cores +1>
4. execute: make install
5. Build contents     : cd RoRdev\build\contents\ && python build_contents.py
6. Build streams      : cd RoRdev\streams\ && python build_streams.py
7. Switch to test mode: cd RoRdev\ && python build2test.py
8. go to RoRdev\test\current to find the release directory