/* SPDX-License-Identifier: BSD 3-Clause "New" or "Revised" License */
/* Copyright © 2019-2020 Giovanni Petrantoni */

#define STB_DS_IMPLEMENTATION 1

#include "runtime.hpp"
#include "blocks/shared.hpp"
#include <boost/stacktrace.hpp>
#include <csignal>
#include <cstdarg>
#include <string.h>
#include <unordered_set>

#ifdef USE_RPMALLOC
#include <rpmalloc/rpmalloc.h>

void *operator new(std::size_t s) {
  rpmalloc_initialize();
  return rpmalloc(s);
}
void *operator new[](std::size_t s) {
  rpmalloc_initialize();
  return rpmalloc(s);
}
void *operator new(std::size_t s, const std::nothrow_t &tag) noexcept {
  rpmalloc_initialize();
  return rpmalloc(s);
}
void *operator new[](std::size_t s, const std::nothrow_t &tag) noexcept {
  rpmalloc_initialize();
  return rpmalloc(s);
}
void *operator new(std::size_t size, std::align_val_t align) {
  rpmalloc_initialize();
  return rpaligned_alloc(static_cast<std::size_t>(align), size);
}

void operator delete(void *ptr) noexcept { rpfree(ptr); }
void operator delete[](void *ptr) noexcept { rpfree(ptr); }
void operator delete(void *ptr, const std::nothrow_t &tag) noexcept {
  rpfree(ptr);
}
void operator delete[](void *ptr, const std::nothrow_t &tag) noexcept {
  rpfree(ptr);
}
void operator delete(void *ptr, std::size_t sz) noexcept { rpfree(ptr); }
void operator delete[](void *ptr, std::size_t sz) noexcept { rpfree(ptr); }

void operator delete(void *ptr, std::size_t size,
                     std::align_val_t align) noexcept {
  rpfree(ptr);
}
void operator delete(void *ptr, std::align_val_t align) noexcept {
  rpfree(ptr);
}
#endif

INITIALIZE_EASYLOGGINGPP

