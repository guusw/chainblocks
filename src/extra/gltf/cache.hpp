#pragma once
#include <memory>
#include "xxhash.h"

namespace chainblocks {
namespace gltf {

template <typename T> struct Cache {
  std::shared_ptr<T> find(uint64_t hash) {
    std::shared_ptr<T> res;
    std::unique_lock lock(_mutex);
    auto it = _cache.find(hash);
    if (it != _cache.end()) {
      res = it->second.lock();
    }
    return res;
  }

  void add(uint64_t hash, const std::shared_ptr<T> &data) {
    std::unique_lock lock(_mutex);
    _cache[hash] = data;
  }

private:
  std::mutex _mutex;
  // weak pointers in order to avoid holding in memory
  std::unordered_map<uint64_t, std::weak_ptr<T>> _cache;
};

struct TextureCache : public Cache<GFXTexture> {
  static uint64_t hashTexture(const Texture &gltexture,
                              const GLTFImage &glsource) {
    XXH3_state_s hashState;
    XXH3_INITSTATE(&hashState);
    XXH3_64bits_reset_withSecret(&hashState, CUSTOM_XXH3_kSecret,
                                 XXH_SECRET_DEFAULT_SIZE);

    XXH3_64bits_update(&hashState, glsource.image.data(),
                       glsource.image.size());

    return XXH3_64bits_digest(&hashState);
  }
};

struct ShadersCache : public Cache<GFXShader> {
  static uint64_t hashShader(const std::string &defines) {
    XXH3_state_s hashState;
    XXH3_INITSTATE(&hashState);
    XXH3_64bits_reset_withSecret(&hashState, CUSTOM_XXH3_kSecret,
                                 XXH_SECRET_DEFAULT_SIZE);

    XXH3_64bits_update(&hashState, defines.data(), defines.size());

    return XXH3_64bits_digest(&hashState);
  }
};

} // namespace gltf
} // namespace chainblocks
