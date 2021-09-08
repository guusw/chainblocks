/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2019 Fragcolor Pte. Ltd. */

#ifndef CB_BLOCKWRAPPER_HPP
#define CB_BLOCKWRAPPER_HPP

#include "chainblocks.hpp"
#include "utility.hpp"

namespace chainblocks {
CB_HAS_MEMBER_TEST(name);
CB_HAS_MEMBER_TEST(hash);
CB_HAS_MEMBER_TEST(help);
CB_HAS_MEMBER_TEST(inputHelp);
CB_HAS_MEMBER_TEST(outputHelp);
CB_HAS_MEMBER_TEST(setup);
CB_HAS_MEMBER_TEST(destroy);
CB_HAS_MEMBER_TEST(inputTypes);
CB_HAS_MEMBER_TEST(outputTypes);
CB_HAS_MEMBER_TEST(exposedVariables);
CB_HAS_MEMBER_TEST(requiredVariables);
CB_HAS_MEMBER_TEST(compose);
CB_HAS_MEMBER_TEST(composed);
CB_HAS_MEMBER_TEST(parameters);
CB_HAS_MEMBER_TEST(setParam);
CB_HAS_MEMBER_TEST(getParam);
CB_HAS_MEMBER_TEST(warmup);
CB_HAS_MEMBER_TEST(activate);
CB_HAS_MEMBER_TEST(nextFrame);
CB_HAS_MEMBER_TEST(cleanup);
CB_HAS_MEMBER_TEST(mutate);
CB_HAS_MEMBER_TEST(crossover);
CB_HAS_MEMBER_TEST(getState);
CB_HAS_MEMBER_TEST(setState);
CB_HAS_MEMBER_TEST(resetState);

// Composition is preferred
template <class T> struct BlockWrapper {
  CBlock header;
  T block;
  static inline const char *name = "";
  static inline uint32_t crc = 0;

  static __cdecl CBlock *create() {
    CBlock *result = reinterpret_cast<CBlock *>(new (std::align_val_t{16})
                                                    BlockWrapper<T>());

    // name
    if constexpr (has_name<T>::value) {
      result->name = static_cast<CBNameProc>([](CBlock *b) {
        return reinterpret_cast<BlockWrapper<T> *>(b)->block.name();
      });
    } else {
      result->name = static_cast<CBNameProc>([](CBlock *b) { return name; });
    }

    // name
    if constexpr (has_hash<T>::value) {
      result->hash = static_cast<CBHashProc>([](CBlock *b) {
        return reinterpret_cast<BlockWrapper<T> *>(b)->block.hash();
      });
    } else {
      result->hash = static_cast<CBHashProc>([](CBlock *b) { return crc; });
    }

    // help
    if constexpr (has_help<T>::value) {
      result->help = static_cast<CBHelpProc>([](CBlock *b) {
        return reinterpret_cast<BlockWrapper<T> *>(b)->block.help();
      });
    } else {
      result->help =
          static_cast<CBHelpProc>([](CBlock *b) { return CBOptionalString(); });
    }

    // inputHelp
    if constexpr (has_inputHelp<T>::value) {
      result->inputHelp = static_cast<CBHelpProc>([](CBlock *b) {
        return reinterpret_cast<BlockWrapper<T> *>(b)->block.inputHelp();
      });
    } else {
      result->inputHelp =
          static_cast<CBHelpProc>([](CBlock *b) { return CBOptionalString(); });
    }

    // outputHelp
    if constexpr (has_outputHelp<T>::value) {
      result->outputHelp = static_cast<CBHelpProc>([](CBlock *b) {
        return reinterpret_cast<BlockWrapper<T> *>(b)->block.outputHelp();
      });
    } else {
      result->outputHelp =
          static_cast<CBHelpProc>([](CBlock *b) { return CBOptionalString(); });
    }

    // setup
    if constexpr (has_setup<T>::value) {
      result->setup = static_cast<CBSetupProc>([](CBlock *b) {
        reinterpret_cast<BlockWrapper<T> *>(b)->block.setup();
      });
    } else {
      result->setup = static_cast<CBSetupProc>([](CBlock *b) {});
    }

    // destroy
    if constexpr (has_destroy<T>::value) {
      result->destroy = static_cast<CBDestroyProc>([](CBlock *b) {
        auto bw = reinterpret_cast<BlockWrapper<T> *>(b);
        bw->block.destroy();
        bw->BlockWrapper<T>::~BlockWrapper<T>();
        ::operator delete (bw, std::align_val_t{16});
      });
    } else {
      result->destroy = static_cast<CBDestroyProc>([](CBlock *b) {
        auto bw = reinterpret_cast<BlockWrapper<T> *>(b);
        bw->BlockWrapper<T>::~BlockWrapper<T>();
        ::operator delete (bw, std::align_val_t{16});
      });
    }

    // inputTypes
    static_assert(has_inputTypes<T>::value,
                  "Blocks must have an \"inputTypes\" method.");
    if constexpr (has_inputTypes<T>::value) {
      result->inputTypes = static_cast<CBInputTypesProc>([](CBlock *b) {
        return reinterpret_cast<BlockWrapper<T> *>(b)->block.inputTypes();
      });
    }

    // outputTypes
    static_assert(has_outputTypes<T>::value,
                  "Blocks must have an \"outputTypes\" method.");
    if constexpr (has_outputTypes<T>::value) {
      result->outputTypes = static_cast<CBOutputTypesProc>([](CBlock *b) {
        return reinterpret_cast<BlockWrapper<T> *>(b)->block.outputTypes();
      });
    }

    // exposedVariables
    if constexpr (has_exposedVariables<T>::value) {
      result->exposedVariables =
          static_cast<CBExposedVariablesProc>([](CBlock *b) {
            return reinterpret_cast<BlockWrapper<T> *>(b)
                ->block.exposedVariables();
          });
    } else {
      result->exposedVariables = static_cast<CBExposedVariablesProc>(
          [](CBlock *b) { return CBExposedTypesInfo(); });
    }

    // requiredVariables
    if constexpr (has_requiredVariables<T>::value) {
      result->requiredVariables =
          static_cast<CBRequiredVariablesProc>([](CBlock *b) {
            return reinterpret_cast<BlockWrapper<T> *>(b)
                ->block.requiredVariables();
          });
    } else {
      result->requiredVariables = static_cast<CBRequiredVariablesProc>(
          [](CBlock *b) { return CBExposedTypesInfo(); });
    }

    // parameters
    if constexpr (has_parameters<T>::value) {
      result->parameters = static_cast<CBParametersProc>([](CBlock *b) {
        return reinterpret_cast<BlockWrapper<T> *>(b)->block.parameters();
      });
    } else {
      result->parameters = static_cast<CBParametersProc>(
          [](CBlock *b) { return CBParametersInfo(); });
    }

    // setParam
    if constexpr (has_setParam<T>::value) {
      result->setParam =
          static_cast<CBSetParamProc>([](CBlock *b, int i, const CBVar *v) {
            reinterpret_cast<BlockWrapper<T> *>(b)->block.setParam(i, *v);
          });
    } else {
      result->setParam =
          static_cast<CBSetParamProc>([](CBlock *b, int i, const CBVar *v) {});
    }

    // getParam
    if constexpr (has_getParam<T>::value) {
      result->getParam = static_cast<CBGetParamProc>([](CBlock *b, int i) {
        return reinterpret_cast<BlockWrapper<T> *>(b)->block.getParam(i);
      });
    } else {
      result->getParam =
          static_cast<CBGetParamProc>([](CBlock *b, int i) { return CBVar(); });
    }

    // compose
    if constexpr (has_compose<T>::value) {
      result->compose =
          static_cast<CBComposeProc>([](CBlock *b, CBInstanceData data) {
            return reinterpret_cast<BlockWrapper<T> *>(b)->block.compose(data);
          });
    } else {
      // compose is optional!
      result->compose = nullptr;
    }

    // composed
    if constexpr (has_composed<T>::value) {
      result->composed = static_cast<CBComposedProc>(
          [](CBlock *b, const CBChain *chain, const CBComposeResult *result) {
            reinterpret_cast<BlockWrapper<T> *>(b)->block.composed(chain,
                                                                   result);
          });
    } else {
      // composed is optional!
      result->composed = nullptr;
    }

    // warmup
    if constexpr (has_warmup<T>::value) {
      result->warmup = static_cast<CBWarmupProc>([](CBlock *b, CBContext *ctx) {
        reinterpret_cast<BlockWrapper<T> *>(b)->block.warmup(ctx);
      });
    } else {
      // warmup is optional!
      result->warmup = nullptr;
    }

    // nextFrame
    if constexpr (has_nextFrame<T>::value) {
      result->nextFrame =
          static_cast<CBNextFrameProc>([](CBlock *b, CBContext *ctx) {
            reinterpret_cast<BlockWrapper<T> *>(b)->block.nextFrame(ctx);
          });
    } else {
      // nextFrame is optional!
      result->nextFrame = nullptr;
    }

    // activate
    static_assert(has_activate<T>::value,
                  "Blocks must have an \"activate\" method.");
    if constexpr (has_activate<T>::value) {
      result->activate = static_cast<CBActivateProc>(
          [](CBlock *b, CBContext *ctx, const CBVar *v) {
            return reinterpret_cast<BlockWrapper<T> *>(b)->block.activate(ctx,
                                                                          *v);
          });
    }

    // cleanup
    if constexpr (has_cleanup<T>::value) {
      result->cleanup = static_cast<CBCleanupProc>([](CBlock *b) {
        reinterpret_cast<BlockWrapper<T> *>(b)->block.cleanup();
      });
    } else {
      result->cleanup = static_cast<CBCleanupProc>([](CBlock *b) {});
    }

    // mutate
    if constexpr (has_mutate<T>::value) {
      result->mutate =
          static_cast<CBMutateProc>([](CBlock *b, CBTable options) {
            reinterpret_cast<BlockWrapper<T> *>(b)->block.mutate(options);
          });
    } else {
      // mutate is optional!
      result->mutate = nullptr;
    }

    // crossover
    if constexpr (has_crossover<T>::value) {
      result->crossover = static_cast<CBCrossoverProc>(
          [](CBlock *b, const CBVar *state0, const CBVar *state1) {
            reinterpret_cast<BlockWrapper<T> *>(b)->block.crossover(*state0,
                                                                    *state1);
          });
    } else {
      // crossover is optional!
      result->crossover = nullptr;
    }

    // getState
    if constexpr (has_getState<T>::value) {
      result->getState = static_cast<CBGetStateProc>([](CBlock *b) {
        return reinterpret_cast<BlockWrapper<T> *>(b)->block.getState();
      });
    } else {
      // getState is optional!
      result->getState = nullptr;
    }

    // setState
    if constexpr (has_setState<T>::value) {
      result->setState =
          static_cast<CBSetStateProc>([](CBlock *b, const CBVar *state) {
            reinterpret_cast<BlockWrapper<T> *>(b)->block.setState(*state);
          });
    } else {
      // setState is optional!
      result->setState = nullptr;
    }

    // resetState
    if constexpr (has_resetState<T>::value) {
      result->resetState = static_cast<CBResetStateProc>([](CBlock *b) {
        reinterpret_cast<BlockWrapper<T> *>(b)->block.resetState();
      });
    } else {
      // resetState is optional!
      result->resetState = nullptr;
    }

    return result;
  }
};

#define REGISTER_CBLOCK(__name__, __type__)                                    \
  ::chainblocks::BlockWrapper<__type__>::name = __name__;                      \
  ::chainblocks::BlockWrapper<__type__>::crc =                                 \
      ::chainblocks::constant<::chainblocks::crc32(                            \
          __name__ CHAINBLOCKS_CURRENT_ABI_STR)>::value;                       \
  ::chainblocks::registerBlock(::chainblocks::BlockWrapper<__type__>::name,    \
                               &::chainblocks::BlockWrapper<__type__>::create, \
                               NAMEOF_FULL_TYPE(__type__))

#define OVERRIDE_ACTIVATE(__data__, __func__)                                  \
  __data__.block->activate = static_cast<CBActivateProc>([](CBlock *b,         \
                                                            CBContext *ctx,    \
                                                            const CBVar *v) {  \
    return reinterpret_cast<                                                   \
               BlockWrapper<std::remove_pointer<decltype(this)>::type> *>(b)   \
        ->block.__func__(ctx, *v);                                             \
  })

template <typename CBCORE, Parameters &Params, size_t NPARAMS, Type &InputType,
          Type &OutputType>
struct TSimpleBlock {
  static CBTypesInfo inputTypes() { return InputType; }
  static CBTypesInfo outputTypes() { return OutputType; }
  static CBParametersInfo parameters() { return Params; }

  void setParam(int index, const CBVar &value) { params[index] = value; }

  CBVar getParam(int index) { return params[index]; }

  void cleanup() {
    for (auto &param : params) {
      params.cleanup();
    }
  }

  void warmup(CBContext *context) {
    for (auto &param : params) {
      params.warmup(context);
    }
  }

protected:
  constexpr CBVar &param(size_t idx) {
    static_assert(idx < NPARAMS, "Parameter index out of range.");
    return params[idx].get();
  }

private:
  std::array<TParamVar<CBCORE>, NPARAMS> params;
};

typedef CBVar (*LambdaActivate)(const CBVar &input);
template <LambdaActivate F, Type &InputType, Type &OutputType>
struct LambdaBlock {
  static CBTypesInfo inputTypes() { return InputType; }
  static CBTypesInfo outputTypes() { return OutputType; }

  CBVar activate(CBContext *context, const CBVar &input) { return F(input); }
};
}; // namespace chainblocks

#endif