namespace chainblocks {
extern void registerChainsBlocks();
extern void registerLoggingBlocks();
extern void registerFlowBlocks();
extern void registerSeqsBlocks();
extern void registerCastingBlocks();
extern void registerBlocksCoreBlocks();
extern void registerSerializationBlocks();
extern void registerFSBlocks();
extern void registerJsonBlocks();
extern void registerNetworkBlocks();
extern void registerStructBlocks();
extern void registerTimeBlocks();
// extern void registerOSBlocks();
extern void registerProcessBlocks();

namespace Math {
namespace LinAlg {
extern void registerBlocks();
}
} // namespace Math

namespace Regex {
extern void registerBlocks();
}

namespace channels {
extern void registerBlocks();
}

namespace Assert {
extern void registerBlocks();
}

#ifdef CB_WITH_EXTRAS
extern void cbInitExtras();
#endif

void registerCoreBlocks() {
  // at this point we might have some auto magical static linked block already
  // keep them stored here and re-register them
  // as we assume the observers were setup in this call caller so too late for
  // them
  std::vector<std::pair<std::string, CBBlockConstructor>> earlyblocks;
  for (auto &pair : Globals::BlocksRegister) {
    earlyblocks.push_back(pair);
  }
  Globals::BlocksRegister.clear();

#ifdef USE_RPMALLOC
  rpmalloc_initialize();
#endif

  static_assert(sizeof(CBVarPayload) == 16);
  static_assert(sizeof(CBVar) == 32);

  registerBlocksCoreBlocks();
  Assert::registerBlocks();
  registerChainsBlocks();
  registerLoggingBlocks();
  registerFlowBlocks();
  registerSeqsBlocks();
  registerCastingBlocks();
  registerSerializationBlocks();
  Math::LinAlg::registerBlocks();
  registerFSBlocks();
  registerJsonBlocks();
  registerNetworkBlocks();
  registerStructBlocks();
  registerTimeBlocks();
  // registerOSBlocks();
  Regex::registerBlocks();
  registerProcessBlocks();
  channels::registerBlocks();

  // Enums are auto registered we need to propagate them to observers
  for (auto &einfo : Globals::EnumTypesRegister) {
    int32_t vendorId = (int32_t)((einfo.first & 0xFFFFFFFF00000000) >> 32);
    int32_t enumId = (int32_t)(einfo.first & 0x00000000FFFFFFFF);
    for (auto &pobs : Globals::Observers) {
      if (pobs.expired())
        continue;
      auto obs = pobs.lock();
      obs->registerEnumType(vendorId, enumId, einfo.second);
    }
  }

#ifdef CB_WITH_EXTRAS
  cbInitExtras();
#endif

  // re run early blocks registration!
  for (auto &pair : earlyblocks) {
    registerBlock(pair.first.c_str(), pair.second);
  }

#ifndef NDEBUG
  // TODO remove when we have better tests/samples

  // Test chain DSL
  auto chain1 = std::unique_ptr<CBChain>(Chain("test-chain")
                                             .looped(true)
                                             .let(1)
                                             .block("Log")
                                             .block("Math.Add", 2)
                                             .block("Assert.Is", 3, true));
  assert(chain1->blocks.size() == 4);

  // Test dynamic array
  CBSeq ts{};
  Var a{0}, b{1}, c{2}, d{3}, e{4}, f{5};
  arrayPush(ts, a);
  assert(ts.len == 1);
  assert(ts.cap == 4);
  arrayPush(ts, b);
  arrayPush(ts, c);
  arrayPush(ts, d);
  arrayPush(ts, e);
  assert(ts.len == 5);
  assert(ts.cap == 8);

  assert(ts.elements[0] == Var(0));
  assert(ts.elements[1] == Var(1));
  assert(ts.elements[2] == Var(2));
  assert(ts.elements[3] == Var(3));
  assert(ts.elements[4] == Var(4));

  arrayInsert(ts, 1, f);

  assert(ts.elements[0] == Var(0));
  assert(ts.elements[1] == Var(5));
  assert(ts.elements[2] == Var(1));
  assert(ts.elements[3] == Var(2));
  assert(ts.elements[4] == Var(3));

  arrayDel(ts, 2);

  assert(ts.elements[0] == Var(0));
  assert(ts.elements[1] == Var(5));
  assert(ts.elements[2] == Var(2));
  assert(ts.elements[3] == Var(3));
  assert(ts.elements[4] == Var(4));

  arrayDelFast(ts, 2);

  assert(ts.elements[0] == Var(0));
  assert(ts.elements[1] == Var(5));
  assert(ts.elements[2] == Var(4));
  assert(ts.elements[3] == Var(3));

  assert(ts.len == 4);

  arrayFree(ts);
  assert(ts.elements == nullptr);
  assert(ts.len == 0);
  assert(ts.cap == 0);
#endif
}

CBlock *createBlock(const char *name) {
  auto it = Globals::BlocksRegister.find(name);
  if (it == Globals::BlocksRegister.end()) {
    return nullptr;
  }

  auto blkp = it->second();

  // Hook inline blocks to override activation in runChain
  if (strcmp(name, "Const") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::CoreConst;
  } else if (strcmp(name, "Stop") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::CoreStop;
  } else if (strcmp(name, "Input") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::CoreInput;
  } else if (strcmp(name, "Restart") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::CoreRestart;
  } else if (strcmp(name, "Sleep") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::CoreSleep;
  } else if (strcmp(name, "Repeat") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::CoreRepeat;
  } else if (strcmp(name, "Get") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::CoreGet;
  } else if (strcmp(name, "Set") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::CoreSet;
  } else if (strcmp(name, "Update") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::CoreUpdate;
  } else if (strcmp(name, "Swap") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::CoreSwap;
  } else if (strcmp(name, "Push") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::CorePush;
  } else if (strcmp(name, "Is") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::CoreIs;
  } else if (strcmp(name, "IsNot") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::CoreIsNot;
  } else if (strcmp(name, "IsMore") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::CoreIsMore;
  } else if (strcmp(name, "IsLess") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::CoreIsLess;
  } else if (strcmp(name, "IsMoreEqual") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::CoreIsMoreEqual;
  } else if (strcmp(name, "IsLessEqual") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::CoreIsLessEqual;
  } else if (strcmp(name, "And") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::CoreAnd;
  } else if (strcmp(name, "Or") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::CoreOr;
  } else if (strcmp(name, "Not") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::CoreNot;
  } else if (strcmp(name, "Math.Add") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathAdd;
  } else if (strcmp(name, "Math.Subtract") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathSubtract;
  } else if (strcmp(name, "Math.Multiply") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathMultiply;
  } else if (strcmp(name, "Math.Divide") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathDivide;
  } else if (strcmp(name, "Math.Xor") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathXor;
  } else if (strcmp(name, "Math.And") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathAnd;
  } else if (strcmp(name, "Math.Or") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathOr;
  } else if (strcmp(name, "Math.Mod") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathMod;
  } else if (strcmp(name, "Math.LShift") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathLShift;
  } else if (strcmp(name, "Math.RShift") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathRShift;
  } else if (strcmp(name, "Math.Abs") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathAbs;
  } else if (strcmp(name, "Math.Exp") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathExp;
  } else if (strcmp(name, "Math.Exp2") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathExp2;
  } else if (strcmp(name, "Math.Expm1") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathExpm1;
  } else if (strcmp(name, "Math.Log") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathLog;
  } else if (strcmp(name, "Math.Log10") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathLog10;
  } else if (strcmp(name, "Math.Log2") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathLog2;
  } else if (strcmp(name, "Math.Log1p") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathLog1p;
  } else if (strcmp(name, "Math.Sqrt") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathSqrt;
  } else if (strcmp(name, "Math.Cbrt") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathCbrt;
  } else if (strcmp(name, "Math.Sin") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathSin;
  } else if (strcmp(name, "Math.Cos") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathCos;
  } else if (strcmp(name, "Math.Tan") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathTan;
  } else if (strcmp(name, "Math.Asin") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathAsin;
  } else if (strcmp(name, "Math.Acos") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathAcos;
  } else if (strcmp(name, "Math.Atan") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathAtan;
  } else if (strcmp(name, "Math.Sinh") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathSinh;
  } else if (strcmp(name, "Math.Cosh") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathCosh;
  } else if (strcmp(name, "Math.Tanh") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathTanh;
  } else if (strcmp(name, "Math.Asinh") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathAsinh;
  } else if (strcmp(name, "Math.Acosh") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathAcosh;
  } else if (strcmp(name, "Math.Atanh") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathAtanh;
  } else if (strcmp(name, "Math.Erf") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathErf;
  } else if (strcmp(name, "Math.Erfc") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathErfc;
  } else if (strcmp(name, "Math.TGamma") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathTGamma;
  } else if (strcmp(name, "Math.LGamma") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathLGamma;
  } else if (strcmp(name, "Math.Ceil") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathCeil;
  } else if (strcmp(name, "Math.Floor") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathFloor;
  } else if (strcmp(name, "Math.Trunc") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathTrunc;
  } else if (strcmp(name, "Math.Round") == 0) {
    blkp->inlineBlockId = CBInlineBlocks::MathRound;
  }

  return blkp;
}

void registerBlock(const char *fullName, CBBlockConstructor constructor) {
  auto cname = std::string(fullName);
  auto findIt = Globals::BlocksRegister.find(cname);
  if (findIt == Globals::BlocksRegister.end()) {
    Globals::BlocksRegister.insert(std::make_pair(cname, constructor));
    // DLOG(INFO) << "added block: " << cname;
  } else {
    Globals::BlocksRegister[cname] = constructor;
    LOG(INFO) << "overridden block: " << cname;
  }

  for (auto &pobs : Globals::Observers) {
    if (pobs.expired())
      continue;
    auto obs = pobs.lock();
    obs->registerBlock(fullName, constructor);
  }
}

void registerObjectType(int32_t vendorId, int32_t typeId, CBObjectInfo info) {
  int64_t id = (int64_t)vendorId << 32 | typeId;
  auto typeName = std::string(info.name);
  auto findIt = Globals::ObjectTypesRegister.find(id);
  if (findIt == Globals::ObjectTypesRegister.end()) {
    Globals::ObjectTypesRegister.insert(std::make_pair(id, info));
    // DLOG(INFO) << "added object type: " << typeName;
  } else {
    Globals::ObjectTypesRegister[id] = info;
    LOG(INFO) << "overridden object type: " << typeName;
  }

  for (auto &pobs : Globals::Observers) {
    if (pobs.expired())
      continue;
    auto obs = pobs.lock();
    obs->registerObjectType(vendorId, typeId, info);
  }
}

void registerEnumType(int32_t vendorId, int32_t typeId, CBEnumInfo info) {
  int64_t id = (int64_t)vendorId << 32 | typeId;
  auto typeName = std::string(info.name);
  auto findIt = Globals::ObjectTypesRegister.find(id);
  if (findIt == Globals::ObjectTypesRegister.end()) {
    Globals::EnumTypesRegister.insert(std::make_pair(id, info));
    // DLOG(INFO) << "added enum type: " << typeName;
  } else {
    Globals::EnumTypesRegister[id] = info;
    LOG(INFO) << "overridden enum type: " << typeName;
  }

  for (auto &pobs : Globals::Observers) {
    if (pobs.expired())
      continue;
    auto obs = pobs.lock();
    obs->registerEnumType(vendorId, typeId, info);
  }
}

void registerRunLoopCallback(const char *eventName, CBCallback callback) {
  chainblocks::Globals::RunLoopHooks[eventName] = callback;
}

void unregisterRunLoopCallback(const char *eventName) {
  auto findIt = chainblocks::Globals::RunLoopHooks.find(eventName);
  if (findIt != chainblocks::Globals::RunLoopHooks.end()) {
    chainblocks::Globals::RunLoopHooks.erase(findIt);
  }
}

void registerExitCallback(const char *eventName, CBCallback callback) {
  chainblocks::Globals::ExitHooks[eventName] = callback;
}

void unregisterExitCallback(const char *eventName) {
  auto findIt = chainblocks::Globals::ExitHooks.find(eventName);
  if (findIt != chainblocks::Globals::ExitHooks.end()) {
    chainblocks::Globals::ExitHooks.erase(findIt);
  }
}

void registerChain(CBChain *chain) {
  std::shared_ptr<CBChain> sc(chain);
  chainblocks::Globals::GlobalChains[chain->name] = sc;
}

void unregisterChain(CBChain *chain) {
  auto findIt = chainblocks::Globals::GlobalChains.find(chain->name);
  if (findIt != chainblocks::Globals::GlobalChains.end()) {
    chainblocks::Globals::GlobalChains.erase(findIt);
  }
}

void callExitCallbacks() {
  // Iterate backwards
  for (auto it = chainblocks::Globals::ExitHooks.begin();
       it != chainblocks::Globals::ExitHooks.end(); ++it) {
    it->second();
  }
}

CBVar *referenceGlobalVariable(CBContext *ctx, const char *name) {
  assert(ctx->main->node);
  CBVar &v = ctx->main->node->variables[name];
  v.refcount++;
  v.flags |= CBVAR_FLAGS_REF_COUNTED;
  return &v;
}

CBVar *referenceVariable(CBContext *ctx, const char *name) {
  assert(ctx->main->node);

  // try find a chain variable
  // from top to bottom of chain stack
  auto rit = ctx->chainStack.rbegin();
  for (; rit != ctx->chainStack.rend(); ++rit) {
    auto it = (*rit)->variables.find(name);
    if (it != (*rit)->variables.end()) {
      // found, lets get out here
      CBVar &cv = it->second;
      cv.refcount++;
      return &cv;
    }
  }

  // Was not in chains.. find in global node,
  // if fails create on top chain
  auto it = ctx->main->node->variables.find(name);
  if (it != ctx->main->node->variables.end()) {
    // found, lets get out here
    CBVar &cv = it->second;
    cv.refcount++;
    return &cv;
  }

  // worst case create in current top chain!
  CBVar &cv = ctx->chainStack.back()->variables[name];
  cv.refcount++;
  cv.flags |= CBVAR_FLAGS_REF_COUNTED;
  return &cv;
}

void releaseVariable(CBVar *variable) {
  assert((variable->flags & CBVAR_FLAGS_REF_COUNTED) ==
         CBVAR_FLAGS_REF_COUNTED);
  assert(variable->refcount > 0);
  variable->refcount--;
  if (variable->refcount == 0) {
    destroyVar(*variable);
  }
}

CBVar suspend(CBContext *context, double seconds) {
  if (seconds <= 0) {
    context->next = Duration(0);
  } else {
    context->next = Clock::now().time_since_epoch() + Duration(seconds);
  }
  context->continuation = context->continuation.resume();
  if (context->restarted) {
    CBVar restart = {};
    restart.valueType = None;
    restart.payload.chainState = CBChainState::Restart;
    return restart;
  } else if (context->aborted) {
    CBVar stop = {};
    stop.valueType = None;
    stop.payload.chainState = CBChainState::Stop;
    return stop;
  }
  CBVar cont = {};
  cont.valueType = None;
  cont.payload.chainState = Continue;
  return cont;
}

FlowState activateBlocks(CBlocks blocks, CBContext *context,
                         const CBVar &chainInput, CBVar &output) {
  auto input = chainInput;
  // validation prevents extra pops so this should be safe
  auto sidx = context->stack.len;
  for (uint32_t i = 0; i < blocks.len; i++) {
    output = activateBlock(blocks.elements[i], context, input);
    if (output.valueType == None) {
      switch (output.payload.chainState) {
      case CBChainState::Restart: {
        context->stack.len = sidx;
        return Continuing;
      }
      case CBChainState::Stop: {
        context->stack.len = sidx;
        return Stopping;
      }
      case CBChainState::Return: {
        output = input; // Invert them, we return previous output (input)
        context->stack.len = sidx;
        return Returning;
      }
      case CBChainState::Rebase: {
        input = chainInput;
        continue;
      }
      case Continue:
        break;
      }
    }
    input = output;
  }
  context->stack.len = sidx;
  return Continuing;
}

FlowState activateBlocks(CBSeq blocks, CBContext *context,
                         const CBVar &chainInput, CBVar &output) {
  auto input = chainInput;
  // validation prevents extra pops so this should be safe
  auto sidx = context->stack.len;
  for (uint32_t i = 0; i < blocks.len; i++) {
    output =
        activateBlock(blocks.elements[i].payload.blockValue, context, input);
    if (output.valueType == None) {
      switch (output.payload.chainState) {
      case CBChainState::Restart: {
        context->stack.len = sidx;
        return Continuing;
      }
      case CBChainState::Stop: {
        context->stack.len = sidx;
        return Stopping;
      }
      case CBChainState::Return: {
        output = input; // Invert them, we return previous output (input)
        context->stack.len = sidx;
        return Returning;
      }
      case CBChainState::Rebase: {
        input = chainInput;
        continue;
      }
      case Continue:
        break;
      }
    }
    input = output;
  }
  context->stack.len = sidx;
  return Continuing;
}

CBSeq *InternalCore::getStack(CBContext *context) { return &context->stack; }
}; // namespace chainblocks

