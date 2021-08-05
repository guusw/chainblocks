set(chainblocks_SOURCES  
  stepA_mal.cpp
  Core.cpp
  CBCore.cpp
  Environment.cpp
  Reader.cpp
  ReadLine.cpp
  String.cpp
  Types.cpp
  Validation.cpp
  CBCore.cpp
)

add_executable(cbl ${chainblocks_SOURCES})
target_link_libraries(cbl
  chainblocks-core-static
  replxx
)