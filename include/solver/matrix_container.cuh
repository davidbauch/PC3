#pragma once
#include "cuda/typedef.cuh"
#include "cuda/cuda_matrix.cuh"

namespace PC3 {

/**
* DEFINE_MATRIX(type, name, size_scaling, index_for_initialization)
* type: The type of the matrix (Type::real, Type::complex, etc.)
* in_ptr_struct: If true, this matrix will be included in the pointer struct and can be accessed from the kernels: TODO
* name: The name of the matrix
* size_scaling: The scaling factor for the size of the matrix
* halo: Integer specifying the size of the halo. Most matrices should not have a halo, so halo = 0
* condition_for_construction: if false, neither the host nor the device matrices are constructed
* 
* We use this not-so-pretty macro to define matrices to shorten this file and to 
* make it easier for the user to add new matrices.
*/

// TODO: Alles was kein subgridding braucht sollte keine CUDAMatrix sondern n normaler device_vector sein!
// FFT, FFTMask, initial_, snapshot_ braucht kein subgridding
// TODO: Ki, pulse, pump, potential soll in std::vector, raw ptr in device vector.

#define MATRIX_LIST \
    DEFINE_MATRIX(Type::complex, false, initial_state_plus, 1, false) \
    DEFINE_MATRIX(Type::complex, false, initial_state_minus, 1, false) \
    DEFINE_MATRIX(Type::complex, false, initial_reservoir_plus, 1, false) \
    DEFINE_MATRIX(Type::complex, false, initial_reservoir_minus, 1, false) \
    DEFINE_MATRIX(Type::complex, true, wavefunction_plus, 1, true) \
    DEFINE_MATRIX(Type::complex, true, wavefunction_minus, 1, use_twin_mode) \
    DEFINE_MATRIX(Type::complex, true, reservoir_plus, 1, true) \
    DEFINE_MATRIX(Type::complex, true, reservoir_minus, 1, use_twin_mode) \
    DEFINE_MATRIX(Type::complex, true, pump_plus, n_pumps, true) \
    DEFINE_MATRIX(Type::complex, true, pump_minus, n_pumps, use_twin_mode) \
    DEFINE_MATRIX(Type::complex, true, pulse_plus, n_pulses, true) \
    DEFINE_MATRIX(Type::complex, true, pulse_minus, n_pulses, use_twin_mode) \
    DEFINE_MATRIX(Type::complex, true, potential_plus, n_potentials, true) \
    DEFINE_MATRIX(Type::complex, true, potential_minus, n_potentials, use_twin_mode) \
    DEFINE_MATRIX(Type::complex, true, buffer_wavefunction_plus, 1, true) \
    DEFINE_MATRIX(Type::complex, true, buffer_wavefunction_minus, 1, use_twin_mode) \
    DEFINE_MATRIX(Type::complex, true, buffer_reservoir_plus, 1, true) \
    DEFINE_MATRIX(Type::complex, true, buffer_reservoir_minus, 1, use_twin_mode) \
    DEFINE_MATRIX(Type::real,    true, fft_mask_plus, 1, use_fft) \
    DEFINE_MATRIX(Type::real,    true, fft_mask_minus, 1, use_fft and use_twin_mode) \
    DEFINE_MATRIX(Type::complex, true, fft_plus, 1, use_fft) \
    DEFINE_MATRIX(Type::complex, true, fft_minus, 1, use_fft and use_twin_mode) \
    DEFINE_MATRIX(Type::complex, true, k1_wavefunction_plus, 1, k_max >= 1) \
    DEFINE_MATRIX(Type::complex, true, k1_wavefunction_minus, 1, k_max >= 1 and use_twin_mode) \
    DEFINE_MATRIX(Type::complex, true, k1_reservoir_plus, 1, k_max >= 1) \
    DEFINE_MATRIX(Type::complex, true, k1_reservoir_minus, 1, k_max >= 1 and use_twin_mode) \
    DEFINE_MATRIX(Type::complex, true, k2_wavefunction_plus, 1, k_max >= 2) \
    DEFINE_MATRIX(Type::complex, true, k2_wavefunction_minus, 1, k_max >= 2 and use_twin_mode) \
    DEFINE_MATRIX(Type::complex, true, k2_reservoir_plus, 1, k_max >= 2) \
    DEFINE_MATRIX(Type::complex, true, k2_reservoir_minus, 1, k_max >= 2 and use_twin_mode) \
    DEFINE_MATRIX(Type::complex, true, k3_wavefunction_plus, 1, k_max >= 3) \
    DEFINE_MATRIX(Type::complex, true, k3_wavefunction_minus, 1, k_max >= 3 and use_twin_mode) \
    DEFINE_MATRIX(Type::complex, true, k3_reservoir_plus, 1, k_max >= 3) \
    DEFINE_MATRIX(Type::complex, true, k3_reservoir_minus, 1, k_max >= 3 and use_twin_mode) \
    DEFINE_MATRIX(Type::complex, true, k4_wavefunction_plus, 1, k_max >= 4) \
    DEFINE_MATRIX(Type::complex, true, k4_wavefunction_minus, 1, k_max >= 4 and use_twin_mode) \
    DEFINE_MATRIX(Type::complex, true, k4_reservoir_plus, 1, k_max >= 4) \
    DEFINE_MATRIX(Type::complex, true, k4_reservoir_minus, 1, k_max >= 4 and use_twin_mode) \
    DEFINE_MATRIX(Type::complex, true, k5_wavefunction_plus, 1, k_max >= 5) \
    DEFINE_MATRIX(Type::complex, true, k5_wavefunction_minus, 1, k_max >= 5 and use_twin_mode) \
    DEFINE_MATRIX(Type::complex, true, k5_reservoir_plus, 1, k_max >= 5) \
    DEFINE_MATRIX(Type::complex, true, k5_reservoir_minus, 1, k_max >= 5 and use_twin_mode) \
    DEFINE_MATRIX(Type::complex, true, k6_wavefunction_plus, 1, k_max >= 6) \
    DEFINE_MATRIX(Type::complex, true, k6_wavefunction_minus, 1, k_max >= 6 and use_twin_mode) \
    DEFINE_MATRIX(Type::complex, true, k6_reservoir_plus, 1, k_max >= 6) \
    DEFINE_MATRIX(Type::complex, true, k6_reservoir_minus, 1, k_max >= 6 and use_twin_mode) \
    DEFINE_MATRIX(Type::complex, true, k7_wavefunction_plus, 1, k_max >= 7) \
    DEFINE_MATRIX(Type::complex, true, k7_wavefunction_minus, 1, k_max >= 7 and use_twin_mode) \
    DEFINE_MATRIX(Type::complex, true, k7_reservoir_plus, 1, k_max >= 7) \
    DEFINE_MATRIX(Type::complex, true, k7_reservoir_minus, 1, k_max >= 7 and use_twin_mode) \
    DEFINE_MATRIX(Type::complex, true, rk_error, 1, k_max > 4) \
    DEFINE_MATRIX(Type::complex, true, random_number, 1, use_stochastic) \
    DEFINE_MATRIX(Type::cuda_random_state, true, random_state, 1, use_stochastic) \
    DEFINE_MATRIX(Type::complex, false, snapshot_wavefunction_plus, 1, false) \
    DEFINE_MATRIX(Type::complex, false, snapshot_wavefunction_minus, 1, false) \
    DEFINE_MATRIX(Type::complex, false, snapshot_reservoir_plus, 1, false) \
    DEFINE_MATRIX(Type::complex, false, snapshot_reservoir_minus, 1, false) // \ // <-- Don't forget the backslash!
    /////////////////////////////
    // Add your matrices here. //
    // Make sure to end each   //
    // but the last line with  //
    // a backslash!            //
    /////////////////////////////

struct MatrixContainer {

