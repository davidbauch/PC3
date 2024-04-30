#pragma once

#include <iostream>
#include <sstream>
#include <iomanip>
#include "solver/gpu_solver.hpp"
#include "misc/helperfunctions.hpp"

#ifdef SFML_RENDER
#    include <SFML/Graphics.hpp>
#    include <SFML/Window.hpp>
#    include "sfml_window.hpp"
#    include "colormap.hpp"
#endif

/*
 * The solver can be rendered using the SFML library, which is a very simple and easy to use library,
 * which we use to implement different methods to plot the solver in real time.
 * 
 * The Plus and Minus Components are always plotted seperately in different rows.
 * Default: Plot Psi and Reservoir with inset
 * The inset can be switched by pressing 'i' in the SFML window.
 * The inset then cycles between the FFT, logscale FFT, Quantum Noise
 * 
 * This plot "helper" is an abomination by itself and will probably be replaced by a more sophisticated
 * plotting library in the future such as QT.
 * 
*/

namespace PC3 {

std::string toScientific( const real_number in ) {
    std::stringstream ss;
    ss << std::scientific << std::setprecision( 2 ) << in;
    return ss.str();
}

#ifdef SFML_RENDER

BasicWindow& getWindow() {
    static BasicWindow window;
    return window;
}

std::unique_ptr<real_number[]> __plotarray;
template <typename T>
void plotMatrix( T* buffer, int NX, int NY, int posX, int posY, int skip, ColorPalette& cp, const std::string& title = "", bool plot_min_max = true ) {
    if (buffer == nullptr)
        return;
    PC3::CUDA::cwiseAbs2( buffer, __plotarray.get(), NX * NY );
    auto [min, max] = PC3::CUDA::minmax( __plotarray.get(), NX * NY );
    PC3::CUDA::normalize( __plotarray.get(), NX * NY, min, max );
    getWindow().blitMatrixPtr( __plotarray.get(), cp, NX, NY, posX, posY, 1 /*border*/, skip );
    if (!plot_min_max)
        return;
    NX = NX / skip;
    NY = NY / skip;
    auto text_height = getWindow().textheight / skip;
    getWindow().scaledPrint( posX + 5, posY + NY - text_height - 5, text_height, title + "Min: " + toScientific( sqrt(min) ) + " Max: " + toScientific( sqrt(max) ), sf::Color::White );
}

// Very bad practice, as this header can only be imported once without redefinitions.
// TODO: make this into a class like a normal friggin programmer.
ColorPalette __local_colorpalette_phase;
ColorPalette __local_colorpalette;

CheckBox* cb_toggle_fft;
CheckBox* cb_min_and_max;
Button* b_add_outevery;
Button* b_sub_outevery;
Button* b_add_dt;
Button* b_sub_dt;
Button* b_snapshot;
Button* b_reset_to_snapshot;
Button* b_reset_to_initial;
double snapshot_time = 0.0;

void initSFMLWindow( PC3::Solver& solver ) {
    if ( solver.system.filehandler.disableRender ) {
        std::cout << "Manually disabled SFML Renderer!" << std::endl;
        return;
    }
    if ( solver.system.use_twin_mode )
        getWindow().construct( 1920, 1080, solver.system.p.N_x * 3, solver.system.p.N_y * 2, "PULSE - TE/TM" );
    else
        getWindow().construct( 1920, 540, solver.system.p.N_x * 3, solver.system.p.N_y, "PULSE - Scalar" );

    // if .pal in __local_colorpalette, read gnuplot __local_colorpalette, else read as .txt
    if ( solver.system.filehandler.color_palette.find( ".pal" ) != std::string::npos ) {
        __local_colorpalette.readColorPaletteFromGnuplotDOTPAL( solver.system.filehandler.color_palette );
        __local_colorpalette_phase.readColorPaletteFromGnuplotDOTPAL( solver.system.filehandler.color_palette_phase );
    } else if ( solver.system.filehandler.color_palette.find( ".txt" ) != std::string::npos ) {
        __local_colorpalette.readColorPaletteFromTXT( solver.system.filehandler.color_palette );
        __local_colorpalette_phase.readColorPaletteFromTXT( solver.system.filehandler.color_palette_phase );
    } else {
        __local_colorpalette.readColorPaletteFromTXT( "resources/"+solver.system.filehandler.color_palette+".txt" );
        __local_colorpalette_phase.readColorPaletteFromTXT( "resources/"+solver.system.filehandler.color_palette_phase+".txt" );
    }
    __local_colorpalette.initColors();
    __local_colorpalette_phase.initColors();
    getWindow().init();
    __plotarray = std::make_unique<real_number[]>( solver.system.p.N_x * solver.system.p.N_y );

    cb_toggle_fft = new CheckBox( 10, 50, "Toggle FFT Plot", false );
    getWindow().addObject( cb_toggle_fft );
    cb_min_and_max = new CheckBox( 10, 80, "Toggle Min/Max", false );
    getWindow().addObject( cb_min_and_max );
    b_add_outevery = new Button( 10, 150, "Increase" );
    getWindow().addObject( b_add_outevery );
    b_sub_outevery = new Button( 10, 180, "Decrease" );
    getWindow().addObject( b_sub_outevery );
    b_add_dt = new Button( 10, 250, "Increase dt" );
    getWindow().addObject( b_add_dt );
    b_sub_dt = new Button( 10, 280, "Decrease dt" );
    getWindow().addObject( b_sub_dt );
    b_snapshot = new Button( 10, 380, "Snapshot" );
    getWindow().addObject( b_snapshot );
    b_reset_to_snapshot = new Button( 10, 410, "Reset to Snapshot" );
    getWindow().addObject( b_reset_to_snapshot );
    b_reset_to_initial = new Button( 10, 440, "Reset to Initial" );
    getWindow().addObject( b_reset_to_initial );
}

int __local_inset = 0;

bool plotSFMLWindow( PC3::Solver& solver, double simulation_time, double elapsed_time, size_t iterations ) {
    if ( solver.system.filehandler.disableRender )
        return true;
    bool running = getWindow().run();

    // Get Device arrays
    solver.syncDeviceArrays();

    complex_number* inset_plot_array_plus = nullptr, *inset_plot_array_minus = nullptr;
    if ( __local_inset == 1 ) {
        inset_plot_array_plus = solver.host.fft_plus.get();
        if ( solver.system.use_twin_mode )
            inset_plot_array_minus = solver.host.fft_minus.get();
    }
    if ( getWindow().keyPressed( BasicWindow::KEY_i ) ) {
        __local_inset = ( __local_inset + 1 ) % 2;
    }

    if (cb_toggle_fft->isChecked())
        __local_inset = 1;
    else
        __local_inset = 0;

    bool plot_min_max = cb_min_and_max->isChecked();
    // Plot Plus
    plotMatrix( solver.host.wavefunction_plus.get(), solver.system.p.N_x, solver.system.p.N_y /*size*/, solver.system.p.N_x, 0, 1, __local_colorpalette, "Psi+ ", plot_min_max );
    plotMatrix( inset_plot_array_plus, solver.system.p.N_x, solver.system.p.N_y /*size*/, solver.system.p.N_x, 0, 3, __local_colorpalette, "FFT+ ", plot_min_max );
    plotMatrix( solver.host.reservoir_plus.get(), solver.system.p.N_x, solver.system.p.N_y /*size*/, 2 * solver.system.p.N_x, 0, 1, __local_colorpalette, "n+ ", plot_min_max );
    PC3::CUDA::angle( solver.host.wavefunction_plus.get(), __plotarray.get(), solver.system.p.N_x * solver.system.p.N_y );
    plotMatrix( __plotarray.get(), solver.system.p.N_x, solver.system.p.N_y, 0, 0, 1, __local_colorpalette_phase, "ang(Psi+) ", plot_min_max );

    // Plot Minus
    if ( solver.system.use_twin_mode ) {
        plotMatrix( solver.host.wavefunction_minus.get(), solver.system.p.N_x, solver.system.p.N_y /*size*/, solver.system.p.N_x, solver.system.p.N_y, 1, __local_colorpalette, "Psi- ", plot_min_max );
        plotMatrix( inset_plot_array_minus, solver.system.p.N_x, solver.system.p.N_y /*size*/, solver.system.p.N_x, solver.system.p.N_y, 3, __local_colorpalette, "FFT- ", plot_min_max );
        plotMatrix( solver.host.reservoir_minus.get(), solver.system.p.N_x, solver.system.p.N_y /*size*/, 2 * solver.system.p.N_x, solver.system.p.N_y, 1, __local_colorpalette, "n- ", plot_min_max );
        PC3::CUDA::angle( solver.host.wavefunction_minus.get(), __plotarray.get(), solver.system.p.N_x * solver.system.p.N_y );
        plotMatrix( __plotarray.get(), solver.system.p.N_x, solver.system.p.N_y, 0, solver.system.p.N_y, 1, __local_colorpalette_phase, "ang(Psi-) ", plot_min_max );
    } 

    const auto ps_per_second = simulation_time / elapsed_time;
    const auto iterations_per_second = iterations / elapsed_time;

    // FPS and ps/s
    getWindow().print( 5, 5, "t = " + std::to_string( int( solver.system.p.t ) ) + ", FPS: " + std::to_string( int( getWindow().fps ) ) + ", ps/s: " + std::to_string( int( ps_per_second ) ) + ", it/s: " + std::to_string( int( iterations_per_second ) ), sf::Color::White );
 
    // If the mouse position is less than 200 on the x axis, draw the gui. else, set all gui components invisible
    if (getWindow().MouseX() < 200) {
        getWindow().drawRect( 0, 300, 0, getWindow().height, sf::Color(0,0,0,180), true );
        for (auto& obj : getWindow().objects) {
            obj->visible = true;
        }
        getWindow().print( 10, 120, "Out Every: " + std::to_string(solver.system.output_every) + "ps");
        getWindow().print( 10, 220, "dt: " + std::to_string(solver.system.p.dt) + "ps");

        // Draw Quick and dirty progressbar because why not.
        double progress = solver.system.p.t / solver.system.t_max;
        getWindow().drawRect( 10, 180, 310, 335, sf::Color( 50, 50, 50 ), true );
        getWindow().drawRect( 13, 13+164*progress, 313, 332, sf::Color( 36, 114, 234, 255 ), true );

    } else {
        for (auto& obj : getWindow().objects) {
            obj->visible = false;
        }
    }

    if (b_add_outevery->isToggled()) {
        if (solver.system.output_every == 0.0) 
            solver.system.output_every = solver.system.p.dt;
        solver.system.output_every *= 2;
    }
    if (b_sub_outevery->isToggled()) {
        solver.system.output_every /= 2;
    }

    if (b_add_dt->isToggled()) {
        solver.system.p.dt *= 1.1;
    }
    if (b_sub_dt->isToggled()) {
        solver.system.p.dt /= 1.1;
    }

    if (b_snapshot->isToggled()) {
        solver.device.wavefunction_plus.toHost( solver.host.snapshot_wavefunction_plus );
        solver.device.reservoir_plus.toHost( solver.host.snapshot_reservoir_plus );
        if ( solver.system.use_twin_mode ) {
            solver.device.wavefunction_minus.toHost( solver.host.snapshot_wavefunction_minus );
            solver.device.reservoir_minus.toHost( solver.host.snapshot_reservoir_minus );
        }
        snapshot_time = solver.system.p.t;
        
        std::cout << "Snapshot taken!" << std::endl;
    }

    if (b_reset_to_snapshot->isToggled()) {
        solver.device.wavefunction_plus.fromHost( solver.host.snapshot_wavefunction_plus );
        solver.device.reservoir_plus.fromHost( solver.host.snapshot_reservoir_plus );
        if ( solver.system.use_twin_mode ) {
            solver.device.wavefunction_minus.fromHost( solver.host.snapshot_wavefunction_minus );
            solver.device.reservoir_minus.fromHost( solver.host.snapshot_reservoir_minus );
        }
        solver.system.p.t = snapshot_time;
        std::cout << "Reset to Snapshot!" << std::endl;
    }

    if (b_reset_to_initial->isToggled()) {
        solver.device.wavefunction_plus.fromHost( solver.host.initial_state_plus );
        solver.device.reservoir_plus.fromHost( solver.host.initial_state_plus );
        if ( solver.system.use_twin_mode ) {
            solver.device.wavefunction_minus.fromHost( solver.host.initial_state_minus );
            solver.device.reservoir_minus.fromHost( solver.host.initial_state_minus );
        }
        solver.system.p.t = 0.0;
        std::cout << "Reset to Initial!" << std::endl;
    }


    getWindow().drawObjects();

    // Blit
    getWindow().flipscreen();
    return running;
}

#else
void initSFMLWindow( PC3::Solver& solver ){};
bool plotSFMLWindow( PC3::Solver& solver, double simulation_time, double elapsed_time, size_t iterationsd ) {
    return true;
};
#endif

} // namespace PC3