#ifndef OVERRIDE_REGISTER_ALL_BLOCKS
void cbRegisterAllBlocks() { chainblocks::registerCoreBlocks(); }
#endif

extern "C" {
EXPORTED struct CBCore __cdecl chainblocksInterface(uint32_t abi_version) {
  CBCore result{};

  if (CHAINBLOCKS_CURRENT_ABI != abi_version) {
    LOG(ERROR) << "A plugin requested an invalid ABI version.";
    return result;
  }

  result.registerBlock = [](const char *fullName,
                            CBBlockConstructor constructor) {
    chainblocks::registerBlock(fullName, constructor);
  };

  result.registerObjectType = [](int32_t vendorId, int32_t typeId,
                                 CBObjectInfo info) {
    chainblocks::registerObjectType(vendorId, typeId, info);
  };

  result.registerEnumType = [](int32_t vendorId, int32_t typeId,
                               CBEnumInfo info) {
    chainblocks::registerEnumType(vendorId, typeId, info);
  };

  result.registerRunLoopCallback = [](const char *eventName,
                                      CBCallback callback) {
    chainblocks::registerRunLoopCallback(eventName, callback);
  };

  result.registerExitCallback = [](const char *eventName, CBCallback callback) {
    chainblocks::registerExitCallback(eventName, callback);
  };

  result.unregisterRunLoopCallback = [](const char *eventName) {
    chainblocks::unregisterRunLoopCallback(eventName);
  };

  result.unregisterExitCallback = [](const char *eventName) {
    chainblocks::unregisterExitCallback(eventName);
  };

  result.referenceVariable = [](CBContext *context, const char *name) {
    return chainblocks::referenceVariable(context, name);
  };

  result.releaseVariable = [](CBVar *variable) {
    return chainblocks::releaseVariable(variable);
  };

  result.getStack = [](CBContext *context) { return &context->stack; };

  result.throwException = [](const char *errorText) {
    throw chainblocks::CBException(errorText);
  };

  result.suspend = [](CBContext *context, double seconds) {
    return chainblocks::suspend(context, seconds);
  };

  result.cloneVar = [](CBVar *dst, const CBVar *src) {
    chainblocks::cloneVar(*dst, *src);
  };

  result.destroyVar = [](CBVar *var) { chainblocks::destroyVar(*var); };

#define CB_ARRAY_IMPL(_arr_, _val_, _name_)                                    \
  result._name_##Free = [](_arr_ *seq) { chainblocks::arrayFree(*seq); };      \
                                                                               \
  result._name_##Resize = [](_arr_ *seq, uint64_t size) {                      \
    chainblocks::arrayResize(*seq, size);                                      \
  };                                                                           \
                                                                               \
  result._name_##Push = [](_arr_ *seq, const _val_ *value) {                   \
    chainblocks::arrayPush(*seq, *value);                                      \
  };                                                                           \
                                                                               \
  result._name_##Insert = [](_arr_ *seq, uint64_t index, const _val_ *value) { \
    chainblocks::arrayInsert(*seq, index, *value);                             \
  };                                                                           \
                                                                               \
  result._name_##Pop = [](_arr_ *seq) {                                        \
    return chainblocks::arrayPop<_arr_, _val_>(*seq);                          \
  };                                                                           \
                                                                               \
  result._name_##FastDelete = [](_arr_ *seq, uint64_t index) {                 \
    chainblocks::arrayDelFast(*seq, index);                                    \
  };                                                                           \
                                                                               \
  result._name_##SlowDelete = [](_arr_ *seq, uint64_t index) {                 \
    chainblocks::arrayDel(*seq, index);                                        \
  }

  CB_ARRAY_IMPL(CBSeq, CBVar, seq);
  CB_ARRAY_IMPL(CBTypesInfo, CBTypeInfo, types);
  CB_ARRAY_IMPL(CBParametersInfo, CBParameterInfo, params);
  CB_ARRAY_IMPL(CBlocks, CBlockPtr, blocks);
  CB_ARRAY_IMPL(CBExposedTypesInfo, CBExposedTypeInfo, expTypes);
  CB_ARRAY_IMPL(CBStrings, CBString, strings);

  result.tableNew = []() {
    CBTable res;
    res.api = &chainblocks::Globals::TableInterface;
    res.opaque = new chainblocks::CBMap();
    return res;
  };

  result.validateChain = [](CBChain *chain, CBValidationCallback callback,
                            void *userData, CBInstanceData data) {
    return validateConnections(chain, callback, userData, data);
  };

  result.runChain = [](CBChain *chain, CBContext *context, CBVar input) {
    return chainblocks::runSubChain(chain, context, input);
  };

  result.validateBlocks = [](CBlocks blocks, CBValidationCallback callback,
                             void *userData, CBInstanceData data) {
    return validateConnections(blocks, callback, userData, data);
  };

  result.runBlocks = [](CBlocks blocks, CBContext *context, CBVar input) {
    CBVar output{};
    chainblocks::activateBlocks(blocks, context, input, output);
    return output;
  };

  result.getChainInfo = [](CBChainRef chainref) {
    auto sc = CBChain::sharedFromRef(chainref);
    auto chain = sc.get();
    CBChainInfo info{chain->name.c_str(),
                     chain->looped,
                     chain->unsafe,
                     chain,
                     {&chain->blocks[0], uint32_t(chain->blocks.size()), 0}};
    return info;
  };

  result.log = [](const char *msg) { LOG(INFO) << msg; };

  result.createBlock = [](const char *name) {
    return chainblocks::createBlock(name);
  };

  result.createChain = [](const char *name, CBlocks blocks, bool looped,
                          bool unsafe) {
    auto chain = new CBChain(name);
    chain->looped = looped;
    chain->unsafe = unsafe;
    for (uint32_t i = 0; i < blocks.len; i++) {
      chain->addBlock(blocks.elements[i]);
    }
    return chain;
  };

  result.destroyChain = [](CBChain *chain) { delete chain; };

  result.createNode = []() { return new CBNode(); };

  result.destroyNode = [](CBNode *node) { delete node; };

  result.schedule = [](CBNode *node, CBChain *chain) { node->schedule(chain); };

  result.tick = [](CBNode *node) { node->tick(); };

  result.sleep = [](double seconds, bool runCallbacks) {
    chainblocks::sleep(seconds, runCallbacks);
  };

  result.getRootPath = []() { return chainblocks::Globals::RootPath.c_str(); };

  result.setRootPath = [](const char *p) {
    chainblocks::Globals::RootPath = p;
  };

  return result;
}
};