    // Cache triggers
    bool use_twin_mode, use_fft, use_stochastic;
    int k_max;

    // Declare all matrices using a macro
    #define DEFINE_MATRIX(type, ptrstruct, name, size_scaling, condition_for_construction) PC3::CUDAMatrix<type> name;
    MATRIX_LIST
    #undef X

    // Halo Map
    PC3::Type::device_vector<int> halo_map;

    // Empty Constructor
    MatrixContainer() = default;

    // Construction Chain. The Host Matrix is always constructed (who carese about RAM right?) and the device matrix is constructed if the condition is met.
    void constructAll( const int N_x, const int N_y, bool use_twin_mode, bool use_fft, bool use_stochastic, int k_max, const int n_pulses, const int n_pumps, const int n_potentials, const int subgrids, const int halo_size ) {
        this->use_twin_mode = use_twin_mode;
        this->k_max = k_max;
        this->use_fft = use_fft;
        this->use_stochastic = use_stochastic;
        #define DEFINE_MATRIX(type, ptrstruct, name, size_scaling, condition_for_construction) \
            name.constructHost( N_x, N_y * size_scaling, subgrids, halo_size, #name); \
            name.constructDevice( N_x, N_y * size_scaling, subgrids, halo_size, #name); 
        MATRIX_LIST
        #undef X
        // if (condition_for_construction) \
        // Construct the halo map. 6*total halo points because we have 6 coordinates for each point
        const Type::uint total_halo_points = (N_x+2*halo_size)*(N_y+2*halo_size) - N_x*N_y;
        halo_map = PC3::Type::device_vector<int>(total_halo_points*6);
     }

    struct Pointers {
        #define DEFINE_MATRIX(type, ptrstruct, name, size_scaling, condition_for_construction) \
                type* name;
        MATRIX_LIST
        #undef X

        // Halo Map
        int* halo_map;

        // Nullptr
        std::nullptr_t discard = nullptr;
    };

    Pointers pointers(const Type::uint subgrid) {
        Pointers ptrs;
        #define DEFINE_MATRIX(type, ptrstruct, name, size_scaling, condition_for_construction) \
                ptrs.name = name.getDevicePtr(subgrid);
        MATRIX_LIST
        #undef X

        ptrs.halo_map = GET_RAW_PTR( halo_map );
        
        return ptrs;
    }
};

} // namespace PC3