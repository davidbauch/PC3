#include <iomanip>

#include <filesystem>
#include "system/filehandler.hpp"
#include "misc/commandline_input.hpp"
#include "misc/escape_sequences.hpp"
#include "omp.h"

PC3::FileHandler::FileHandler() : 
    outputPath( "data" ),
    loadPath( "data" ),
    outputName( "" ),
    color_palette( "resources/vik.txt" ),
    color_palette_phase( "resources/viko.txt" ),
    disableRender( false ) {};

PC3::FileHandler::FileHandler( int argc, char** argv) : FileHandler() {
    init( argc, argv );
}

void PC3::FileHandler::init( int argc, char** argv ) {
    int index = 0;
    if ( ( index = findInArgv( "--path", argc, argv ) ) != -1 )
        outputPath = getNextStringInput( argv, "path", ++index );
    if ( outputPath.back() != '/' )
        outputPath += "/";

    if ( ( index = findInArgv( "--name", argc, argv ) ) != -1 )
        outputName = getNextStringInput( argv, "name", ++index );

    // Save Load Path if passed, else use output path as laod path
    loadPath = outputPath;
    if ( ( index = findInArgv( "--loadFrom", argc, argv ) ) != -1 )
        loadPath = getNextStringInput( argv, "loadFrom", ++index );

    // Colormap
    if ( ( index = findInArgv( "--cmap", argc, argv ) ) != -1 )
        color_palette = getNextStringInput( argv, "cmap", ++index );

    // We can also disable to SFML renderer by using the --nosfml flag.
    if ( findInArgv( "-nosfml", argc, argv ) != -1 )
        disableRender = true;
        
    // Creating output directory.
    try {
        std::filesystem::create_directories(outputPath);
        std::cout << "Successfully created directory " << outputPath << std::endl;
    } catch (std::filesystem::filesystem_error& e) {
        std::cout << EscapeSequence::RED << "Error creating directory " << outputPath << ": " << e.what() << EscapeSequence::RESET << std::endl;
    }

    // Create timeoutput subdirectory if --historyMatrix is passed.
    if ( findInArgv( "--historyMatrix", argc, argv ) != -1 ) {
        try {
            std::filesystem::create_directories(outputPath + "timeoutput");
            std::cout << "Successfully created sub directory " << outputPath + "timeoutput" << std::endl;
        } catch (std::filesystem::filesystem_error& e) {
            std::cout << EscapeSequence::RED << "Error creating directory " << outputPath + "timeoutput" << ": " << e.what() << EscapeSequence::RESET << std::endl;
        }
    }
}

std::string PC3::FileHandler::toPath( const std::string& name ) {
    return outputPath + ( outputPath.back() == '/' ? "" : "/" ) + outputName + ( outputName.empty() ? "" : "_" ) + name + ".txt";
}

std::ofstream& PC3::FileHandler::getFile( const std::string& name ) {
    if ( files.find( name ) == files.end() ) {
        files[name] = std::ofstream( toPath( name ) );
    }
    return files[name];
}

void PC3::FileHandler::loadMatrixFromFile( const std::string& filepath, complex_number* buffer ) {
    std::ifstream filein;
    filein.open( filepath, std::ios::in );
    std::istringstream inputstring;
    std::string line;
    int i = 0;
    real_number x, y, re, im;
    if ( filein.is_open() ) {
        while ( getline( filein, line ) ) {
            if ( line.size() > 3 ) {
                inputstring = std::istringstream( line );
                inputstring >> x >> y >> re >> im;
                buffer[i] = { re, im };
                i++;
            }
        }
        filein.close();
        std::cout << "Loaded " << i << " elements from " << filepath << std::endl;
    } else {
        #pragma omp critical
        std::cout << EscapeSequence::YELLOW << "Warning: Couldn't load " << filepath << EscapeSequence::RESET << std::endl;
    }
}

void PC3::FileHandler::loadMatrixFromFile( const std::string& filepath, real_number* buffer ) {
    std::ifstream filein;
    filein.open( filepath, std::ios::in );
    std::istringstream inputstring;
    std::string line;
    int i = 0;
    real_number x, y, val;
    if ( filein.is_open() ) {
        while ( getline( filein, line ) ) {
            if ( line.size() > 2 ) {
                inputstring = std::istringstream( line );
                inputstring >> x >> y >> val;
                buffer[i] = val;
                i++;
            }
        }
        filein.close();
        std::cout << "Loaded " << i << " elements from " << filepath << std::endl;
    } else {
        #pragma omp critical
        std::cout << EscapeSequence::YELLOW << "Warning: Couldn't load " << filepath << EscapeSequence::RESET << std::endl;
    }
}