bool matchTypes(const CBTypeInfo &inputType, const CBTypeInfo &receiverType,
                bool isParameter, bool strict) {
  if (receiverType.basicType == Any ||
      // receiver is a none type input block basically
      (!isParameter && receiverType.basicType == None))
    return true;

  if (inputType.basicType != receiverType.basicType) {
    // Fail if basic type differs
    return false;
  }

  switch (inputType.basicType) {
  case Object: {
    if (inputType.object.vendorId != receiverType.object.vendorId ||
        inputType.object.typeId != receiverType.object.typeId) {
      return false;
    }
    break;
  }
  case Enum: {
    if (inputType.enumeration.vendorId != receiverType.enumeration.vendorId ||
        inputType.enumeration.typeId != receiverType.enumeration.typeId) {
      return false;
    }
    break;
  }
  case Seq: {
    if (strict) {
      if (inputType.seqTypes.len > 0 && receiverType.seqTypes.len > 0) {
        // all input types must be in receiver, receiver can have more ofc
        for (uint32_t i = 0; i < inputType.seqTypes.len; i++) {
          for (uint32_t j = 0; i < receiverType.seqTypes.len; j++) {
            if (receiverType.seqTypes.elements[j].basicType == Any ||
                inputType.seqTypes.elements[i] ==
                    receiverType.seqTypes.elements[j])
              goto matched;
          }
          return false;
        matched:
          continue;
        }
      } else if (inputType.seqTypes.len == 0 ||
                 receiverType.seqTypes.len == 0) {
        return false;
      }
    }
    break;
  }
  case Table: {
    if (strict) {
      auto atypes = inputType.table.types.len;
      auto btypes = receiverType.table.types.len;
      //  btypes != 0 assume consumer is not strict
      for (uint32_t i = 0; i < atypes && (isParameter || btypes != 0); i++) {
        // Go thru all exposed types and make sure we get a positive match with
        // the consumer
        auto atype = inputType.table.types.elements[i];
        auto matched = false;
        for (uint32_t y = 0; y < btypes; y++) {
          auto btype = receiverType.table.types.elements[y];
          if (matchTypes(atype, btype, isParameter, strict)) {
            matched = true;
            break;
          }
        }
        if (!matched) {
          return false;
        }
      }
    }
    break;
  }
  default:
    return true;
  }
  return true;
}

struct ValidationContext {
  std::unordered_map<std::string, std::unordered_set<CBExposedTypeInfo>>
      exposed;
  std::unordered_set<std::string> variables;
  std::unordered_set<std::string> references;

  CBTypeInfo previousOutputType{};
  CBTypeInfo originalInputType{};
  std::vector<CBTypeInfo> stackTypes;

  CBlock *bottom{};

  CBValidationCallback cb{};
  void *userData{};
};

