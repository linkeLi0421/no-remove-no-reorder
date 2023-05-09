INCLUDE(FindProtobuf)
# Create protobuf .h and .cc files, and put them in a library for use
find_package(Protobuf REQUIRED)
add_definitions(-DGOOGLE_PROTOBUF_NO_RTTI)

include_directories(${PROTOBUF_INCLUDE_DIRS})

PROTOBUF_GENERATE_CPP(IRPROTO_SRCS PROTO_HDRS IRInfo.proto MIRInfo.proto SecurityCheck.proto)
message("-- Generated Protobuf cpp files in: " ${CMAKE_CURRENT_BINARY_DIR})

# the name of a LLVM component library has the prefix of "LLVM"
add_llvm_component_library(LLVMMyProto
  ${IRPROTO_SRCS}
  ${PROTO_HDRS}
  )
target_link_libraries(LLVMMyProto PRIVATE ${PROTOBUF_LIBRARY})
