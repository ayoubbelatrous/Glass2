#include "Base/Base.h"
#include "Base/Log.h"
#include "Base/Types.h"

#include <iostream>
#include <string>
#include <string_view>

#include <vector>
#include <map>
#include <unordered_map>
#include <set>

#include <fstream>
#include <ostream>
#include <sstream>

#include <algorithm>
#include <filesystem>

#include <chrono>

#include <optional>

#include <spdlog/fmt/fmt.h>

#pragma warning(push, 0)
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/Triple.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Target/CGPassBuilderOption.h"
#include "llvm/Support/Host.h"
#include "llvm/IR/DIBuilder.h"
#pragma warning(pop)