void validateConnection(ValidationContext &ctx) {
  auto previousOutput = ctx.previousOutputType;

  auto inputInfos = ctx.bottom->inputTypes(ctx.bottom);
  auto inputMatches = false;
  // validate our generic input
  for (uint32_t i = 0; inputInfos.len > i; i++) {
    auto &inputInfo = inputInfos.elements[i];
    if (matchTypes(previousOutput, inputInfo, false, false)) {
      inputMatches = true;
      break;
    }
  }

  if (!inputMatches) {
    std::string err("Could not find a matching input type, block: " +
                    std::string(ctx.bottom->name(ctx.bottom)));
    ctx.cb(ctx.bottom, err.c_str(), false, ctx.userData);
  }

  // infer and specialize types if we need to
  // If we don't we assume our output will be of the same type of the previous!
  if (ctx.bottom->compose) {
    CBInstanceData data{};
    data.block = ctx.bottom;
    data.inputType = previousOutput;
    // Pass all we got in the context!
    for (auto &info : ctx.exposed) {
      for (auto &type : info.second) {
        chainblocks::arrayPush(data.shared, type);
      }
    }
    for (auto &info : ctx.stackTypes) {
      chainblocks::arrayPush(data.stack, info);
    }

    // this ensures e.g. SetVariable exposedVars have right type from the actual
    // input type (previousOutput)!
    ctx.previousOutputType = ctx.bottom->compose(ctx.bottom, data);

    chainblocks::arrayFree(data.stack);
    chainblocks::arrayFree(data.shared);
  } else {
    // Short-cut if it's just one type and not any type
    // Any type tho means keep previous output type!
    auto outputTypes = ctx.bottom->outputTypes(ctx.bottom);
    if (outputTypes.len == 1 && outputTypes.elements[0].basicType != Any) {
      ctx.previousOutputType = outputTypes.elements[0];
    }
  }

  // Grab those after type inference!
  auto exposedVars = ctx.bottom->exposedVariables(ctx.bottom);
  // Add the vars we expose
  for (uint32_t i = 0; exposedVars.len > i; i++) {
    auto &exposed_param = exposedVars.elements[i];
    std::string name(exposed_param.name);
    ctx.exposed[name].insert(exposed_param);

    // Reference mutability checks
    if (strcmp(ctx.bottom->name(ctx.bottom), "Ref") == 0) {
      // make sure we are not Ref-ing a Set
      // meaning target would be overwritten, yet Set will try to deallocate
      // it/manage it
      if (ctx.variables.count(name)) {
        // Error
        std::string err(
            "Ref variable name already used as Set. Overwriting a previously "
            "Set variable with Ref is not allowed, name: " +
            name);
        ctx.cb(ctx.bottom, err.c_str(), false, ctx.userData);
      }
      ctx.references.insert(name);
    } else if (strcmp(ctx.bottom->name(ctx.bottom), "Set") == 0) {
      // make sure we are not Set-ing a Ref
      // meaning target memory could be any block temporary buffer, yet Set will
      // try to deallocate it/manage it
      if (ctx.references.count(name)) {
        // Error
        std::string err(
            "Set variable name already used as Ref. Overwriting a previously "
            "Ref variable with Set is not allowed, name: " +
            name);
        ctx.cb(ctx.bottom, err.c_str(), false, ctx.userData);
      }
      ctx.variables.insert(name);
    } else if (strcmp(ctx.bottom->name(ctx.bottom), "Update") == 0) {
      // make sure we are not Set-ing a Ref
      // meaning target memory could be any block temporary buffer, yet Set will
      // try to deallocate it/manage it
      if (ctx.references.count(name)) {
        // Error
        std::string err("Update variable name already used as Ref. Overwriting "
                        "a previously "
                        "Ref variable with Update is not allowed, name: " +
                        name);
        ctx.cb(ctx.bottom, err.c_str(), false, ctx.userData);
      }
    } else if (strcmp(ctx.bottom->name(ctx.bottom), "Push") == 0) {
      // make sure we are not Push-ing a Ref
      // meaning target memory could be any block temporary buffer, yet Push
      // will try to deallocate it/manage it
      if (ctx.references.count(name)) {
        // Error
        std::string err(
            "Push variable name already used as Ref. Overwriting a previously "
            "Ref variable with Push is not allowed, name: " +
            name);
        ctx.cb(ctx.bottom, err.c_str(), false, ctx.userData);
      }
      ctx.variables.insert(name);
    }
  }

  // Take selector checks
  if (strcmp(ctx.bottom->name(ctx.bottom), "Take") == 0) {
    if (previousOutput.basicType == Seq) {
      ctx.bottom->inlineBlockId = CBInlineBlocks::CoreTakeSeq;
    } else if (previousOutput.basicType >= Int2 &&
               previousOutput.basicType <= Int16) {
      ctx.bottom->inlineBlockId = CBInlineBlocks::CoreTakeInts;
    } else if (previousOutput.basicType >= Float2 &&
               previousOutput.basicType <= Float4) {
      ctx.bottom->inlineBlockId = CBInlineBlocks::CoreTakeFloats;
    } else if (previousOutput.basicType == Color) {
      ctx.bottom->inlineBlockId = CBInlineBlocks::CoreTakeColor;
    } else if (previousOutput.basicType == Bytes) {
      ctx.bottom->inlineBlockId = CBInlineBlocks::CoreTakeBytes;
    } else if (previousOutput.basicType == String) {
      ctx.bottom->inlineBlockId = CBInlineBlocks::CoreTakeString;
    }
  }

  // Finally do checks on what we consume
  auto requiredVar = ctx.bottom->requiredVariables(ctx.bottom);

  std::unordered_map<std::string, std::vector<CBExposedTypeInfo>> requiredVars;
  for (uint32_t i = 0; requiredVar.len > i; i++) {
    auto &required_param = requiredVar.elements[i];
    std::string name(required_param.name);
    requiredVars[name].push_back(required_param);
  }

  // make sure we have the vars we need, collect first
  for (const auto &required : requiredVars) {
    auto matching = false;

    for (const auto &required_param : required.second) {
      std::string name(required_param.name);
      if (name.find(' ') !=
          std::string::npos) { // take only the first part of variable name
        // the remaining should be a table key which we don't care here
        name = name.substr(0, name.find(' '));
      }
      auto findIt = ctx.exposed.find(name);
      if (findIt == ctx.exposed.end()) {
        std::string err("Required required variable not found: " + name);
        // Warning only, delegate compose to decide
        ctx.cb(ctx.bottom, err.c_str(), true, ctx.userData);
      } else {

        for (auto type : findIt->second) {
          auto exposedType = type.exposedType;
          auto requiredType = required_param.exposedType;
          // Finally deep compare types
          if (matchTypes(exposedType, requiredType, false, true)) {
            matching = true;
            break;
          }
        }
      }
      if (matching)
        break;
    }

    if (!matching) {
      std::string err(
          "Required required types do not match currently exposed ones: " +
          required.first);
      err += " exposed types:";
      for (const auto &info : ctx.exposed) {
        err += " (" + info.first + " [";

        for (auto type : info.second) {
          if (type.exposedType.basicType == Table &&
              type.exposedType.table.types.elements &&
              type.exposedType.table.keys.elements) {
            err +=
                "(" + type2Name(type.exposedType.basicType) + " [" +
                type2Name(type.exposedType.table.types.elements[0].basicType) +
                " " + type.exposedType.table.keys.elements[0] + "]) ";
          } else if (type.exposedType.basicType == Seq) {
            err += "(" + type2Name(type.exposedType.basicType) + " [";
            for (uint32_t i = 0; i < type.exposedType.seqTypes.len; i++) {
              if (i < type.exposedType.seqTypes.len - 1)
                err +=
                    type2Name(type.exposedType.seqTypes.elements[i].basicType) +
                    " ";
              else
                err +=
                    type2Name(type.exposedType.seqTypes.elements[i].basicType) +
                    " ";
            }
            err += "]) ";
          } else {
            err += type2Name(type.exposedType.basicType) + " ";
          }
        }
        err.erase(err.end() - 1);

        err += "])";
      }
      ctx.cb(ctx.bottom, err.c_str(), false, ctx.userData);
    }
  }
}

