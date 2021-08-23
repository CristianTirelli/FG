add_llvm_library( LLVMFGNL MODULE
  FG.cpp
  myGraph.cpp
	

  DEPENDS
  intrinsics_gen
  PLUGIN_TOOL
  opt
  )
