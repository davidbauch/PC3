#pragma once
#include "cuda_complex.cuh"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <map>
#include <vector>
#include "omp.h"
#include <algorithm>
#include <memory>

/**
 * @brief Very lightweight System Class containing all of the required system variables.
 * Reading the input from the commandline and filling the system variables is done
 * in the helperfunctions.hpp -> initializeSystem() function.
 */
class System {
   public:
    // SI Rescaling Units
    real_number m_e = 9.10938356E-31;
    real_number h_bar = 1.0545718E-34;
    real_number e_e = 1.60217662E-19;
    real_number h_bar_s = 6.582119514E-4;

    // System Variables
    real_number m_eff;
    real_number gamma_c = 0.15;          // ps^-1
    real_number gamma_r = 1.5 * gamma_c; // ps^-1
    real_number g_c = 3.E-6;            // meV mum^2
    real_number g_r = 2. * g_c;          // meV mum^2
    real_number R = 0.01;                // ps^-1 mum^2
    // real_number P0 = 100.;                     // ps^-1 mum^-2
    // real_number w = 10.;                       // mum
    real_number xmax = 100.;                   // mum
    real_number g_pm = -g_c / 5;               // meV mum^2
    real_number delta_LT = 0.025E-3; // meV
    real_number m_plus = 0.;
    real_number m_minus = 0.;

    // Numerics
    int s_N = 400;
    real_number dx;
    real_number t_max = 1000;
    bool normalize_phase_states = true;
    bool normalizePhasePulse = false;
    int iteration = 0;
    // RK Solver Variables
    real_number dt;
    real_number t;
    real_number dt_max = 0.3;
    real_number dt_min = 0.0001; // also dt_delta
    real_number tolerance = 1E-1;

    // FFT Mask Parameters
    real_number fft_every = 1; // ps
    real_number fft_power = 6.;
    real_number fft_mask_area = 0.7;

    // Kernel Block Size
    int block_size = 16;
    int omp_max_threads = omp_get_max_threads();

    // If this is true, the solver will use a fixed timestep RK4 method instead of the variable timestep RK45 method
    bool fixed_time_step = true;

    // Output of Variables
    std::vector<std::string> output_keys = {"mat","scalar"};

    // Pump arrays. These are highly deprecated as the pump is now cached
    std::vector<real_number> pump_amp;
    std::vector<real_number> pump_width;
    std::vector<real_number> pump_X;
    std::vector<real_number> pump_Y;
    std::vector<real_number> pump_exponent;
    std::vector<int> pump_pol;
    std::vector<int> pump_type;
    // TODO: pump m???

    // Pulse arrays
    std::vector<real_number> pulse_t0;
    std::vector<real_number> pulse_amp;
    std::vector<real_number> pulse_freq;
    std::vector<real_number> pulse_sigma;
    std::vector<int> pulse_m;
    std::vector<int> pulse_pol;
    std::vector<real_number> pulse_width;
    std::vector<real_number> pulse_X;
    std::vector<real_number> pulse_Y;

    class Envelope {
        public:
        std::vector<real_number> amp, width, x, y, exponent;
        std::vector<real_number> freq, sigma, t0;
        std::vector<int> m;
        
        enum class Type {
            Gauss,
            GaussOuter,
            Ring
        };
        std::vector<Type> type;

        enum class Polarization {
            Plus,
            Minus,
            Both
        };
        std::vector<Polarization> pol;

        enum class Behavior {
            Add,
            Multiply,
            Replace,
            Adaptive
        };
        std::vector<Behavior> behavior;

        void addSpacial(real_number amp, real_number width, real_number x, real_number y, real_number exponent, Type type, Polarization pol, Behavior behavior) {
            this->amp.push_back(amp);
            this->width.push_back(width);
            this->x.push_back(x);
            this->y.push_back(y);
            this->exponent.push_back(exponent);
            this->type.push_back(type);
            this->pol.push_back(pol);
            this->behavior.push_back(behavior);
        }
        void addTemporal(real_number t0, real_number sigma, real_number freq) {
            this->t0.push_back(t0);
            this->sigma.push_back(sigma);
            this->freq.push_back(freq);
        }

    };