CBValidationResult validateConnections(const std::vector<CBlock *> &chain,
                                       CBValidationCallback callback,
                                       void *userData, CBInstanceData data,
                                       bool globalsOnly) {
  auto ctx = ValidationContext();
  ctx.originalInputType = data.inputType;
  ctx.previousOutputType = data.inputType;
  ctx.cb = callback;
  ctx.userData = userData;
  for (uint32_t i = 0; i < data.stack.len; i++) {
    ctx.stackTypes.push_back(data.stack.elements[i]);
  }

  if (data.shared.elements) {
    for (uint32_t i = 0; i < data.shared.len; i++) {
      auto &info = data.shared.elements[i];
      ctx.exposed[info.name].insert(info);
    }
  }

  for (auto blk : chain) {
    if (strcmp(blk->name(blk), "Input") == 0 ||
        strcmp(blk->name(blk), "And") == 0 ||
        strcmp(blk->name(blk), "Or") == 0) {
      // Hard code behavior for Input block and And and Or, in order to validate
      // with actual chain input the followup
      ctx.previousOutputType = ctx.originalInputType;
    } else if (strcmp(blk->name(blk), "SetInput") == 0) {
      if (ctx.previousOutputType != ctx.originalInputType) {
        throw chainblocks::CBException(
            "SetInput input type must be the same original chain input type.");
      }
    } else if (strcmp(blk->name(blk), "Push") == 0) {
      // Check first param see if empty/null
      auto seqName = blk->getParam(blk, 0);
      if (seqName.payload.stringValue == nullptr ||
          seqName.payload.stringValue[0] == 0) {
        blk->inlineBlockId = StackPush;
        // keep previous output type
        // push stack type
        ctx.stackTypes.push_back(ctx.previousOutputType);
      } else {
        ctx.bottom = blk;
        validateConnection(ctx);
      }
    } else if (strcmp(blk->name(blk), "Pop") == 0) {
      // Check first param see if empty/null
      auto seqName = blk->getParam(blk, 0);
      if (seqName.payload.stringValue == nullptr ||
          seqName.payload.stringValue[0] == 0) {
        blk->inlineBlockId = StackPop;
        if (ctx.stackTypes.empty()) {
          throw chainblocks::CBException("Stack Pop, but stack was empty!");
        }
        ctx.previousOutputType = ctx.stackTypes.back();
        ctx.stackTypes.pop_back();
      } else {
        ctx.bottom = blk;
        validateConnection(ctx);
      }
    } else if (strcmp(blk->name(blk), "Drop") == 0) {
      // Check first param see if empty/null
      auto seqName = blk->getParam(blk, 0);
      if (seqName.payload.stringValue == nullptr ||
          seqName.payload.stringValue[0] == 0) {
        blk->inlineBlockId = StackDrop;
        if (ctx.stackTypes.empty()) {
          throw chainblocks::CBException("Stack Drop, but stack was empty!");
        }
        ctx.stackTypes.pop_back();
      } else {
        ctx.bottom = blk;
        validateConnection(ctx);
      }
    } else if (strcmp(blk->name(blk), "Swap") == 0) {
      // Check first param see if empty/null
      auto aname = blk->getParam(blk, 0);
      auto bname = blk->getParam(blk, 1);
      if ((aname.payload.stringValue == nullptr ||
           aname.payload.stringValue[0] == 0) &&
          (bname.payload.stringValue == nullptr ||
           bname.payload.stringValue[0] == 0)) {
        blk->inlineBlockId = StackSwap;
        if (ctx.stackTypes.size() < 2) {
          throw chainblocks::CBException("Stack Swap, but stack size < 2!");
        }
        std::swap(ctx.stackTypes[ctx.stackTypes.size() - 1],
                  ctx.stackTypes[ctx.stackTypes.size() - 2]);
        ctx.previousOutputType = ctx.stackTypes.back();
      } else {
        ctx.bottom = blk;
        validateConnection(ctx);
      }
    } else {
      ctx.bottom = blk;
      validateConnection(ctx);
    }
  }

  CBValidationResult result = {ctx.previousOutputType};
  for (auto &exposed : ctx.exposed) {
    for (auto &type : exposed.second) {
      if (!globalsOnly || type.global)
        chainblocks::arrayPush(result.exposedInfo, type);
    }
  }
  return result;
}

CBValidationResult validateConnections(const CBChain *chain,
                                       CBValidationCallback callback,
                                       void *userData, CBInstanceData data) {
  return validateConnections(chain->blocks, callback, userData, data, true);
}

CBValidationResult validateConnections(const CBlocks chain,
                                       CBValidationCallback callback,
                                       void *userData, CBInstanceData data) {
  std::vector<CBlock *> blocks;
  for (uint32_t i = 0; chain.len > i; i++) {
    blocks.push_back(chain.elements[i]);
  }
  return validateConnections(blocks, callback, userData, data, false);
}

void freeDerivedInfo(CBTypeInfo info) {
  switch (info.basicType) {
  case Seq: {
    for (uint32_t i = 0; info.seqTypes.len > i; i++) {
      freeDerivedInfo(info.seqTypes.elements[i]);
    }
    chainblocks::arrayFree(info.seqTypes);
  }
  case Table: {
    for (uint32_t i = 0; info.table.types.len > i; i++) {
      freeDerivedInfo(info.table.types.elements[i]);
    }
    chainblocks::arrayFree(info.table.types);
  }
  default:
    break;
  };
}

CBTypeInfo deriveTypeInfo(CBVar &value) {
  // We need to guess a valid CBTypeInfo for this var in order to validate
  // Build a CBTypeInfo for the var
  auto varType = CBTypeInfo();
  varType.basicType = value.valueType;
  varType.seqTypes = {};
  varType.table.types = {};
  switch (value.valueType) {
  case Object: {
    varType.object.vendorId = value.payload.objectVendorId;
    varType.object.typeId = value.payload.objectTypeId;
    break;
  }
  case Enum: {
    varType.enumeration.vendorId = value.payload.enumVendorId;
    varType.enumeration.typeId = value.payload.enumTypeId;
    break;
  }
  case Seq: {
    std::unordered_set<CBTypeInfo> types;
    for (uint32_t i = 0; i < value.payload.seqValue.len; i++) {
      auto derived = deriveTypeInfo(value.payload.seqValue.elements[i]);
      if (!types.count(derived)) {
        chainblocks::arrayPush(varType.seqTypes, derived);
        types.insert(derived);
      }
    }
    break;
  }
  case Table: {
    std::unordered_set<CBTypeInfo> types;
    auto &ta = value.payload.tableValue;
    struct iterdata {
      std::unordered_set<CBTypeInfo> *types;
      CBTypeInfo *varType;
    } data;
    ta.api->tableForEach(
        ta,
        [](const char *key, CBVar *value, void *_data) {
          auto data = (iterdata *)_data;
          auto derived = deriveTypeInfo(*value);
          if (!data->types->count(derived)) {
            chainblocks::arrayPush(data->varType->table.types, derived);
            data->types->insert(derived);
          }
          return true;
        },
        &data);

    break;
  }
  default:
    break;
  };
  return varType;
}

bool validateSetParam(CBlock *block, int index, CBVar &value,
                      CBValidationCallback callback, void *userData) {
  auto params = block->parameters(block);
  if (params.len <= (uint32_t)index) {
    std::string err("Parameter index out of range");
    callback(block, err.c_str(), false, userData);
    return false;
  }

  auto param = params.elements[index];

  // Build a CBTypeInfo for the var
  auto varType = deriveTypeInfo(value);

  for (uint32_t i = 0; param.valueTypes.len > i; i++) {
    if (matchTypes(varType, param.valueTypes.elements[i], true, true)) {
      freeDerivedInfo(varType);
      return true; // we are good just exit
    }
  }

  // Failed until now but let's check if the type is a sequenced too
  if (value.valueType == Seq) {
    // Validate each type in the seq
    for (uint32_t i = 0; value.payload.seqValue.len > i; i++) {
      if (validateSetParam(block, index, value.payload.seqValue.elements[i],
                           callback, userData)) {
        freeDerivedInfo(varType);
        return true;
      }
    }
  }

  std::string err("Parameter not accepting this kind of variable");
  callback(block, err.c_str(), false, userData);

  freeDerivedInfo(varType);

  return false;
}

