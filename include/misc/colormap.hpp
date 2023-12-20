// Import gnuplot color palettes
// Version 1.0.1, last change: ability to import multiple format styles

#pragma once
#include <vector>
#include <string>
             
class ColorPalette {
    public:
        class Color {
            public:
                int r,g,b;
                /* Constructor */
                Color(): r(0), g(0), b(0) {}
                Color(int r = 0, int g = 0, int b = 0): r(r), g(g), b(b) {}
        };

        std::vector<Color> input_colors;
        std::vector<Color> output_colors;

        // Convert inpurtcolors to this number of output colors by interpolation
        // The getColor() function does not need to inerpolate itself, thus saving time while using more memory
        int numberOfOutputColors = 5000;
        
        Color black = Color(0,0,0); // define as bg color!
        // add heading color (for max)

        // TODO: optional: return as tuple (with std::tie(r,g,b)) such that the Color class is not needed outside this class
        Color &getColor(double value, bool invert = false, bool cutoff = false);
        
        /*
            Read colormalette from TXT file. Colors can be either hex or r g b values
        */
        void readColorPaletteFromTXT(std::string filepath, int repetitions = 1);
        /* 
            Reads gnuplot colorpalette .pal, converst contained hex colors to r,g,b and saves them into input_colors array
        */ 
        void readColorPaletteFromGnuplotDOTPAL(std::string filepath, int repetitions = 1);
        
        double lerp(double v0, double v1, double t);

        void initColors();
};