void PC3::FileHandler::outputMatrixToFile( const complex_number* buffer, int row_start, int row_stop, int col_start, int col_stop, const unsigned int N, unsigned int increment, const real_number xmax, const real_number dx, std::ofstream& out, const std::string& name ) {
    if ( !out.is_open() )
        std::cout << "File " << name << " is not open!" << std::endl;
    for ( int i = row_start; i < row_stop; i+=increment ) {
        for ( int j = col_start; j < col_stop; j+=increment ) {
            auto index = j + i * N;
            auto x = -xmax + dx * i;
            auto y = -xmax + dx * j;
            out << x << " " << y << " " << CUDA::real( buffer[index] ) << " " << CUDA::imag( buffer[index] ) << "\n";
        }
        out << "\n";
    }
    out.flush();
    out.close();
    #pragma omp critical
    std::cout << "Output " << ( row_stop - row_start ) * ( col_stop - col_start ) / increment << " elements to " << toPath( name ) << "." << "\n";
}

void PC3::FileHandler::outputMatrixToFile( const complex_number* buffer, int row_start, int row_stop, int col_start, int col_stop, const unsigned int N, unsigned int increment, const real_number xmax, const real_number dx, const std::string& out ) {
    auto& file = getFile( out );
    outputMatrixToFile( buffer, row_start, row_stop, col_start, col_stop, N, increment, xmax, dx, file, out );
}
void PC3::FileHandler::outputMatrixToFile( const complex_number* buffer, const unsigned int N, const real_number xmax, const real_number dx, const std::string& out ) {
    auto& file = getFile( out );
    outputMatrixToFile( buffer, 0, N, 0, N, N, 1.0, xmax, dx, file, out );
}
void PC3::FileHandler::outputMatrixToFile( const complex_number* buffer, const unsigned int N, const real_number xmax, const real_number dx, std::ofstream& out, const std::string& name ) {
    outputMatrixToFile( buffer, 0, N, 0, N, N, 1.0, xmax, dx, out, name );
}

void PC3::FileHandler::outputMatrixToFile( const real_number* buffer, int row_start, int row_stop, int col_start, int col_stop, const unsigned int N, unsigned int increment, const real_number xmax, const real_number dx, std::ofstream& out, const std::string& name ) {
    if ( !out.is_open() )
        std::cout << "File " << name << " is not open!" << std::endl;
    for ( int i = row_start; i < row_stop; i+=increment ) {
        for ( int j = col_start; j < col_stop; j+=increment ) {
            auto index = j + i * N;
            auto x = -xmax + dx * i;
            auto y = -xmax + dx * j;
            out << x << " " << y << " " << buffer[index] << "\n";
        }
        out << "\n";
    }
    out.flush();
    out.close();
    #pragma omp critical
    std::cout << "Output " << ( row_stop - row_start ) * ( col_stop - col_start ) / increment << " elements to " << toPath( name ) << "." << std::endl;
}
void PC3::FileHandler::outputMatrixToFile( const real_number* buffer, int row_start, int row_stop, int col_start, int col_stop, const unsigned int N, unsigned int increment, const real_number xmax, const real_number dx, const std::string& out ) {
    auto& file = getFile( out );
    outputMatrixToFile( buffer, row_start, row_stop, col_start, col_stop, N, increment, xmax, dx, file, out );
}
void PC3::FileHandler::outputMatrixToFile( const real_number* buffer, const unsigned int N, const real_number xmax, const real_number dx, const std::string& out ) {
    auto& file = getFile( out );
    outputMatrixToFile( buffer, 0, N, 0, N, N, 1.0, xmax, dx, file, out );
}
void PC3::FileHandler::outputMatrixToFile( const real_number* buffer, const unsigned int N, const real_number xmax, const real_number dx, std::ofstream& out, const std::string& name ) {
    outputMatrixToFile( buffer, 0, N, 0, N, N, 1.0, xmax, dx, out, name );
}