    Envelope pulse, pump, mask, initial_state;
    bool normalize_before_masking = false;

    bool randomly_initialize_system = false;
    real_number random_system_amplitude = 1.0;

    template <typename... Args>
    bool doOutput( const Args&... args ) {
        return ( ( std::find( output_keys.begin(), output_keys.end(), args ) != output_keys.end() ) || ... );
    }
};

class Buffer {
   public:
    std::unique_ptr<complex_number[]> u_P_plus;
    std::unique_ptr<complex_number[]> u_P_minus;
    std::unique_ptr<complex_number[]> Psi_Plus;
    std::unique_ptr<complex_number[]> Psi_Minus;
    std::unique_ptr<complex_number[]> n_Plus;
    std::unique_ptr<complex_number[]> n_Minus;
    std::unique_ptr<complex_number[]> fft_plus;
    std::unique_ptr<complex_number[]> fft_minus;

    // Cache Arrays for max values of Psi
    std::vector<real_number> cache_Psi_Plus_max;
    std::vector<real_number> cache_Psi_Minus_max;

    // Cache Arrays for history (cut at Y = 0) of Psi
    std::vector<std::vector<complex_number>> cache_Psi_Plus_history;
    std::vector<std::vector<complex_number>> cache_Psi_Minus_history;

    Buffer( const int N ) {
        u_P_plus = std::make_unique<complex_number[]>(N * N);
        u_P_minus = std::make_unique<complex_number[]>(N * N);
        Psi_Plus = std::make_unique<complex_number[]>(N * N);
        Psi_Minus = std::make_unique<complex_number[]>(N * N);
        n_Plus = std::make_unique<complex_number[]>(N * N);
        n_Minus = std::make_unique<complex_number[]>(N * N);
        fft_plus = std::make_unique<complex_number[]>(N * N);
        fft_minus = std::make_unique<complex_number[]>(N * N);
    }
    Buffer( const System& s ) : Buffer( s.s_N ) {}
    Buffer(){};
};

class FileHandler {
   public:
    std::map<std::string, std::ofstream> files;
    std::string outputPath = "data";
    std::string loadPath = "";
    std::string outputName = "";
    std::string color_palette = "resources/vik.txt";
    std::string color_palette_phase = "resources/viko.txt";
    int out_modulo = 5;
    bool disableRender = false;

    FileHandler(){};
    FileHandler( FileHandler& other ) : files( std::move( other.files ) ),
                                        outputPath( other.outputPath ),
                                        outputName( other.outputName ),
                                        color_palette( other.color_palette ),
                                        out_modulo( other.out_modulo ),
                                        loadPath( other.loadPath ),
                                        disableRender( other.disableRender ){};

    std::string toPath( const std::string& name ) {
        return outputPath + ( outputPath.back() == '/' ? "" : "/" ) + outputName + ( outputName.empty() ? "" : "_" ) + name + ".txt";
    }

    std::ofstream& getFile( const std::string& name ) {
        if ( files.find( name ) == files.end() ) {
            files[name] = std::ofstream( toPath( name ) );
        }
        return files[name];
    }

    void loadMatrixFromFile( const std::string& filepath, complex_number* buffer ) {
        std::ifstream filein;
        filein.open( filepath, std::ios::in );
        std::istringstream inputstring;
        std::string line;
        int i = 0;
        real_number x, y, re, im;
        if ( filein.is_open() ) {
            while ( getline( filein, line ) ) {
                if ( line.size() > 2 ) {
                    inputstring = std::istringstream( line );
                    inputstring >> x >> y >> re >> im;
                    buffer[i] = { re, im };
                    i++;
                }
            }
            filein.close();
            std::cout << "Loaded " << i << " elements from " << filepath << std::endl;
        } else {
            std::cout << "Error: Couldn't load " << filepath << std::endl;
        }
    }

