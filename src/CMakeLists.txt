add_executable (gsm
  GSM.cpp
  CodeGen.cpp
  Lexer.cpp
  Parser.cpp
  Sema.cpp
  )
target_link_libraries(gsm PRIVATE ${llvm_libs})
set (CMAKE_CXX_FLAGS "-g3 -Wall -O0")

