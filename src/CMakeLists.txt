add_executable (gsm
  gsm.cpp
  CodeGen.cpp
  Lexer.cpp
  Parser.cpp
  Sema.cpp
  )
target_link_libraries(gsm PRIVATE ${llvm_libs})
