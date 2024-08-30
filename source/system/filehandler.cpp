#include <iomanip>
#include <filesystem>
#include <vector>
#include <string>

#include "system/filehandler.hpp"
#include "misc/commandline_io.hpp"
#include "misc/escape_sequences.hpp"
#include "omp.h"

PC3::FileHandler::FileHandler() : outputPath( "data" ),
                                  outputName( "" ),
                                  color_palette( "vik" ),
                                  color_palette_phase( "viko" ){};

PC3::FileHandler::FileHandler( int argc, char** argv ) : FileHandler() {
    init( argc, argv );
}

void PC3::FileHandler::init( int argc, char** argv ) {
    int index = 0;
    if ( ( index = PC3::CLIO::findInArgv( "--path", argc, argv ) ) != -1 )
        outputPath = PC3::CLIO::getNextStringInput( argv, argc, "path", ++index );
    if ( outputPath.back() != '/' )
        outputPath += "/";

    if ( ( index = PC3::CLIO::findInArgv( "--name", argc, argv ) ) != -1 )
        outputName = PC3::CLIO::getNextStringInput( argv, argc, "name", ++index );

    // Colormap
    if ( ( index = PC3::CLIO::findInArgv( "--cmap", argc, argv ) ) != -1 ) {
        color_palette = PC3::CLIO::getNextStringInput( argv, argc, "cmap", ++index );
        color_palette_phase = PC3::CLIO::getNextStringInput( argv, argc, "cmap", index );
    }

    // Creating output directory.
    try {
        std::filesystem::create_directories( outputPath );
        std::cout << PC3::CLIO::prettyPrint( "Successfully created directory '" + outputPath + "'", PC3::CLIO::Control::Info) << std::endl;
    } catch ( std::filesystem::filesystem_error& e ) {
        std::cout << PC3::CLIO::prettyPrint(  "Error creating directory '" + outputPath + "'", PC3::CLIO::Control::FullError ) << std::endl;
    }

    // Create timeoutput subdirectory if --historyMatrix is passed.
    if ( PC3::CLIO::findInArgv( "--historyMatrix", argc, argv ) != -1 ) {
        try {
            std::filesystem::create_directories( outputPath + "timeoutput" );
            std::cout << PC3::CLIO::prettyPrint( "Successfully created sub-directory '" + outputPath + "timeoutput'", PC3::CLIO::Control::Info) << std::endl;
        } catch ( std::filesystem::filesystem_error& e ) {
            std::cout << PC3::CLIO::prettyPrint(  "Error creating directory '" + outputPath + "timeoutput'", PC3::CLIO::Control::FullError ) << std::endl;
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

bool PC3::FileHandler::loadMatrixFromFile( const std::string& filepath, Type::complex* buffer ) {
    std::ifstream filein;
    filein.open( filepath, std::ios::in );
    std::istringstream inputstring;
    std::string line;
    int i = 0;
    Type::real re, im;
    if ( not filein.is_open() ) {
#pragma omp critical
        std::cout << PC3::CLIO::prettyPrint( "Unable to load '" + filepath + "'", PC3::CLIO::Control::FullWarning ) << std::endl;
        return false;
    }
    // Header
    getline( filein, line );
    inputstring = std::istringstream( line );
    // Read SIZE Nx Ny sLx sLy dx dy
    Type::uint n_x, n_y;
    Type::real header;
    inputstring >> line >> line >> n_x >> n_y;
    Type::uint N = n_x * n_y;
    while ( getline( filein, line ) ) {
        inputstring = std::istringstream( line );
        // If the line is empty or starts with "#", skip it.
        if ( line.size() < 1 or line[0] == '#' )
            continue;
        if (i < N)
            while ( inputstring >> re ) {
                buffer[i] = Type::complex( Type::real(re), 0 );
                i++;
            }
        else
            while ( inputstring >> im ) {
                buffer[i - N] = Type::complex( CUDA::real( buffer[i - N] ), Type::real(im) );
                i++;
            }
    }
    filein.close();
    std::cout << PC3::CLIO::prettyPrint( "Loaded " + std::to_string(i) + " elements from '" + filepath + "'", PC3::CLIO::Control::Success) << std::endl;
    return true;
}

bool PC3::FileHandler::loadMatrixFromFile( const std::string& filepath, Type::real* buffer ) {
    std::ifstream filein;
    filein.open( filepath, std::ios::in );
    std::istringstream inputstring;
    std::string line;
    int i = 0;
    Type::real val;
    if ( not filein.is_open() ) {
#pragma omp critical
        std::cout << PC3::CLIO::prettyPrint(  "Unable to load '" + filepath + "'", PC3::CLIO::Control::FullWarning ) << std::endl;
        return false;
    }

    // Header
    getline( filein, line );
    inputstring = std::istringstream( line );
    // Read SIZE Nx Ny sLx sLy dx dy
    Type::uint n_x, n_y;
    Type::real header;
    inputstring >> line >> line >> n_x >> n_y;
    Type::uint N = n_x * n_y;
    while ( getline( filein, line ) ) {
        // If the line is empty or starts with "#", skip it.
        if ( line.size() < 1 or line[0] == '#' )
            continue;
        while ( inputstring >> val ) {
            buffer[i] = Type::real(val);
            i++;
        }
    }
    filein.close();
    std::cout << PC3::CLIO::prettyPrint( "Loaded " + std::to_string(i) + " elements from '" + filepath + "'", PC3::CLIO::Control::Success) << std::endl;
    return true;
}

void PC3::FileHandler::outputMatrixToFile( const Type::complex* buffer,Type::uint col_start, Type::uint col_stop, Type::uint row_start, Type::uint row_stop, const Type::uint N_x, const Type::uint N_y, Type::uint increment, const Header& header, std::ofstream& out, const std::string& name ) {
    if ( !out.is_open() ) {
        std::cout << PC3::CLIO::prettyPrint( "File '" + name + "' is not open! Cannot output matrix to file!", PC3::CLIO::Control::Error ) << std::endl;
        return;
    }
    // Header
    out << "# SIZE " << col_stop-col_start << " " << row_stop-row_start << " " << header << " :: PULSE MATRIX\n";
    std::stringstream output_buffer;
    // Real
    for ( int i = row_start; i < row_stop; i += increment ) {
        for ( int j = col_start; j < col_stop; j += increment ) {
            auto index = j + i * N_x;
            output_buffer << CUDA::real( buffer[index] ) << " ";
        }
        output_buffer << "\n";
    }
    // Imag
    for ( int i = row_start; i < row_stop; i += increment ) {
        for ( int j = col_start; j < col_stop; j += increment ) {
            auto index = j + i * N_x;
            output_buffer << CUDA::imag( buffer[index] ) << " ";
        }
        output_buffer << "\n";
    }
    out << output_buffer.str();
    out.flush();
    out.close();
#pragma omp critical
    std::cout << PC3::CLIO::prettyPrint( "Output " + std::to_string( ( row_stop - row_start ) * ( col_stop - col_start ) / increment ) + " elements to '" + toPath( name ) + "'.", PC3::CLIO::Control::Success ) << std::endl;
}

void PC3::FileHandler::outputMatrixToFile( const Type::complex* buffer,Type::uint col_start, Type::uint col_stop, Type::uint row_start, Type::uint row_stop, const Type::uint N_x, const Type::uint N_y, Type::uint increment, const Header& header, const std::string& out ) {
    auto& file = getFile( out );
    outputMatrixToFile( buffer, col_start, col_stop, row_start, row_stop, N_x, N_y, increment, header, file, out );
}
void PC3::FileHandler::outputMatrixToFile( const Type::complex* buffer, const Type::uint N_x, const Type::uint N_y, const Header& header, const std::string& out ) {
    auto& file = getFile( out );
    outputMatrixToFile( buffer, 0, N_x, 0, N_y, N_x, N_y, 1.0, header, file, out );
}
void PC3::FileHandler::outputMatrixToFile( const Type::complex* buffer, const Type::uint N_x, const Type::uint N_y, const Header& header, std::ofstream& out, const std::string& name ) {
    outputMatrixToFile( buffer, 0, N_x, 0, N_y, N_x, N_y, 1.0, header, out, name );
}

void PC3::FileHandler::outputMatrixToFile( const Type::real* buffer, Type::uint col_start, Type::uint col_stop, Type::uint row_start, Type::uint row_stop, const Type::uint N_x, const Type::uint N_y, Type::uint increment, const Header& header, std::ofstream& out, const std::string& name ) {
    if ( !out.is_open() ) {
        std::cout << PC3::CLIO::prettyPrint( "File '" + name + "' is not open! Cannot output matrix to file!", PC3::CLIO::Control::Error ) << std::endl;
        return;
    }
    // Header
    out << "# SIZE " << col_stop-col_start << " " << row_stop-row_start << " " << header << " :: PULSE MATRIX\n";
    std::stringstream output_buffer;
    // Real
    for ( int i = row_start; i < row_stop; i += increment ) {
        for ( int j = col_start; j < col_stop; j += increment ) {
            auto index = j + i * N_x;
            output_buffer << buffer[index] << " ";
        }
        output_buffer << "\n";
    }
    out << output_buffer.str();
    out.flush();
    out.close();
#pragma omp critical
    std::cout << PC3::CLIO::prettyPrint( "Output " + std::to_string( ( row_stop - row_start ) * ( col_stop - col_start ) / increment ) + " elements to '" + toPath( name ) + "'.", PC3::CLIO::Control::Success ) << std::endl;
}
void PC3::FileHandler::outputMatrixToFile( const Type::real* buffer, Type::uint col_start, Type::uint col_stop, Type::uint row_start, Type::uint row_stop, const Type::uint N_x, const Type::uint N_y, Type::uint increment, const Header& header, const std::string& out ) {
    auto& file = getFile( out );
    outputMatrixToFile( buffer, col_start, col_stop, row_start, row_stop, N_x, N_y, increment, header, file, out );
}
void PC3::FileHandler::outputMatrixToFile( const Type::real* buffer, const Type::uint N_x, const Type::uint N_y, const Header& header, const std::string& out ) {
    auto& file = getFile( out );
    outputMatrixToFile( buffer, 0, N_x, 0, N_y, N_x, N_y, 1.0, header, file, out );
}
void PC3::FileHandler::outputMatrixToFile( const Type::real* buffer, const Type::uint N_x, const Type::uint N_y, const Header& header, std::ofstream& out, const std::string& name ) {
    outputMatrixToFile( buffer, 0, N_x, 0, N_y, N_x, N_y, 1.0, header, out, name );
}

std::vector<std::vector<PC3::Type::real>> PC3::FileHandler::loadListFromFile( const std::string& path, const std::string& name ) {
    std::vector<std::vector<Type::real>> data;
    std::ifstream filein;
    filein.open( path, std::ios::in );
    std::istringstream inputstring;
    std::string line;
    Type::real val;
    if ( not filein.is_open() ) { 
        std::cout << PC3::CLIO::prettyPrint( "Unable to load '" + path + "' for purpose '" + name + "'", PC3::CLIO::Control::FullWarning ) << std::endl;
        return data;
    }
    while ( getline( filein, line ) ) {
        // If the line is empty or starts with "#", skip it.
        if ( line.size() < 1 or line[0] == '#' )
            continue;
        inputstring = std::istringstream( line );
        Type::uint col = 0;
        while ( inputstring >> val ) {
            if ( data.size() <= col )
                data.push_back( std::vector<Type::real>() );
            data[col].push_back( val );
            col++;
        }
    }
    filein.close();
    std::cout << PC3::CLIO::prettyPrint( "Loaded " + std::to_string( data.size() ) + " columns from '" + path + "' for purpose: '" + name + "'", PC3::CLIO::Control::Success) << std::endl;
    return data;
}

void PC3::FileHandler::outputListToFile( const std::string& path, std::vector<std::vector<Type::real>>& data, const std::string& name ) {
    std::ofstream fileout;
    fileout.open( path, std::ios::out );
    if ( not fileout.is_open() ) {
        std::cout << PC3::CLIO::prettyPrint( "Unable to open '" + path + "' for purpose '" + name + "' for writing!", PC3::CLIO::Control::Error ) << std::endl;
        return;
    }
    for ( Type::uint i = 0; i < data[0].size(); i++ ) {
        for ( Type::uint j = 0; j < data.size(); j++ ) {
            if ( j >= data.size() )
                fileout << "NaN ";
                continue;
            fileout << std::setprecision( 10 ) << data[j][i] << " ";
        }
        fileout << "\n";
    }
    fileout.flush();
    fileout.close();
    std::cout << PC3::CLIO::prettyPrint( "Output " + std::to_string( data[0].size() ) + " columns to '" + path + "' - '" + name + "'", PC3::CLIO::Control::Success) << std::endl;
}