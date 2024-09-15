#pragma once
#include "cuda/typedef.cuh"
#include "solver/gpu_solver.hpp"

namespace PC3::Kernel::Summation {

template <typename buffer_type, Type::uint32 NMax, float w, float... W>
PULSE_DEVICE PULSE_INLINE buffer_type sum_single_k( Type::uint32 i, buffer_type* buffer,
                                             Type::uint32 offset ) {
    if constexpr ( sizeof...( W ) == 0 ) {
        // Last Weight
        return w * buffer[i + offset * ( NMax - sizeof...(W)-1 )];
    }
    if constexpr ( w == 0.0 ) {
        return sum_single_k<buffer_type, NMax, W...>( i, buffer, offset );
    }
    // The constexpr if is logically redundant, but we need it so the compiler doesnt complain about not being able to call <buffer_type,int,w>
    // For sizeof..(W)==0, this line is never reached, but the compiler stil complains.
    if constexpr ( sizeof...( W ) > 0 ) {   
        return w * buffer[i + offset * ( NMax - sizeof...(W)-1 )] + sum_single_k<buffer_type, NMax, W...>( i, buffer, offset );
    }
}

// TODO: summation kernel aufteilen in part mit und part ohne dw
// TODO: summation kernel nur noch einzen für jeden buffer. dann 
// für gpu switch case machen mit wf,wf+rv,wfp,wfm,wfp+wfm+rvp+rvm
// für cu auch; launched dann entsprechend die loops. switch VOR den loops,
// d.h. in jedem case steht ein loop. 
// Sollte für cu im fall ohne rv dann dafür sorgen, dass da nur eine einzelne summation steht, die ezpz simd optimiert werden kann.

// Specifically use Type::uint32 N instead of sizeof(Weights) to force MSVC to NOT inline this function for different solvers (RK3,RK4) which cases the respective RK solver to call the wrong template function.


template <typename buffer_type, Type::uint32 N, float... Weights>
PULSE_DEVICE PULSE_INLINE void runge_sum_to_input_kw( Type::uint32 i, buffer_type dw, Type::uint32 current_halo, Solver::VKernelArguments time, Solver::KernelArguments args, buffer_type* input, buffer_type* output, buffer_type* k_vec) {
    GENERATE_SUBGRID_INDEX( i, current_halo );

    buffer_type res = sum_single_k<buffer_type, sizeof...(Weights), Weights...>( i, k_vec, args.p.subgrid_N2_with_halo );
    
    output[i] = input[i] + time.dt * res + dw;
}

template <typename buffer_type, Type::uint32 N, float... Weights>
PULSE_DEVICE PULSE_INLINE void runge_add_to_input_kw( Type::uint32 i, buffer_type dw, Type::uint32 current_halo, Solver::VKernelArguments time, Solver::KernelArguments args, buffer_type* input_output, buffer_type* k_vec) {
    GENERATE_SUBGRID_INDEX( i, current_halo );

    buffer_type res = sum_single_k<buffer_type, sizeof...(Weights), Weights...>( i, k_vec, args.p.subgrid_N2_with_halo );
    
    input_output[i] += time.dt * res + dw;
}

template <float... Weights>
constexpr float sum_weights() {
    return (Weights + ...);  // Fold expression in C++17 or later
}

// Hardcoded RK1 Kernel
template <typename buffer_type, Type::uint32 N, float w>
PULSE_DEVICE PULSE_INLINE void runge_sum_to_input_k1( Type::uint32 i, Type::uint32 offset, buffer_type dw, Type::uint32 current_halo, Solver::VKernelArguments time, Solver::KernelArguments args, buffer_type* input, buffer_type* output, buffer_type* k_vec) {
    GENERATE_SUBGRID_INDEX( i, current_halo );
        
    output[i] = input[i] + time.dt * w * k_vec[i+offset] + dw;
}
template <typename buffer_type, Type::uint32 N, float w>
PULSE_DEVICE PULSE_INLINE void runge_add_to_input_k1( Type::uint32 i, Type::uint32 offset, buffer_type dw, Type::uint32 current_halo, Solver::VKernelArguments time, Solver::KernelArguments args, buffer_type* input_output, buffer_type* k_vec) {
    GENERATE_SUBGRID_INDEX( i, current_halo );
        
    input_output[i] += time.dt * w * k_vec[i+offset] + dw;
}

// Hardcoded RK2 Kernel
template <typename buffer_type, Type::uint32 N, float w1, float w2>
PULSE_DEVICE PULSE_INLINE void runge_sum_to_input_k2( Type::uint32 i, buffer_type dw, Type::uint32 current_halo, Solver::VKernelArguments time, Solver::KernelArguments args, buffer_type* input, buffer_type* output, buffer_type* k_vec) {
    GENERATE_SUBGRID_INDEX( i, current_halo );

    output[i] = input[i] + time.dt * ( w1*k_vec[i] + w2* k_vec[i + args.p.subgrid_N2_with_halo ] ) + dw;
}
template <typename buffer_type, Type::uint32 N, float w1, float w2>
PULSE_DEVICE PULSE_INLINE void runge_add_to_input_k2( Type::uint32 i, buffer_type dw, Type::uint32 current_halo, Solver::VKernelArguments time, Solver::KernelArguments args, buffer_type* input_output, buffer_type* k_vec) {
    GENERATE_SUBGRID_INDEX( i, current_halo );

    input_output[i] += time.dt * ( w1*k_vec[i] + w2* k_vec[i + args.p.subgrid_N2_with_halo ] ) + dw;
}

// Hardcoded RK3 Kernel
template <typename buffer_type, Type::uint32 N, float w1, float w2, float w3>
PULSE_DEVICE PULSE_INLINE void runge_sum_to_input_k3( Type::uint32 i, buffer_type dw, Type::uint32 current_halo, Solver::VKernelArguments time, Solver::KernelArguments args, buffer_type* input, buffer_type* output, buffer_type* k_vec) {
    GENERATE_SUBGRID_INDEX( i, current_halo );

    output[i] = input[i] + time.dt * ( w1*k_vec[i] + w2* k_vec[i + args.p.subgrid_N2_with_halo ] + w3*k_vec[i + 2*args.p.subgrid_N2_with_halo ] ) + dw;
}
template <typename buffer_type, Type::uint32 N, float w1, float w2, float w3>
PULSE_DEVICE PULSE_INLINE void runge_add_to_input_k3( Type::uint32 i, buffer_type dw, Type::uint32 current_halo, Solver::VKernelArguments time, Solver::KernelArguments args, buffer_type* input_output, buffer_type* k_vec) {
    GENERATE_SUBGRID_INDEX( i, current_halo );

    input_output[i] += time.dt * ( w1*k_vec[i] + w2* k_vec[i + args.p.subgrid_N2_with_halo ] + w3*k_vec[i + 2*args.p.subgrid_N2_with_halo ] ) + dw;
}

// Hardcoded RK4 kernel
template <typename buffer_type, Type::uint32 N, float w1, float w2, float w3, float w4>
PULSE_DEVICE PULSE_INLINE void runge_sum_to_input_k4( Type::uint32 i, buffer_type dw, Type::uint32 current_halo, Solver::VKernelArguments time, Solver::KernelArguments args, buffer_type* input, buffer_type* output, buffer_type* k_vec) {
    GENERATE_SUBGRID_INDEX( i, current_halo );

    output[i] = input[i] + time.dt * ( w1*k_vec[i] + w2* k_vec[i + args.p.subgrid_N2_with_halo ] + w3*k_vec[i + 2*args.p.subgrid_N2_with_halo ] + w4*k_vec[i + 3*args.p.subgrid_N2_with_halo ] ) + dw;
}
template <typename buffer_type, Type::uint32 N, float w1, float w2, float w3, float w4>
PULSE_DEVICE PULSE_INLINE void runge_add_to_input_k4( Type::uint32 i, buffer_type dw, Type::uint32 current_halo, Solver::VKernelArguments time, Solver::KernelArguments args, buffer_type* input_output, buffer_type* k_vec) {
    GENERATE_SUBGRID_INDEX( i, current_halo );

    input_output[i] += time.dt * ( w1*k_vec[i] + w2* k_vec[i + args.p.subgrid_N2_with_halo ] + w3*k_vec[i + 2*args.p.subgrid_N2_with_halo ] + w4*k_vec[i + 3*args.p.subgrid_N2_with_halo ] ) + dw;
}

// Helper function to chose the correct kernel function
// This way we can hardcode a lot of the RK kernels and still have a single kernel function to call, hopefully at no performance cost.
// This way we can also hardcode more K functions, if we want to.
template <typename buffer_type, bool include_dw, Type::uint32 N, float... Weights>
PULSE_GLOBAL PULSE_COMPILER_SPECIFIC void runge_sum_to_input_k( Type::uint32 i, Type::uint32 current_halo, Solver::VKernelArguments time, Solver::KernelArguments args, buffer_type* input, buffer_type* output, buffer_type* k_vec ) {
    buffer_type dw = 0.0;
    if constexpr ( include_dw ) {
        if ( args.p.stochastic_amplitude > 0.0 ) {
            constexpr float w_sum = sum_weights<Weights...>();
            dw = w_sum * args.dev_ptrs.random_number[i] * CUDA::sqrt( ( args.p.R * args.dev_ptrs.reservoir_plus[i] + args.p.gamma_c ) / ( Type::real( 4.0 ) * args.p.dV ) ) / time.dt;
        }
    }
    if constexpr ( sizeof...( Weights ) == 1 ) {
        runge_sum_to_input_k1<buffer_type, N, Weights...>( i, (N-1)*args.p.subgrid_N2_with_halo, dw, current_halo, time, args, input, output, k_vec );
    } else if constexpr ( sizeof...( Weights ) == 2 ) {
        runge_sum_to_input_k2<buffer_type, N, Weights...>( i, dw, current_halo, time, args, input, output, k_vec );
    } else if constexpr ( sizeof...( Weights ) == 3 ) {
        runge_sum_to_input_k3<buffer_type, N, Weights...>( i, dw, current_halo, time, args, input, output, k_vec );
    } else if constexpr ( sizeof...( Weights ) == 4 ) {
        runge_sum_to_input_k4<buffer_type, N, Weights...>( i, dw, current_halo, time, args, input, output, k_vec );
    } else {
        runge_sum_to_input_kw<buffer_type, N, Weights...>( i, dw, current_halo, time, args, input, output, k_vec );
    }
}
template <typename buffer_type, bool include_dw, Type::uint32 N, float... Weights>
PULSE_GLOBAL PULSE_COMPILER_SPECIFIC void runge_add_to_input_k( Type::uint32 i, Type::uint32 current_halo, Solver::VKernelArguments time, Solver::KernelArguments args, buffer_type* input_output, buffer_type* k_vec ) {
    buffer_type dw = 0.0;
    if constexpr ( include_dw ) {
        if ( args.p.stochastic_amplitude > 0.0 ) {
            constexpr float w_sum = sum_weights<Weights...>();
            dw = w_sum * args.dev_ptrs.random_number[i] * CUDA::sqrt( ( args.p.R * args.dev_ptrs.reservoir_plus[i] + args.p.gamma_c ) / ( Type::real( 4.0 ) * args.p.dV ) ) / time.dt;
        }
    }
    if constexpr ( sizeof...( Weights ) == 1 ) {
        runge_add_to_input_k1<buffer_type, N, Weights...>( i, (N-1)*args.p.subgrid_N2_with_halo, dw, current_halo, time, args, input_output, k_vec );
    } else if constexpr ( sizeof...( Weights ) == 2 ) {
        runge_add_to_input_k2<buffer_type, N, Weights...>( i, dw, current_halo, time, args, input_output, k_vec );
    } else if constexpr ( sizeof...( Weights ) == 3 ) {
        runge_add_to_input_k3<buffer_type, N, Weights...>( i, dw, current_halo, time, args, input_output, k_vec );
    } else if constexpr ( sizeof...( Weights ) == 4 ) {
        runge_add_to_input_k4<buffer_type, N, Weights...>( i, dw, current_halo, time, args, input_output, k_vec );
    } else {
        runge_add_to_input_kw<buffer_type, N, Weights...>( i, dw, current_halo, time, args, input_output, k_vec );
    }
}


















template <int NMax, int N, float w, float... W>
PULSE_DEVICE PULSE_COMPILER_SPECIFIC void sum_single_error_k( int i, Type::complex& error, Type::complex* k_wavefunction, Type::uint32 offset ) {
    if constexpr ( w != 0.0 ) {
        error += w * k_wavefunction[i + offset * ( NMax - N )];
    }
    if constexpr ( sizeof...( W ) > 0 ) {
        sum_single_error_k<NMax, N - 1, W...>( i, error, k_wavefunction, offset );
    }
}




template <float... Weights>
PULSE_GLOBAL PULSE_COMPILER_SPECIFIC void runge_sum_to_error( int i, Type::uint32 current_halo, Solver::VKernelArguments time, Solver::KernelArguments args ) {
    GENERATE_SUBGRID_INDEX( i, current_halo );

    Type::complex error = 0.0;

    sum_single_error_k<sizeof...( Weights ), sizeof...( Weights ), Weights...>( i, error, args.dev_ptrs.k_wavefunction_plus, args.p.subgrid_N2_with_halo );

    args.dev_ptrs.rk_error[i] = CUDA::abs2( time.dt * error );
    if ( not args.p.use_twin_mode )
        return;

    error = 0.0;

    sum_single_error_k<sizeof...( Weights ), sizeof...( Weights ), Weights...>( i, error, args.dev_ptrs.k_wavefunction_minus, args.p.subgrid_N2_with_halo );

    args.dev_ptrs.rk_error[i] += args.p.i * CUDA::abs2( time.dt * error );
}
} // namespace PC3::Kernel::Summation