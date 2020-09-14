/* 
 * GridTools
 * 
 * Copyright (c) 2014-2020, ETH Zurich
 * All rights reserved.
 * 
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 */
#ifndef INCLUDED_GHEX_STRUCTURED_RMA_PUT_HPP
#define INCLUDED_GHEX_STRUCTURED_RMA_PUT_HPP

#include "../common/utils.hpp"
#include "../cuda_utils/stream.hpp"
#include "./rma_range.hpp"

namespace gridtools {
namespace ghex {
namespace structured {

// Here are different specializations of put functions. A put function takes 2 arguments (source and
// target range) and performs the RMA put.

template<typename SourceField, typename TargetField>
using cpu_to_cpu = std::integral_constant<bool,
    std::is_same<typename SourceField::arch_type, gridtools::ghex::cpu>::value &&
    std::is_same<typename TargetField::arch_type, gridtools::ghex::cpu>::value>;
template<typename SourceField, typename TargetField>
using cpu_to_gpu = std::integral_constant<bool,
    std::is_same<typename SourceField::arch_type, gridtools::ghex::cpu>::value &&
    std::is_same<typename TargetField::arch_type, gridtools::ghex::gpu>::value>;
template<typename SourceField, typename TargetField>
using gpu_to_cpu = std::integral_constant<bool,
    std::is_same<typename SourceField::arch_type, gridtools::ghex::gpu>::value &&
    std::is_same<typename TargetField::arch_type, gridtools::ghex::cpu>::value>;
template<typename SourceField, typename TargetField>
using gpu_to_gpu = std::integral_constant<bool,
    std::is_same<typename SourceField::arch_type, gridtools::ghex::gpu>::value &&
    std::is_same<typename TargetField::arch_type, gridtools::ghex::gpu>::value>;

template<typename SourceField, typename TargetField>
inline std::enable_if_t<
    cpu_to_cpu<SourceField,TargetField>::value && !rma_range<SourceField>::fuse_components::value>
put(rma_range<SourceField>& s, rma_range<TargetField>& t
#ifdef __CUDACC__
    , cudaStream_t
#endif
)
{
    using sv_t = rma_range<SourceField>;
    using coordinate = typename sv_t::coordinate;
    gridtools::ghex::detail::for_loop<
        sv_t::dimension::value,
        sv_t::dimension::value,
        typename sv_t::layout, 1>::
    apply([&s,&t](auto... c)
    {
        std::memcpy(t.ptr(coordinate{c...}), s.ptr(coordinate{c...}), s.m_chunk_size);
    },
    s.m_begin, s.m_end);
}

template<typename SourceField, typename TargetField>
inline std::enable_if_t<
    cpu_to_cpu<SourceField,TargetField>::value && rma_range<SourceField>::fuse_components::value>
put(rma_range<SourceField>& s, rma_range<TargetField>& t
#ifdef __CUDACC__
    , cudaStream_t
#endif
)
{
    using sv_t = rma_range<SourceField>;
    using coordinate = typename sv_t::coordinate;
    const auto nc = s.m_field.num_components();
    gridtools::ghex::detail::for_loop<
        sv_t::dimension::value,
        sv_t::dimension::value,
        typename sv_t::layout, 2>::
    apply([&s,&t,nc](auto... c)
    {
        std::memcpy(t.ptr(coordinate{c...}), s.ptr(coordinate{c...}), s.m_chunk_size*nc);
    },
    s.m_begin, s.m_end);
}

template<typename SourceField, typename TargetField>
inline std::enable_if_t<
    cpu_to_gpu<SourceField,TargetField>::value>
put(rma_range<SourceField>&, rma_range<TargetField>&
#ifdef __CUDACC__
    , cudaStream_t
#endif
)
{
    // TODO
}

template<typename SourceField, typename TargetField>
inline std::enable_if_t<
    gpu_to_cpu<SourceField,TargetField>::value>
put(rma_range<SourceField>&, rma_range<TargetField>&
#ifdef __CUDACC__
    , cudaStream_t
#endif
)
{
    // TODO
}

#ifdef __CUDACC__
template<typename SourceRange, typename TargetRange>
__global__ void put_device_to_device_kernel(SourceRange sr, TargetRange tr)
{
    const unsigned int index = blockIdx.x*blockDim.x + threadIdx.x;
    if (index < sr.m_size)
    {
        auto s_it = sr.begin();
        s_it += index;
        auto s_chunk = *s_it;
        auto t_it = tr.begin();
        t_it += index;
        auto t_chunk = *t_it;
        memcpy(t_chunk.data(), s_chunk.data(), s_chunk.bytes());
    }
}
#endif

template<typename SourceField, typename TargetField>
inline std::enable_if_t<
    gpu_to_gpu<SourceField,TargetField>::value>
put(rma_range<SourceField>& s, rma_range<TargetField>& t
#ifdef __CUDACC__
    , cudaStream_t st
#endif
)
{
#ifdef __CUDACC__
    /*//cuda::stream st;
    static constexpr unsigned int block_dim = 128;
    const unsigned int num_blocks = (s.m_size+block_dim-1)/block_dim;
    put_device_to_device_kernel<<<num_blocks,block_dim,0,st>>>(s, t);
    //st.sync();*/
    using sv_t = rma_range<SourceField>;
    using coordinate = typename sv_t::coordinate;
    gridtools::ghex::detail::for_loop<
        sv_t::dimension::value,
        sv_t::dimension::value,
        typename sv_t::layout, 1>::
    apply([&s,&t,&st](auto... c)
    {
        cudaMemcpyAsync(t.ptr(coordinate{c...}), s.ptr(coordinate{c...}), s.m_chunk_size, 
            cudaMemcpyDeviceToDevice, st);
    cudaStreamSynchronize(st);
    },
    s.m_begin, s.m_end);
#endif
}

} // namespace structured
} // namespace ghex
} // namespace gridtools

#endif /* INCLUDED_GHEX_STRUCTURED_RMA_PUT_HPP */
