/**
 * Copyright      2022  Xiaomi Corporation (authors: Fangjun Kuang)
 *
 * See LICENSE for clarification regarding multiple authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "sherpa-ncnn/csrc/symbol-table.h"

#include <cassert>
#include <cstdint>
#include <fstream>
#include <sstream>


#if __ANDROID_API__ >= 9
#include "android/asset_manager.h"
#include "android/asset_manager_jni.h"
#include "android/log.h"
#include <strstream>
#endif

namespace sherpa_ncnn {

SymbolTable::SymbolTable(const std::string &filename) {
  std::ifstream is(filename);
  Init(is);
}

#if __ANDROID_API__ >= 9
SymbolTable::SymbolTable(AAssetManager *mgr, const std::string &filename) {
  AAsset *asset = AAssetManager_open(mgr, filename.c_str(), AASSET_MODE_BUFFER);
  if (!asset) {
    __android_log_print(ANDROID_LOG_FATAL, "sherpa-ncnn",
                        "SymbolTable: Load %s failed", filename.c_str());
    exit(-1);
  }

  auto p = reinterpret_cast<const char *>(AAsset_getBuffer(asset));
  size_t asset_length = AAsset_getLength(asset);
  std::istrstream is(p, asset_length);
  Init(is);
  AAsset_close(asset);
}
#endif

void SymbolTable::Init(std::istream &is) {
  std::string sym;
  int32_t id;
  while (is >> sym >> id) {
    if (sym.size() >= 3) {
      // For BPE-based models, we replace ▁ with a space
      // Unicode 9601, hex 0x2581, utf8 0xe29681
      const uint8_t *p = reinterpret_cast<const uint8_t *>(sym.c_str());
      if (p[0] == 0xe2 && p[1] == 0x96 && p[2] == 0x81) {
        sym = sym.replace(0, 3, " ");
      }
    }

    assert(!sym.empty());
    assert(sym2id_.count(sym) == 0);
    assert(id2sym_.count(id) == 0);

    sym2id_.insert({sym, id});
    id2sym_.insert({id, sym});
  }
  assert(is.eof());
}

std::string SymbolTable::ToString() const {
  std::ostringstream os;
  char sep = ' ';
  for (const auto &p : sym2id_) {
    os << p.first << sep << p.second << "\n";
  }
  return os.str();
}

const std::string &SymbolTable::operator[](int32_t id) const {
  return id2sym_.at(id);
}

int32_t SymbolTable::operator[](const std::string &sym) const {
  return sym2id_.at(sym);
}

bool SymbolTable::contains(int32_t id) const { return id2sym_.count(id) != 0; }

bool SymbolTable::contains(const std::string &sym) const {
  return sym2id_.count(sym) != 0;
}

std::ostream &operator<<(std::ostream &os, const SymbolTable &symbol_table) {
  return os << symbol_table.ToString();
}

}  // namespace sherpa_ncnn