void CBChain::cleanup() {
  if (node) {
    node->remove(this);
    node = nullptr;
  }

  for (auto it = blocks.rbegin(); it != blocks.rend(); ++it) {
    (*it)->cleanup(*it);
    (*it)->destroy(*it);
    // blk is responsible to free itself, as they might use any allocation
    // strategy they wish!
  }
  blocks.clear();

  if (ownedOutput) {
    chainblocks::destroyVar(finishedOutput);
    ownedOutput = false;
  }
}

namespace chainblocks {
void error_handler(int err_sig) {
  std::signal(err_sig, SIG_DFL);

  auto printTrace = false;

  switch (err_sig) {
  case SIGINT:
  case SIGTERM:
    LOG(INFO) << "Exiting due to INT/TERM signal";
    std::exit(0);
    break;
  case SIGFPE:
    LOG(ERROR) << "Fatal SIGFPE";
    printTrace = true;
    break;
  case SIGILL:
    LOG(ERROR) << "Fatal SIGILL";
    printTrace = true;
    break;
  case SIGABRT:
    LOG(ERROR) << "Fatal SIGABRT";
    printTrace = true;
    break;
  case SIGSEGV:
    LOG(ERROR) << "Fatal SIGSEGV";
    printTrace = true;
    break;
  }

  if (printTrace) {
    std::signal(SIGABRT, SIG_DFL); // also make sure to remove this due to
                                   // logger double report on FATAL
    LOG(ERROR) << boost::stacktrace::stacktrace();
  }

  std::raise(err_sig);
}

void installSignalHandlers() {
  std::signal(SIGFPE, &error_handler);
  std::signal(SIGILL, &error_handler);
  std::signal(SIGABRT, &error_handler);
  std::signal(SIGSEGV, &error_handler);
}

Chain::operator CBChain *() {
  auto chain = new CBChain(_name.c_str());
  chain->looped = _looped;
  for (auto blk : _blocks) {
    chain->addBlock(blk);
  }
  // blocks are unique so drain them here
  _blocks.clear();
  return chain;
}

CBRunChainOutput runChain(CBChain *chain, CBContext *context,
                          const CBVar &chainInput) {
  chain->previousOutput = CBVar();
  chain->started = true;
  chain->context = context;

  // store stack index
  auto sidx = context->stack.len;

  auto input = chainInput;
  for (auto blk : chain->blocks) {
    try {
      input = chain->previousOutput = activateBlock(blk, context, input);
      if (chain->previousOutput.valueType == None) {
        switch (chain->previousOutput.payload.chainState) {
        case CBChainState::Restart: {
          context->stack.len = sidx;
          return {chain->previousOutput, Restarted};
        }
        case CBChainState::Stop: {
          context->stack.len = sidx;
          return {chain->previousOutput, Stopped};
        }
        case CBChainState::Return: {
          context->stack.len = sidx;
          // Use input as output, return previous block result
          return {input, Restarted};
        }
        case CBChainState::Rebase:
          // Rebase means we need to put back main input
          input = chainInput;
          break;
        case CBChainState::Continue:
          break;
        }
      }
    } catch (boost::context::detail::forced_unwind const &e) {
      throw; // required for Boost Coroutine!
    } catch (const std::exception &e) {
      LOG(ERROR) << "Block activation error, failed block: "
                 << std::string(blk->name(blk));
      LOG(ERROR) << e.what();
      context->stack.len = sidx;
      return {chain->previousOutput, Failed};
    } catch (...) {
      LOG(ERROR) << "Block activation error, failed block: "
                 << std::string(blk->name(blk));
      context->stack.len = sidx;
      return {chain->previousOutput, Failed};
    }
  }

  context->stack.len = sidx;
  return {chain->previousOutput, Running};
}

bool warmup(CBChain *chain, CBContext *context) {
  for (auto blk : chain->blocks) {
    try {
      if (blk->warmup)
        blk->warmup(blk, context);
    } catch (const std::exception &e) {
      LOG(ERROR) << "Block warmup error, failed block: "
                 << std::string(blk->name(blk));
      LOG(ERROR) << e.what();
      return false;
    } catch (...) {
      LOG(ERROR) << "Block warmup error, failed block: "
                 << std::string(blk->name(blk));
      return false;
    }
  }
  return true;
}

boost::context::continuation run(CBChain *chain,
                                 boost::context::continuation &&sink) {
  auto running = true;
  // Reset return state
  chain->returned = false;
  // Clean previous output if we had one
  if (chain->ownedOutput) {
    destroyVar(chain->finishedOutput);
    chain->ownedOutput = false;
  }
  // Reset error
  chain->failed = false;
  // Create a new context and copy the sink in
  CBContext context(std::move(sink), chain);

  // We prerolled our coro, suspend here before actually starting.
  // This allows us to allocate the stack ahead of time.
  // And call warmup on all the blocks!
  if (!warmup(chain, &context)) {
    chain->failed = true;
    context.aborted = true;
    goto endOfChain;
  }

  context.continuation = context.continuation.resume();
  if (context.aborted) // We might have stopped before even starting!
    goto endOfChain;

  while (running) {
    running = chain->looped;
    context.restarted = false; // Remove restarted flag

    chain->finished = false; // Reset finished flag (atomic)
    auto runRes = runChain(chain, &context, chain->rootTickInput);
    chain->finishedOutput = runRes.output; // Write result before setting flag
    chain->finished = true;                // Set finished flag (atomic)
    context.iterationCount++;              // increatse iteration counter
    context.stack.len = 0;
    if (unlikely(runRes.state == Failed)) {
      chain->failed = true;
      context.aborted = true;
      break;
    } else if (unlikely(runRes.state == Stopped)) {
      context.aborted = true;
      break;
    }

    if (!chain->unsafe && chain->looped) {
      // Ensure no while(true), yield anyway every run
      context.next = Duration(0);
      context.continuation = context.continuation.resume();
      // This is delayed upon continuation!!
      if (context.aborted)
        break;
    }
  }

endOfChain:
  // Copy the output variable since the next call might wipe it
  auto tmp = chain->finishedOutput;
  // Reset it, we are not sure on the internal state
  chain->finishedOutput = {};
  chain->ownedOutput = true;
  cloneVar(chain->finishedOutput, tmp);

  // run cleanup on all the blocks
  cleanup(chain);

  // Need to take care that we might have stopped the chain very early due to
  // errors and the next eventual stop() should avoid resuming
  chain->returned = true;
  return std::move(context.continuation);
}

template <typename T>
NO_INLINE void arrayGrow(T &arr, size_t addlen, size_t min_cap) {
  // safety check to make sure this is not a borrowed foreign array!
  assert((arr.cap == 0 && arr.elements == nullptr) ||
         (arr.cap > 0 && arr.elements != nullptr));

  size_t min_len = arr.len + addlen;

  // compute the minimum capacity needed
  if (min_len > min_cap)
    min_cap = min_len;

  if (min_cap <= arr.cap)
    return;

  // increase needed capacity to guarantee O(1) amortized
  if (min_cap < 2 * arr.cap)
    min_cap = 2 * arr.cap;

#ifdef USE_RPMALLOC
  rpmalloc_initialize();
  arr.elements = (decltype(arr.elements))rpaligned_realloc(
      arr.elements, 16, sizeof(arr.elements[0]) * min_cap,
      sizeof(arr.elements[0]) * arr.len, 0);
#else
  auto newbuf =
      new (std::align_val_t{16}) uint8_t[sizeof(arr.elements[0]) * min_cap];
  if (arr.elements) {
    memcpy(newbuf, arr.elements, sizeof(arr.elements[0]) * arr.len);
    ::operator delete (arr.elements, std::align_val_t{16});
  }
  arr.elements = (decltype(arr.elements))newbuf;
#endif

  // also memset to 0 new memory in order to make cloneVars valid on new items
  size_t size = sizeof(arr.elements[0]) * (min_cap - arr.len);
  memset(arr.elements + arr.len, 0x0, size);

  arr.cap = min_cap;
}

template <typename T> NO_INLINE void arrayFree(T &arr) {
  if (arr.elements)
    ::operator delete (arr.elements, std::align_val_t{16});
  memset(&arr, 0x0, sizeof(T));
}

NO_INLINE void _destroyVarSlow(CBVar &var) {
  switch (var.valueType) {
  case Seq: {
    // notice we use .cap! because we make sure to 0 new empty elements
    for (size_t i = var.payload.seqValue.cap; i > 0; i--) {
      destroyVar(var.payload.seqValue.elements[i - 1]);
    }
    chainblocks::arrayFree(var.payload.seqValue);
  } break;
  case Table: {
    assert(var.payload.tableValue.api == &Globals::TableInterface);
    assert(var.payload.tableValue.opaque);
    auto map = (CBMap *)var.payload.tableValue.opaque;
    delete map;
  } break;
  default:
    break;
  };
}

NO_INLINE void _cloneVarSlow(CBVar &dst, const CBVar &src) {
  switch (src.valueType) {
  case Seq: {
    size_t srcLen = src.payload.seqValue.len;
#if 0
    destroyVar(dst);
    dst.valueType = Seq;
    dst.payload.seqValue = {};
    for (size_t i = 0; i < srcLen; i++) {
      auto &subsrc = src.payload.seqValue.elements[i];
      CBVar tmp{};
      cloneVar(tmp, subsrc);
      chainblocks::arrayPush(dst.payload.seqValue, tmp);
    }
#else
    // try our best to re-use memory
    if (dst.valueType != Seq) {
      destroyVar(dst);
      dst.valueType = Seq;
      for (size_t i = 0; i < srcLen; i++) {
        auto &subsrc = src.payload.seqValue.elements[i];
        CBVar tmp{};
        cloneVar(tmp, subsrc);
        chainblocks::arrayPush(dst.payload.seqValue, tmp);
      }
    } else {
      if (src.payload.seqValue.elements == dst.payload.seqValue.elements)
        return;

      if (srcLen <= dst.payload.seqValue.cap) {
        // clone on top of current values
        chainblocks::arrayResize(dst.payload.seqValue, srcLen);
        for (size_t i = 0; i < srcLen; i++) {
          auto &subsrc = src.payload.seqValue.elements[i];
          cloneVar(dst.payload.seqValue.elements[i], subsrc);
        }
      } else {
        size_t dstLen = dst.payload.seqValue.len;
        // re-use avail ones
        for (size_t i = 0; i < dstLen; i++) {
          auto &subsrc = src.payload.seqValue.elements[i];
          cloneVar(dst.payload.seqValue.elements[i], subsrc);
        }
        // append new values
        for (size_t i = dstLen; i < srcLen; i++) {
          auto &subsrc = src.payload.seqValue.elements[i];
          CBVar tmp{};
          cloneVar(tmp, subsrc);
          chainblocks::arrayPush(dst.payload.seqValue, subsrc);
        }
      }
    }
#endif
  } break;
  case Path:
  case ContextVar:
  case String: {
    auto srcSize = strlen(src.payload.stringValue) + 1;
    if ((dst.valueType != String && dst.valueType != ContextVar) ||
        dst.payload.stringCapacity < srcSize) {
      destroyVar(dst);
      dst.payload.stringValue = new char[srcSize];
      dst.payload.stringCapacity = srcSize;
    } else {
      if (src.payload.stringValue == dst.payload.stringValue)
        return;
    }

    dst.valueType = src.valueType;
    memcpy((void *)dst.payload.stringValue, (void *)src.payload.stringValue,
           srcSize - 1);
    ((char *)dst.payload.stringValue)[srcSize - 1] = 0;
  } break;
  case Image: {
    size_t srcImgSize = src.payload.imageValue.height *
                        src.payload.imageValue.width *
                        src.payload.imageValue.channels;
    size_t dstCapacity = dst.payload.imageValue.height *
                         dst.payload.imageValue.width *
                         dst.payload.imageValue.channels;
    if (dst.valueType != Image || srcImgSize > dstCapacity) {
      destroyVar(dst);
      dst.valueType = Image;
      dst.payload.imageValue.data = new uint8_t[srcImgSize];
    }

    dst.payload.imageValue.flags = dst.payload.imageValue.flags;
    dst.payload.imageValue.height = src.payload.imageValue.height;
    dst.payload.imageValue.width = src.payload.imageValue.width;
    dst.payload.imageValue.channels = src.payload.imageValue.channels;

    if (src.payload.imageValue.data == dst.payload.imageValue.data)
      return;

    memcpy(dst.payload.imageValue.data, src.payload.imageValue.data,
           srcImgSize);
  } break;
  case Table: {
    if (src.payload.tableValue.opaque == dst.payload.tableValue.opaque)
      return;

    CBMap *map;
    if (dst.valueType == Table) {
      map = (CBMap *)dst.payload.tableValue.opaque;
      map->clear();
    } else {
      destroyVar(dst);
      dst.valueType = Table;
      dst.payload.tableValue.api = &Globals::TableInterface;
      map = new CBMap();
      dst.payload.tableValue.opaque = map;
    }

    auto &ta = src.payload.tableValue;
    ta.api->tableForEach(
        ta,
        [](const char *key, CBVar *value, void *data) {
          auto map = (CBMap *)data;
          (*map)[key] = *value;
          return true;
        },
        map);
  } break;
  case Bytes: {
    if (dst.valueType != Bytes ||
        dst.payload.bytesCapacity < src.payload.bytesSize) {
      destroyVar(dst);
      dst.valueType = Bytes;
      dst.payload.bytesValue = new uint8_t[src.payload.bytesSize];
      dst.payload.bytesCapacity = src.payload.bytesSize;
    }

    dst.payload.bytesSize = src.payload.bytesSize;

    if (src.payload.bytesValue == dst.payload.bytesValue)
      return;

    memcpy((void *)dst.payload.bytesValue, (void *)src.payload.bytesValue,
           src.payload.bytesSize);
  } break;
  case CBType::Chain:
    if (dst != src) {
      destroyVar(dst);
    }
    dst.valueType = CBType::Chain;
    dst.payload.chainValue = CBChain::addRef(src.payload.chainValue);
    break;
  default:
    break;
  };
}
}; // namespace chainblocks

#ifdef TESTING
static CBChain mainChain("MainChain");

int main() {
  auto blk = chainblocks::createBlock("SetTableValue");
  LOG(INFO) << blk->name(blk);
  LOG(INFO) << blk->exposedVariables(blk);
}
#endif