    void outputMatrixToFile( const complex_number* buffer, int row_start, int row_stop, int col_start, int col_stop, const System& s, std::ofstream& out, const std::string& name ) {
        if ( !out.is_open() )
            std::cout << "File " << name << " is not open!" << std::endl;
        for ( int i = row_start; i < row_stop; i++ ) {
            for ( int j = col_start; j < col_stop; j++ ) {
                auto index = j + i * s.s_N;
                auto x = -s.xmax + j * s.dx;
                auto y = -s.xmax + i * s.dx;
                out << x << " " << y << " " << std::setprecision( 10 ) << real( buffer[index] ) << " " << imag( buffer[index] ) << std::endl;
            }
            out << std::endl;
        }
        std::cout << "Output " << ( row_stop - row_start ) * ( col_stop - col_start ) << " elements to " << toPath( name ) << "." << std::endl;
    }
    void outputMatrixToFile( const complex_number* buffer, int row_start, int row_stop, int col_start, int col_stop, const System& s, const std::string& out ) {
        auto& file = getFile( out );
        outputMatrixToFile( buffer, row_start, row_stop, col_start, col_stop, s, file, out );
    }
    void outputMatrixToFile( const complex_number* buffer, const System& s, const std::string& out ) {
        auto& file = getFile( out );
        outputMatrixToFile( buffer, 0, s.s_N, 0, s.s_N, s, file, out );
    }
    void outputMatrixToFile( const complex_number* buffer, const System& s, std::ofstream& out, const std::string& name ) {
        outputMatrixToFile( buffer, 0, s.s_N, 0, s.s_N, s, out, name );
    }

    void outputMatrixToFile( const real_number* buffer, int row_start, int row_stop, int col_start, int col_stop, const System& s, std::ofstream& out, const std::string& name ) {
        if ( !out.is_open() )
            std::cout << "File " << name << " is not open!" << std::endl;
        for ( int i = row_start; i < row_stop; i++ ) {
            for ( int j = col_start; j < col_stop; j++ ) {
                auto index = j + i * s.s_N;
                auto x = -s.xmax + j * s.dx;
                auto y = -s.xmax + i * s.dx;
                out << x << " " << y << " " << std::setprecision( 10 ) << buffer[index] << std::endl;
            }
            out << std::endl;
        }
        std::cout << "Output " << ( row_stop - row_start ) * ( col_stop - col_start ) << " elements to " << toPath( name ) << "." << std::endl;
    }
    void outputMatrixToFile( const real_number* buffer, int row_start, int row_stop, int col_start, int col_stop, const System& s, const std::string& out ) {
        auto& file = getFile( out );
        outputMatrixToFile( buffer, row_start, row_stop, col_start, col_stop, s, file, out );
    }
    void outputMatrixToFile( const real_number* buffer, const System& s, const std::string& out ) {
        auto& file = getFile( out );
        outputMatrixToFile( buffer, 0, s.s_N, 0, s.s_N, s, file, out );
    }
    void outputMatrixToFile( const real_number* buffer, const System& s, std::ofstream& out, const std::string& name ) {
        outputMatrixToFile( buffer, 0, s.s_N, 0, s.s_N, s, out, name );
    }

    void outputMatrices( System& system, Buffer& buffer ) {
        std::vector<std::string> fileoutputkeys = { "Psi_plus", "Psi_minus", "n_plus", "n_minus", "fft_plus", "fft_minus" };
#pragma omp parallel for
        for ( int i = 0; i < fileoutputkeys.size(); i++ ) {
            auto key = fileoutputkeys[i];
            if ( key == "Psi_plus" and system.doOutput( "mat", "psi_plus", "plus", "psi" ) )
                outputMatrixToFile( buffer.Psi_Plus.get(), system, key );
            if ( key == "n_plus" and system.doOutput( "mat", "n_plus", "plus", "n" ) )
                outputMatrixToFile( buffer.n_Plus.get(), system, key );
            if ( key == "fft_plus" and system.doOutput( "mat", "fft_plus", "plus", "fft" ) )
                outputMatrixToFile( buffer.fft_plus.get(), system, key );
#ifdef TETMSPLITTING
            if ( key == "Psi_minus" and system.doOutput( "mat", "psi_minus", "minus", "psi" ) )
                outputMatrixToFile( buffer.Psi_Minus.get(), system, key );
            if ( key == "n_minus" and system.doOutput( "mat", "n_minus", "minus", "n" ) )
                outputMatrixToFile( buffer.n_Minus.get(), system, key );
            if ( key == "fft_minus" and system.doOutput( "mat", "fft_minus", "minus", "fft" ) )
                outputMatrixToFile( buffer.fft_minus.get(), system, key );
#endif
        }
    }

