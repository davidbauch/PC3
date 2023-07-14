#pragma once

// #define SFML_RENDER
#include "system.hpp"
#include "helperfunctions.hpp"
#include "kernel.hpp"
#include "cuda_device_variables.cuh"

#ifdef SFML_RENDER
#    include <SFML/Graphics.hpp>
#    include <SFML/Window.hpp>
#    include "sfml_window.hpp"
#    include "colormap.hpp"

#    include <iostream>
#    include <sstream>
#    include <iomanip>

std::string toScientific( const double in ) {
    std::stringstream ss;
    ss << std::scientific << std::setprecision( 2 ) << in;
    return ss.str();
}

std::unique_ptr<double[]> plotarray;
template <typename T>
void plotMatrix( BasicWindow& window, T* buffer, int N, int posX, int posY, int skip, ColorPalette& cp, const std::string& title = "" ) {
    cwiseAbs2( buffer, plotarray.get(), N * N );
    auto [min, max] = minmax( plotarray.get(), N * N );
    normalize( plotarray.get(), N * N, min, max );
    window.blitMatrixPtr( plotarray.get(), cp, N, N, posX, posY, 1 /*border*/, skip );
    N = N / skip;
    auto text_height = N * 0.05;
    window.print( posX + 5, posY + N - text_height - 5, text_height, title + "Min: " + toScientific( min ) + " Max: " + toScientific( max ), sf::Color::White );
}

BasicWindow window;
ColorPalette colorpalette;

#endif

void initSFMLWindow( System& system, FileHandler& filehandler ) {
#ifdef SFML_RENDER
    if ( filehandler.disableRender ) {
        std::cout << "Manually disabled SFML Renderer!" << std::endl;
        return;
    }
    window.construct( 1920, 1080, system.s_N * 3, system.s_N * 2, "PC3" );
    // if .pal in colorpalette, read gnuplot colorpalette, else read as .txt
    if ( filehandler.colorPalette.find( ".pal" ) != std::string::npos )
        colorpalette.readColorPaletteFromGnuplotDOTPAL( filehandler.colorPalette );
    else
        colorpalette.readColorPaletteFromTXT( filehandler.colorPalette );
    colorpalette.initColors();
    window.init();
    plotarray = std::make_unique<double[]>( system.s_N * system.s_N );
#else
    std::cout << "PC^3 Compiled without SFML Renderer!" << std::endl;
#endif
}

bool plotSFMLWindow( System& system, FileHandler& handler, Buffer& buffer ) {
#ifdef SFML_RENDER
    if ( handler.disableRender )
        return true;
    bool running = window.run();
    getDeviceArrays( buffer.Psi_Plus, buffer.Psi_Minus, buffer.n_Plus, buffer.n_Minus, buffer.fft_plus, buffer.fft_minus, system.s_N );
    plotMatrix( window, buffer.Psi_Plus, system.s_N /*size*/, system.s_N, 0, 1, colorpalette, "Psi+ " );
    plotMatrix( window, buffer.fft_plus, system.s_N /*size*/, system.s_N, 0, 3, colorpalette, "FFT+ " );
    plotMatrix( window, buffer.Psi_Minus, system.s_N /*size*/, system.s_N, system.s_N, 1, colorpalette, "Psi- " );
    plotMatrix( window, buffer.fft_minus, system.s_N /*size*/, system.s_N, system.s_N, 3, colorpalette, "FFT- " );
    plotMatrix( window, buffer.n_Plus, system.s_N /*size*/, 2 * system.s_N, 0, 1, colorpalette, "n+ " );
    plotMatrix( window, buffer.n_Minus, system.s_N /*size*/, 2 * system.s_N, system.s_N, 1, colorpalette, "n- " );
    angle( buffer.Psi_Plus, plotarray.get(), system.s_N * system.s_N );
    plotMatrix( window, plotarray.get(), system.s_N, 0, 0, 1, colorpalette, "ang(Psi+) " );
    angle( buffer.Psi_Minus, plotarray.get(), system.s_N * system.s_N );
    plotMatrix( window, plotarray.get(), system.s_N, 0, system.s_N, 1, colorpalette, "ang(Psi-) " );
    window.flipscreen();
    return running;
#endif
    return true;
}