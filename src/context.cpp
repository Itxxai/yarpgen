/*
Copyright (c) 2019-2020, Intel Corporation
Copyright (c) 2019-2020, University of Utah

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

//////////////////////////////////////////////////////////////////////////////
#include "context.h"

#include <utility>

using namespace yarpgen;

std::shared_ptr<EmitCtx> EmitCtx::default_emit_ctx =
    std::make_shared<EmitCtx>();

PopulateCtx::PopulateCtx(std::shared_ptr<PopulateCtx> _par_ctx)
    : par_ctx(std::move(_par_ctx)), ext_inp_sym_tbl(par_ctx->ext_inp_sym_tbl),
      ext_out_sym_tbl(par_ctx->ext_out_sym_tbl), arith_depth(0), taken(true),
      inside_mutation(false), inside_omp_simd(false), in_stencil(false) {
    local_sym_tbl = std::make_shared<SymbolTable>();
    if (par_ctx.use_count() != 0) {
        gen_policy = par_ctx->gen_policy;
        local_sym_tbl =
            std::make_shared<SymbolTable>(*(par_ctx->getLocalSymTable()));
        loop_depth = par_ctx->getLoopDepth();
        arith_depth = par_ctx->getArithDepth();
        taken = par_ctx->isTaken();
        inside_mutation = par_ctx->isInsideMutation();
        inside_omp_simd = par_ctx->inside_omp_simd;
        dims = par_ctx->dims;
        in_stencil = par_ctx->in_stencil;
    }
}

PopulateCtx::PopulateCtx() {
    par_ctx = nullptr;
    local_sym_tbl = std::make_shared<SymbolTable>();
    ext_inp_sym_tbl = std::make_shared<SymbolTable>();
    ext_out_sym_tbl = std::make_shared<SymbolTable>();
    arith_depth = 0;
    taken = true;
    inside_mutation = false;
    inside_omp_simd = false;
    in_stencil = false;
}

size_t PopulateCtx::generateNumberOfDims(ArrayDimsUseKind dims_use_kind) const {
    if (dims_use_kind == ArrayDimsUseKind::SAME ||
        (dims_use_kind == ArrayDimsUseKind::FEWER && dims.size() == 1))
        return dims.size();
    else if (dims_use_kind == ArrayDimsUseKind::FEWER && dims.size() > 1)
        return rand_val_gen->getRandValue(static_cast<size_t>(1),
                                          dims.size() - 1);
    else if (dims_use_kind == ArrayDimsUseKind::MORE) {
        return rand_val_gen->getRandValue(
            dims.size() + 1,
            std::min(static_cast<size_t>(std::ceil(
                         dims.size() * gen_policy->arrays_dims_ext_factor)),
                     gen_policy->array_dims_num_limit));
    }
    else
        ERROR("Unsupported case!");
}

void SymbolTable::addArray(std::shared_ptr<Array> array) {
    arrays.push_back(array);
    assert(array->getType()->isArrayType() &&
           "Array should have an array type");
    auto array_type = std::static_pointer_cast<ArrayType>(array->getType());
    array_dim_map[array_type->getDimensions().size()].push_back(array);
}

std::vector<std::shared_ptr<Array>>
SymbolTable::getArraysWithDimNum(size_t dim) {
    auto find_res = array_dim_map.find(dim);
    if (find_res != array_dim_map.end())
        return find_res->second;
    return {};
}
