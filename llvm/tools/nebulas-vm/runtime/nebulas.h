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

#pragma once

#ifdef _cplusplus
extern "C" {
#endif

typedef enum {
  code_succ = 0,
  code_invalid_assembly_file,
  code_invalid_priviliege,
  code_invalid_with_global_var
} nebulas_code_t;

// TODO Define all needed apis that communicate with the block chain

nebulas_code_t check_assembly(const char *filePath, const char *signature);

nebulas_code_t check_priviliege(const char *signature);

#ifdef _cplusplus
}
#endif
