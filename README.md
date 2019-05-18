# MSXize

The GIMP plug-in for MSX-ize pictures. (c) Weber Estevan Roder Kai, 2019

Built up from the [ZX-ize plug-in](http://jafma.net/software/zxscreen/) by Juan-Antonio Fernández-Madrigal  
(c) Juan-Antonio Fernández-Madrigal, 2011  
"jafmag" (remove quotes) at gmail.

Built up from the [example blur plug-in](http://developer.gimp.org/writing-a-plug-in/1/index.html) by David Neary  

Built up from the [C++ ColorSpace Library](https://github.com/berendeanicolae/ColorSpace) by Berendea Nicolae  

To install the plug-in in the ~/.gimp-2.x/plug-ins , close The GIMP and write in console:
	
`CC=g++ LIBS="-lm" CFLAGS="-std=c++11 -O3 -omsxize" gimptool-2.0 --install "ColorSpace.cpp Comparison.cpp Conversion.cpp msxize.cpp"`
	
If there is any package missing (typically libgimp-dev), you will be warned at that call.

I've created the palette using values from VDP TMS9928 datasheet with R-Y = 0.80 in color 9 to avoid overflow.  
But the colors weren't like my childhood memories... In our TV we used to adjust brightness and contrast, so I did this as well...  
Added -0.25 to each RGB and multiplied to 1.34...  Well done!

For mixing colors, I've converted each color to LCH space then grouped them by hue.  
Then we will mix colors inside each group, between 2 neighbours color groups and will mix the no color group and each color group.

Parameters:  
MSXDMANUAIS is the maximum Y difference between 2 colors to merge them. Range: 0 - 255  
MSXTDITHER is the matriz size for ordered dithering. Values: 2, 4 or 8

For better results, in The GIMP always adjust black/white levels, brightness, contrast, saturation and resize image before running the plug-in.

Many thanks to Leandro Correia and Rogerio Penchel for inspiration.

This project contains a GNU general public license version 3.
