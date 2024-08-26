#pragma once

#include <fstream>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <memory>
#include <bit>
#include "cuda/typedef.cuh"
#include "system/filehandler.hpp"
#include "system/envelope.hpp"

namespace PC3 {

/**
 * @brief Lightweight System Class containing all of the required system variables.
 * The System class reads the inputs from the commandline and saves them. The system class
 * also contains envelope wrappers for the different system variables and provides a few
 * helper functions called by the main loop cpu functions.
 */
class SystemParameters {
   public:

    // System Parameters. These are also passed to the Kernels
    struct KernelParameters {

        // Size Variables
        size_t N_x, N_y, N2;
        // Subgrid and Halo
        size_t halo_size;
        size_t subgrid_N_x, subgrid_N_y, subgrid_N2;
        size_t subgrids_x, subgrids_y; // For now, subgrids_x = subgrids_y at all times, even if N_x != N_y
        // Time variables
        Type::real t, dt;
        
        // SI Rescaling Units
        Type::real m_e, h_bar, e_e, h_bar_s, m_eff, m_eff_scaled;
        
        // System Variables
        Type::real L_x, L_y, dx, dy, dV, stochastic_amplitude, one_over_dx2, one_over_dy2, m2_over_dx2_p_dy2;
        Type::real gamma_c, gamma_r, g_c, g_r, R, g_pm, delta_LT;
        
        // Complex Scaled Values
        Type::real one_over_h_bar_s;
        Type::complex minus_i_over_h_bar_s, i_h_bar_s;
        Type::complex i = {0.0,1.0};
        Type::complex half_i = {0.0,0.5};
        Type::complex minus_half_i = {0.0,-0.5};
        Type::complex minus_i = {0.0,-1.0};

        // Boundary Conditions
        bool periodic_boundary_x, periodic_boundary_y;

        // Twin Mode
        bool use_twin_mode;

        ////////////////////////////////
        // Custom Parameters go here! //
        ////////////////////////////////
        // TODO: maybe (at compile time) do a "include custom_parameters.hpp" here. bad practice, but it works

    } kernel_parameters;
    // Truncated alias for the Kernel parameters so we can use p. instead of kernel_parameters.
    KernelParameters& p = kernel_parameters;

    // Numerics
    size_t iteration;
    bool disableRender;

    // RK Solver Variables
    Type::real t_max, dt_max, dt_min, tolerance, fft_every, random_system_amplitude, magic_timestep;

    // Kernel Block Size
    size_t block_size, omp_max_threads;

    // Initialize the system randomly
    bool randomly_initialize_system;
    
    // Periodic Boundary Conditions
    bool periodic_boundary_x, periodic_boundary_y;

    std::string iterator;

    // Seed for random number generator
    unsigned int random_seed;
    
    // History Output
    size_t history_matrix_start_x, history_matrix_start_y, history_matrix_end_x, history_matrix_end_y, history_matrix_output_increment;
    bool do_output_history_matrix;
    size_t output_history_matrix_every; // Scales outEvery for the history matrices with this value
    Type::real output_history_start_time; // Only output history matrices after this time
    Type::real output_every;

    bool do_overwrite_dt;

    // Imag Time Amp
    Type::real imag_time_amplitude;

    // Output of Variables
    std::vector<std::string> output_keys;

    // Envelope ReadIns
    PC3::Envelope pulse, pump, mask, initial_state, initial_reservoir, fft_mask, potential;

    FileHandler filehandler;

    // Default Constructor; Initializes all variables to their default values
    SystemParameters();
    // CMD Argument Constructor; Initializes passed variables according to the CMD arguments
    SystemParameters( int argc, char** argv );

    template <typename... Args>
    bool doOutput( const Args&... args ) {
        return ( ( std::find( output_keys.begin(), output_keys.end(), args ) != output_keys.end() ) || ... );
    }

    bool evaluateStochastic();

    void init( int argc, char** argv );
    void calculateAuto();
    void validateInputs();

    void printHelp();
    void printSummary( std::map<std::string, std::vector<double>> timeit_times, std::map<std::string, double> timeit_times_total );
    void printCMD( double complete_duration, double complete_iterations );
    void finishCMD();
};

} // namespace PC3