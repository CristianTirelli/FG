add_llvm_library( LLVMFG MODULE
  FG.cpp
  myGraph.cpp
	

  DEPENDS
  intrinsics_gen
  PLUGIN_TOOL
  opt
  )
