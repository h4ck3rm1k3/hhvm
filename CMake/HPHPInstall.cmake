cmake_minimum_required(VERSION 2.8)


FILE(GLOB binfiles "${CMAKE_CURRENT_SOURCE_DIR}/bin/*")
INSTALL(FILES ${binfiles} DESTINATION share/hphphome/bin/)

# hphi 
FILE(GLOB binfiles_hphpi "${CMAKE_CURRENT_SOURCE_DIR}/src/hphpi/hphpi*")
INSTALL(FILES ${binfiles_hphpi} DESTINATION share/hphphome/bin/)


# copy the cmake files
FILE(GLOB cmakefiles "${CMAKE_CURRENT_SOURCE_DIR}/CMake/*")
INSTALL(FILES ${binfiles} DESTINATION share/hphphome/CMake/)

# runtime base
SET(INCDIRS runtime/base runtime/base/array runtime/base/debug runtime/base/file runtime/base/memory runtime/base/server runtime/base/shared runtime/base/taint runtime/base/time runtime/base/util runtime/base/zend runtime/eval runtime/eval/analysis runtime/eval/ast runtime/eval/base runtime/eval/debugger runtime/eval/debugger/cmd runtime/eval/ext runtime/eval/parser runtime/eval/runtime runtime/ext runtime/ext/bcmath runtime/ext/hash runtime/ext/icu runtime/ext/mailparse runtime/ext/profile runtime/ext/sep/mhash runtime/ext/soap runtime/ext/thrift system system/gen/cls system/gen/php/classes system/gen/php/globals system/gen/sys system/lib util util/neo util/parser)

FOREACH (INCDIR ${INCDIRS})

  FILE(GLOB hphp_runtime "${CMAKE_CURRENT_SOURCE_DIR}/src/${INCDIR}/*.h")
  INSTALL(FILES ${hphp_runtime} DESTINATION include/hphp/${INCDIR})
  FILE(GLOB hphp_runtime "${CMAKE_CURRENT_SOURCE_DIR}/src/${INCDIR}/*.hpp")
  INSTALL(FILES ${hphp_runtime} DESTINATION include/hphp/${INCDIR})
  FILE(GLOB hphp_runtime "${CMAKE_CURRENT_SOURCE_DIR}/src/${INCDIR}/*.INC")
  INSTALL(FILES ${hphp_runtime} DESTINATION include/hphp/${INCDIR})

ENDFOREACH(INCDIR)


# needed for bin/run.sh
INSTALL(FILES LICENSE.PHP  DESTINATION share/hphphome/)