    void loadMatrices( System& system, Buffer& buffer ) {
        if ( loadPath.size() < 1 )
            return;
        std::cout << "Loading Matrices from " << loadPath << std::endl;
        if ( loadPath.back() != '/' )
            loadPath += "/";
        std::vector<std::string> fileoutputkeys = { "Psi_plus", "Psi_minus", "n_plus", "n_minus" };
#pragma omp parallel for
        for ( auto i = 0; i < fileoutputkeys.size(); i++ ) {
            if ( fileoutputkeys[i] == "Psi_plus" )
                loadMatrixFromFile( loadPath + fileoutputkeys[i] + ".txt", buffer.Psi_Plus.get() );
            else if ( fileoutputkeys[i] == "n_plus" )
                loadMatrixFromFile( loadPath + fileoutputkeys[i] + ".txt", buffer.n_Plus.get() );
#ifdef TETMSPLITTING
            else if ( fileoutputkeys[i] == "Psi_minus" )
                loadMatrixFromFile( loadPath + fileoutputkeys[i] + ".txt", buffer.Psi_Minus.get() );
            else if ( fileoutputkeys[i] == "n_minus" )
                loadMatrixFromFile( loadPath + fileoutputkeys[i] + ".txt", buffer.n_Minus.get() );
#endif
        }
    }

    void cacheToFiles( System& system, const Buffer& buffer ) {
        if ( system.doOutput( "max", "scalar" ) ) {
            auto& file_max = getFile( "max" );
            file_max << "t Psi_Plus";
#ifdef TETMSPLITTING
            file_max << " Psi_Minus";
#endif
            file_max << std::endl;
            for ( int i = 0; i < buffer.cache_Psi_Plus_max.size(); i++ ) {
#ifdef TETMSPLITTING
                file_max << i << " " << buffer.cache_Psi_Plus_max[i] << " " << buffer.cache_Psi_Minus_max[i] << std::endl;
#else
                file_max << i << " " << buffer.cache_Psi_Plus_max[i] << std::endl;
#endif
            }
            file_max.close();
        }

        if ( system.doOutput( "mat", "history" ) ) {
            auto& file_history_plus = getFile( "history_plus" );
#ifdef TETMSPLITTING
            auto& file_history_minus = getFile( "history_minus" );
#endif
            const auto interval_time = int( std::max( 1., buffer.cache_Psi_Plus_history.size() / 200. ) );
            const auto interval_x = int( std::max( 1., buffer.cache_Psi_Plus_history.front().size() / 200. ) );
            for ( int i = 0; i < buffer.cache_Psi_Plus_history.size(); i += interval_time ) {
                std::cout << "Writing history " << i << " of " << buffer.cache_Psi_Plus_max.size() << "\r";
                for ( int k = 0; k < buffer.cache_Psi_Plus_history.front().size(); k += interval_x ) {
                    const auto current_plus = buffer.cache_Psi_Plus_history[i][k];
                    file_history_plus << i << " " << k << " " << real( current_plus ) << " " << imag( current_plus ) << "\n";
#ifdef TETMSPLITTING
                    const auto current_minus = buffer.cache_Psi_Minus_history[i][k];
                    file_history_minus << i << " " << k << " " << real( current_minus ) << " " << imag( current_minus ) << "\n";
#endif
                }
                file_history_plus << "\n";
#ifdef TETMSPLITTING
                file_history_minus << "\n";
#endif
            }
            std::cout << "Writing history done, closing all files." << std::endl;
            file_history_plus.close();
#ifdef TETMSPLITTING
            file_history_minus.close();
#endif
        }
    }
};