cmake_minimum_required(VERSION 3.8)
project(USBCanTools)

add_executable(
  ifclient
  src/ifclient.c
  src/log.c
  src/util.c
  include/defs.h
  include/ifshare.h
  include/log.h
  include/util.h)

target_include_directories(ifclient PUBLIC include)
# target_link_libraries(ifclient m)

add_executable(
  ifserver
  src/ifserver.c
  src/log.c
  src/util.c
  include/defs.h
  include/ifshare.h
  include/log.h
  include/util.h)

target_include_directories(ifserver PUBLIC include)

install(TARGETS ifclient ifserver DESTINATION bin)
