#ifndef OBJMSX_H
#define OBJMSX_H

#include "objgimp.h"

#include <cmath>        // std::abs
#include <vector>
#include <map>

#include "ColorSpace.h"
#include "Comparison.h"

#include "objdither.h"

/* --------------------- MSX PARAMETERS ------------------------ */

/* Number of pixels of a MSX attribute, both for width and height */ 
#define WIDTHATTR 8
#define HEIGHTATTR 1

/* Number of MSX colors */
#define MSXNCOLORS 16

/* Merge MSX colors */
#define MSXNGRUPOS 7

/* Merge color with Y difference below... */
#define MSXDMANUAIS 100

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
	{0,183,3,108},
	{41,209,75,144},
	{27,28,232,51},
	{82,72,253,96},
	{198,24,17,75},
	{3,231,243,164},
	{253,29,27,96},
	{255,99,75,143},
	{198,173,27,164},
	{222,190,85,188},
	{0,150,0,88},
	{185,37,164,96},
	{188,188,188,188},
	{255,255,255,255},
};

static int MSX_Groups[MSXNGRUPOS][3]={
	//Converted each color to LCH space then grouped them by hue
	//Will mix them inside each group
	//Will mix them between 2 neighbours color groups
	//Will mix no color group and each color group
	{ 1, 14, 15}, //No color
	{ 6,  8,  9}, //Red
	{10, 11,  0}, //Yellow
	{12,  2,  3}, //Green
	{ 7,  0,  0}, //Magenta
	{ 4,  5,  0}, //Blue
	{13,  0,  0}  //Cyan
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
