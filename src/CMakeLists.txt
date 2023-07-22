#
# Author:      Richard Cornwell (rich@sky-visions.com)
#
# Copyright 2023, Richard Cornwell
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

set(GB_PATH ${CMAKE_CURRENT_SOURCE_DIR})
## Use all the *.cpp files we found under this folder for the project
FILE(GLOB GB_SRCS Apu.cpp Cpu.cpp ROM.cpp Memory.cpp Ppu.cpp Cartridge.cpp)
target_sources(${PROJECT_NAME} PUBLIC main.cpp ${GB_SRCS})

if (RUN_TESTS) 
add_executable(gb_test ${GB_SRCS} test/main.cpp test/cpu_instrs.cpp test/instr_timing.cpp test/mem_timing.cpp)
target_include_directories(gb_test PRIVATE ${GB_PATH})
target_include_directories(gb_test PRIVATE ${CPPUTEST_INCLUDE_DIRS})
add_test(NAME gb_test COMMAND gb_test 
	WORKING_DIRECTORY ${GB_PATH}/test)
target_link_directories(gb_test PRIVATE ${CPPUTEST_LIBRARIES})
if(MSVC)
    target_link_libraries(gb_test winmm)
endif()
target_link_libraries(gb_test ${CPPUTEST_LDFLAGS})
#add_custom_target(gb_test COMMAND gb_test)
#add_custom_command(TARGET gb_test POST_BUILD 
#	COMMAND gb_test
#	WORKING_DIRECTORY ${GB_PATH}/test
#	COMMENT "Running gb test" VERBATIM)
endif()


