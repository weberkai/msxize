#ifndef OBJMSX_H
#define OBJMSX_H

#include "objgimp.h"

#include <cmath>        // std::abs
#include <vector>
#include <map>

#include "ColorSpace.h"
#include "Comparison.h"

#include "objdither.h"
#include <iostream>

/* --------------------- MSX PARAMETERS ------------------------ */

/* Number of pixels of a MSX attribute, both for width and height */ 
#define WIDTHATTR 8
#define HEIGHTATTR 1

/* Number of MSX colors */
#define MSXNCOLORS 16

/* Merge MSX colors */
#define MSXNGRUPOS 7

/* Merge color with Y difference below... */
#define MSXDMANUAIS 128

/* Ordered dither matrix size */
#define MSXTDITHER 8

/* For debugging (undefine if not) */
/* #define DEBUGGING */

static int MSX_Colors1[MSXNCOLORS][4]={ /* R,G,B for each MSX color (MSX order) */
	//I've created the palette using values from VDP TMS9928 datasheet with R-Y = 0.80 in color 9 to avoid overflow.
	//But the colors weren't like my childhood memories...
	//In our TV we used to adjust brightness and contrast, so I did this as well...
	//Added -0.25 to each RGB and multiplied to 1.34...
	//Well done!
	{0,0,0,0},
	{0,0,0,0},
	{0,182,3,107},
	{41,208,75,143},
	{27,28,231,51},
	{82,72,252,95},
	{197,24,17,75},
	{3,229,241,163},
	{252,29,27,95},
	{255,99,75,143},
	{197,172,27,163},
	{221,189,85,187},
	{0,150,0,88},
	{184,37,163,95},
	{187,187,187,187},
	{255,255,255,255}
};

static int MSX_Groups[MSXNGRUPOS][10]={
	//Converted each color to LCH space then grouped them by hue
	//Will mix them inside each group
	//Will mix them between 2 neighbours color groups
	//Will mix no color group and each color group
	{ 1, 14, 15}, //No color
	{ 6,  8,  9}, //Red
	{10, 11,  0}, //Yellow
	{12,  2,  3}, //Green
	{ 7,  0,  0}, //Cyan
	{ 4,  5,  0}, //Blue
	{13,  0,  0}  //Magenta
};

struct CorPossivel {
  double R;
  double G;
  double B;
  int cor1;
  int cor2;
};

struct InfoCores {
  int quantidade;
  std::vector< std::pair< int, int > > pontos;
  double R;
  double G;
  double B;
};

class BufferMSX {
	public:
		BufferMSX(int width, int height);
		void setMSX(int x, int y, int cormsx);
		void getRGB(int x, int y, double &R, double &G, double &B);
	private:
		int width, height;
		std::vector< std::vector< int > > buffer;
};


class Bloco {
	public:
		Bloco(int width, int height, double limit);
		void setORIG(int x, int y, double R, double G, double B, double pos);
		void ProcessaBloco(std::vector< CorPossivel > &corespossiveis);
		void getMSX(int x, int y, int &cormsx);
	private:
		int width, height;
		double limit;
		std::vector< std::vector< std::vector< double > > > bufferORIG;
		std::vector< std::vector< std::vector< double > > > bufferNOVO;
		std::vector< std::vector< int > > bufferMSXORIG;
		std::vector< std::vector< int > > bufferMSXNOVO;
		void AchaCoresMaisProximas(bool comdither);
		void ContaCores(std::map< int, InfoCores > &mapacores);
		void CopiaNovo();
		void AchaCoresMixar(std::map< int, InfoCores > &mapacores);
		void MixaCores(std::map< int, InfoCores > &mapacores);
		void CopiaNovoMSX();
		std::vector< CorPossivel > *corespossiveis;
};

class MSXConvert : public ObjSpecific {
	public:
		void Convert(BufferRGB &bufferin, BufferRGB &bufferout, ObjGimp &objgimp);
	private:
		int chunkV, chunksV, chunkH, chunksH;
		std::vector< CorPossivel > corespossiveis;
		void PreencheCoresPossiveis();
		void ArmazenaCor(int cor1, int cor2);
};

#endif // OBJMSX_H
