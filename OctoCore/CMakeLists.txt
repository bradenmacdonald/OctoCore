add_subdirectory(deps/protobuf)

set(OCTOCORE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/.. ${PROTOBUF_INCLUDE_DIRECTORY})
add_library(octocore
    messages/CommandData.pb.cc
    messages/CommandData.pb.h
    messages/GenericValue.pb.cc
    messages/GenericValue.pb.h

    Command.h
    DataTypes.h
    Exception.h
    FieldHash.h
    src/State.cpp
    State.h
)
target_link_libraries(octocore libprotobuf-lite)
target_include_directories(octocore PUBLIC ${OCTOCORE_INCLUDE_DIRECTORIES} . deps)
