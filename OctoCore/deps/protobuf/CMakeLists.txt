# Copy of protobuf version f0640b5a046522d7ffbab02b87a8ce9a654f79d1
# List of files should be updated to match upstream's protobuf/cmake/libprotobuf-lite.cmake

set(PROTOBUF_INCLUDE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set(libprotobuf_lite_files
  google/protobuf/arena.cc
  google/protobuf/arenastring.cc
  google/protobuf/extension_set.cc
  google/protobuf/generated_message_util.cc
  google/protobuf/io/coded_stream.cc
  google/protobuf/io/zero_copy_stream.cc
  google/protobuf/io/zero_copy_stream_impl_lite.cc
  google/protobuf/message_lite.cc
  google/protobuf/repeated_field.cc
  google/protobuf/stubs/atomicops_internals_x86_gcc.cc
  google/protobuf/stubs/atomicops_internals_x86_msvc.cc
  google/protobuf/stubs/bytestream.cc
  google/protobuf/stubs/common.cc
  google/protobuf/stubs/once.cc
  google/protobuf/stubs/status.cc
  google/protobuf/stubs/statusor.cc
  google/protobuf/stubs/stringpiece.cc
  google/protobuf/stubs/stringprintf.cc
  google/protobuf/stubs/strutil.cc
  google/protobuf/stubs/time.cc
  google/protobuf/wire_format_lite.cc
)

add_library(libprotobuf-lite ${libprotobuf_lite_files})
target_compile_definitions(libprotobuf-lite PUBLIC LIBPROTOBUF_EXPORTS)
target_include_directories(libprotobuf-lite PUBLIC ${PROTOBUF_INCLUDE_DIRECTORY})
if (UNIX)
  target_compile_definitions(libprotobuf-lite PUBLIC HAVE_PTHREAD)
endif (UNIX)
