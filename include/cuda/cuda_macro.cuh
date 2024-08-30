#pragma once

#include "cuda/typedef.cuh"

// Helper Macro to iterate a specific RK K
#define CALCULATE_K( index, input_wavefunction, input_reservoir )                                                                                                                                                                             \
    {                                                                                                                                                                                                                                         \
        const Type::uint current_halo = system.p.halo_size - index;                                                                                                                                                                               \
        auto current_grid = getGridSize( system.p.subgrid_N_x + 2 * current_halo, system.p.subgrid_N_y + 2 * current_halo );                                                                                                                  \
        dim3 current_block = {system.block_size, 1, 1};\
        CALL_KERNEL(                                                                                                                                                                                                                          \
            RUNGE_FUNCTION_GP, "K" #index, current_grid, current_block, stream,                                                                                                                                                           \
            current_halo, { system.p.t, system.p.dt }, kernel_arguments,                                                                                                                                                                          \
            { kernel_arguments.dev_ptrs.input_wavefunction##_plus, kernel_arguments.dev_ptrs.input_wavefunction##_minus, kernel_arguments.dev_ptrs.input_reservoir##_plus, kernel_arguments.dev_ptrs.input_reservoir##_minus,                 \
              kernel_arguments.dev_ptrs.k##index##_wavefunction_plus, kernel_arguments.dev_ptrs.k##index##_wavefunction_minus, kernel_arguments.dev_ptrs.k##index##_reservoir_plus, kernel_arguments.dev_ptrs.k##index##_reservoir_minus } ); \
    };

#define INTERMEDIATE_SUM_K( index, ... )                                                                                                                                                                                                                                                                                                                                                                                                                             \
    {                                                                                                                                                                                                                                                                                                                                                                                                                                                                \
        const Type::uint current_halo = system.p.halo_size - index;                                                                                                                                                                                                                                                                                                                                                                                                      \
        auto current_grid = getGridSize( system.p.subgrid_N_x + 2 * current_halo, system.p.subgrid_N_y + 2 * current_halo );                                                                                                                                                                                                                                                                                                                                         \
        dim3 current_block = {system.block_size, 1, 1};\
        CALL_KERNEL(                                                                                                                                                                                                                                                                                                                                                                                                                                                 \
            Kernel::RK::runge_sum_to_input_kw, "Sum for K" #index, current_grid, current_block, stream,                                                                                                                                                                                                                                                                                                                                                          \
            current_halo, { system.p.t, system.p.dt }, kernel_arguments, { kernel_arguments.dev_ptrs.wavefunction_plus, kernel_arguments.dev_ptrs.wavefunction_minus, kernel_arguments.dev_ptrs.reservoir_plus, kernel_arguments.dev_ptrs.reservoir_minus, kernel_arguments.dev_ptrs.buffer_wavefunction_plus, kernel_arguments.dev_ptrs.buffer_wavefunction_minus, kernel_arguments.dev_ptrs.buffer_reservoir_plus, kernel_arguments.dev_ptrs.buffer_reservoir_minus }, \
            { __VA_ARGS__ } );                                                                                                                                                                                                                                                                                                                                                                                                                                       \
    };

#define FINAL_SUM_K( ... )                                                                                                                                                                                                                                                                                                                                                                                                    \
    {                                                                                                                                                                                                                                                                                                                                                                                                                         \
        auto current_grid = getGridSize( system.p.subgrid_N_x, system.p.subgrid_N_y );                                                                                                                                                                                                                                                                                                  \
        dim3 current_block = {system.block_size, 1, 1};\
        CALL_KERNEL(                                                                                                                                                                                                                                                                                                                                                                                                          \
            Kernel::RK::runge_sum_to_input_kw, "Sum for Psi", current_grid, current_block, stream,                                                                                                                                                                                                                                                                                                                        \
            0, { system.p.t, system.p.dt }, kernel_arguments, { kernel_arguments.dev_ptrs.wavefunction_plus, kernel_arguments.dev_ptrs.wavefunction_minus, kernel_arguments.dev_ptrs.reservoir_plus, kernel_arguments.dev_ptrs.reservoir_minus, kernel_arguments.dev_ptrs.wavefunction_plus, kernel_arguments.dev_ptrs.wavefunction_minus, kernel_arguments.dev_ptrs.reservoir_plus, kernel_arguments.dev_ptrs.reservoir_minus }, \
            { __VA_ARGS__ } );                                                                                                                                                                                                                                                                                                                                                                                                \
    };

#ifdef USE_CUDA
// Execudes a CUDA Command, checks for the latest error and prints it
// This is technically not a requirement, but usually good practice
#    define CHECK_CUDA_ERROR( func, msg )                             \
        {                                                             \
            func;                                                     \
            cudaError_t err = cudaGetLastError();                     \
            if ( err != cudaSuccess ) {                               \
                printf( "%s: %s\n", msg, cudaGetErrorString( err ) ); \
            }                                                         \
        }

// Calls a Kernel and also checks for errors.
// The Kernel call requires a name and a grid and block size that
// are not further passed to the actual compute Kernel. Instead, they
// are used as launch parameters and for debugging.
#    define CALL_KERNEL( func, name, grid, block, stream, ... ) \
        {                                                       \
            func<<<grid, block, 0, stream>>>( 0, __VA_ARGS__ ); \
        }
// Wraps the successive calls to the CUDA Kernels into a single CUDA Graph.
//    std::vector<KernelArguments> kernel_arguments; \
//            for (Type::uint subgrid = 0; subgrid < system.p.subgrids_x * system.p.subgrids_y; subgrid++) { \
//                kernel_arguments.push_back( updateKernelArguments(subgrid) ); \
//            }

#    define SOLVER_SEQUENCE( with_graph, content )                                                                                                        \
        {                                                                                                                                                 \
            static bool cuda_graph_created = false;                                                                                                       \
            static cudaGraph_t graph;                                                                                                                     \
            static cudaGraphExec_t instance;                                                                                                              \
            static cudaStream_t stream;                                                                                                                   \
            std::vector<Solver::KernelArguments> v_kernel_arguments;                                                                                      \
            for (Type::uint subgrid = 0; subgrid < system.p.subgrids_x*system.p.subgrids_y; subgrid++) {                                                        \
                v_kernel_arguments.push_back( generateKernelArguments(subgrid) );                                                                         \
            }                                                                                                                                             \
            if ( !cuda_graph_created or not with_graph ) {                                                                                                \
                if ( with_graph ) {                                                                                                                       \
                    cudaStreamCreate( &stream );                                                                                                          \
                    cudaStreamBeginCapture( stream, cudaStreamCaptureModeGlobal );                                                                        \
                    std::cout << PC3::CLIO::prettyPrint( "Capturing CUDA Graph", PC3::CLIO::Control::Secondary | PC3::CLIO::Control::Info ) << std::endl; \
                }                                                                                                                                         \
                for ( Type::uint subgrid = 0; subgrid < system.p.subgrids_x * system.p.subgrids_y; subgrid++ ) {                                              \
                    auto& kernel_arguments = v_kernel_arguments[subgrid];                                                                                 \
                    content;                                                                                                                              \
                }                                                                                                                                         \
                auto current_grid = getGridSize( system.p.subgrid_N_x, system.p.subgrid_N_y );                                                            \
                dim3 current_block = {system.block_size, 1, 1};                                                                                           \
                CALL_KERNEL(                                                                                                                              \
                    Kernel::Halo::synchronize_halos, "Synchronization", current_grid, current_block, stream,                                              \
                    system.p.subgrids_x, system.p.subgrids_y, system.p.subgrid_N_x, system.p.subgrid_N_y, system.p.halo_size, matrix.halo_map.size(),     \
                    system.p.periodic_boundary_x, system.p.periodic_boundary_y, matrix.wavefunction_plus.getSubgridDevicePtrs(),                          \
                    v_kernel_arguments[0].dev_ptrs.halo_map                                                                                                    \
                )                                                                                                                                         \
                if ( with_graph ) {                                                                                                                       \
                    cudaStreamEndCapture( stream, &graph );                                                                                               \
                    cudaGraphInstantiate( &instance, graph, NULL, NULL, 0 );                                                                              \
                    cuda_graph_created = true;                                                                                                            \
                }                                                                                                                                         \
            } else {                                                                                                                                      \
                CHECK_CUDA_ERROR( cudaGraphLaunch( instance, stream ), "graph launch" );                                                                  \
            }                                                                                                                                             \
        }

#else
// On the CPU, the check for CUDA errors does nothing
#    define CHECK_CUDA_ERROR( func, msg )
// On the CPU, the Kernel call does not execute a parallel GPU Kernel. Instead,
// it launches a group of threads using a #pragma omp instruction. This executes
// the Kernel in parallel on the CPU.
// TODO: this is not correct anymore
#    include <functional>
#    define CALL_KERNEL( func, name, grid, block, stream, ... )                                                                                                                                                                                 \
        {                                                                                                                                                                                                                                                      \
            _Pragma( "omp parallel for schedule(static)" )                                                                                                                                                               \
            for ( Type::uint i = 0; i < block.x; ++i ) {                                                                                                                                                                               \
                for ( Type::uint j = 0; j < grid.x; ++j ) {                                                                                                                                                                           \
                    const Type::uint index = i * grid.x + j;                                                                                                                                                                          \
                    func( index, __VA_ARGS__ );                                                                                                                                                                                   \
                }                                                                                                                                                                                                                               \
            }                                                                                                                                                                                                                                   \
        }
// Merges the Kernel calls into a single function call. This is not required on the CPU.
#    define SOLVER_SEQUENCE( with_graph, content ) \
        {                              \
           for ( Type::uint subgrid = 0; subgrid < system.p.subgrids_x * system.p.subgrids_y; subgrid++ ) { \
                auto kernel_arguments = generateKernelArguments( subgrid ); \
                content; \
            } \
        }
#endif

// Swaps symbols a and b
#define swap_symbol( a, b ) \
    {                       \
        auto tmp = a;       \
        a = b;              \
        b = tmp;            \
    }

// CUDA Specific Alloc and Free
#ifndef USE_CPU
#    define DEVICE_ALLOC( ptr, size, name )                             \
        {                                                               \
            CHECK_CUDA_ERROR( cudaMalloc( (void**)&ptr, size ), name ); \
        }
#    define MEMCOPY_TO_DEVICE( dst, src, size, name )                                       \
        {                                                                                   \
            CHECK_CUDA_ERROR( cudaMemcpy( dst, src, size, cudaMemcpyHostToDevice ), name ); \
        }
#    define MEMCOPY_FROM_DEVICE( dst, src, size, name )                                     \
        {                                                                                   \
            CHECK_CUDA_ERROR( cudaMemcpy( dst, src, size, cudaMemcpyDeviceToHost ), name ); \
        }
#    define SYMBOL_TO_DEVICE( dest, source, size, name )                        \
        {                                                                       \
            CHECK_CUDA_ERROR( cudaMemcpyToSymbol( dest, source, size ), name ); \
        }
#    define SYMBOL_TO_HOST( dest, source, size, name )                            \
        {                                                                         \
            CHECK_CUDA_ERROR( cudaMemcpyFromSymbol( dest, source, size ), name ); \
        }
#    define DEVICE_FREE( ptr, name )                   \
        {                                              \
            CHECK_CUDA_ERROR( cudaFree( ptr ), name ); \
        }
#else
#    define DEVICE_ALLOC( ptr, size, name )        \
        {                                          \
            ptr = (decltype( ptr ))malloc( size ); \
        }
#    define MEMCOPY_TO_DEVICE( dst, src, size, name ) \
        {                                             \
            memcpy( dst, src, size );                 \
        }
#    define MEMCOPY_FROM_DEVICE( dst, src, size, name ) \
        {                                               \
            memcpy( dst, src, size );                   \
        }
#    define SYMBOL_TO_DEVICE( dest, source, size, name ) \
        {                                                \
            dest = *( source );                          \
        }
#    define SYMBOL_TO_HOST( dest, source, size, name ) \
        {                                              \
            dest = *( source );                        \
        }
#    define DEVICE_FREE( ptr, name ) \
        {                            \
            free( ptr );             \
        }
#endif

// Helper to retrieve the raw device pointer. When using nvcc and thrust, we need a raw pointer cast.
#ifdef USE_CPU
#    define GET_RAW_PTR( vec ) vec.data()
#else
#    define GET_RAW_PTR( vec ) thrust::raw_pointer_cast( vec.data() )
#endif