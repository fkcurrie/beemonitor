# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/frank/Projects/beemonitor/esp-idf/components/bootloader/subproject"
  "/home/frank/Projects/beemonitor/beemonitor/build/bootloader"
  "/home/frank/Projects/beemonitor/beemonitor/build/bootloader-prefix"
  "/home/frank/Projects/beemonitor/beemonitor/build/bootloader-prefix/tmp"
  "/home/frank/Projects/beemonitor/beemonitor/build/bootloader-prefix/src/bootloader-stamp"
  "/home/frank/Projects/beemonitor/beemonitor/build/bootloader-prefix/src"
  "/home/frank/Projects/beemonitor/beemonitor/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/frank/Projects/beemonitor/beemonitor/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/frank/Projects/beemonitor/beemonitor/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
