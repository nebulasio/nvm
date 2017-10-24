// Copyright (C) 2017 go-nebulas authors
//
// This file is part of the go-nebulas library.
//
// the go-nebulas library is free software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// the go-nebulas library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with the go-nebulas library.  If not, see
// <http://www.gnu.org/licenses/>.
//

#include "memory_manager.h"

MemoryManager::MemoryManager() {}

MemoryManager::~MemoryManager() {}

JITSymbol MemoryManager::findSymbol(const std::string &Name) {
  const char *NameStr = Name.c_str();

// DynamicLibrary::SearchForAddresOfSymbol expects an unmangled 'C' symbol
// name so ff we're on Darwin, strip the leading '_' off.
#ifdef __APPLE__
  if (NameStr[0] == '_') {
    ++NameStr;
  }
#endif

  printf("Name is %s, NameStr is %s\n", Name.c_str(), NameStr);

  uint64_t addr = 0;

  auto it = this->bindingSymbols.find(std::string(NameStr));
  if (it == this->bindingSymbols.end()) {
    addr = getSymbolAddress(Name);
  } else {
    addr = it->second;
  }

  return JITSymbol(addr, JITSymbolFlags::Exported);
}

void MemoryManager::bindSymbol(const char *Name, void *Address) {
  this->bindSymbol(std::string(Name), Address);
}

void MemoryManager::bindSymbol(const std::string &Name, void *Address) {
  this->bindingSymbols[Name] = (uint64_t)Address;
}
