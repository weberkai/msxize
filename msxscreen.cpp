/*
	The Gimp plug-in for msx-ize pictures.
	(c) Weber Estevan Roder Kai, 2019

	Built up from the zx-ize plug-in by Juan-Antonio Fernández-Madrigal
	(http://jafma.net/software/zxscreen/)
	(c) Juan-Antonio Fernández-Madrigal, 2011
	"jafmag" (remove quotes) at gmail.

	Built up from the example blur plug-in by David Neary 
	(http://developer.gimp.org/writing-a-plug-in/1/index.html)

	Built up from the C++ ColorSpace Library by Berendea Nicolae
	(https://github.com/berendeanicolae/ColorSpace)

	To install the plug-in in the ~/.gimp-2.x/plug-ins , close The Gimp and write in console:
	
	CC=g++ LIBS="-lm" CFLAGS="-std=c++11" gimptool-2.0 --install "msxscreen.cpp ColorSpace.cpp Comparison.cpp Conversion.cpp"
	
	If there is any package missing (typically libgimp-dev), you will be warned at that call.
*/

#include <libgimp/gimp.h>
#include <cmath>        // std::abs
#include <vector>
#include <map>
#include <functional>

#include "ColorSpace.h"
#include "Comparison.h"
#include "Conversion.h"
#include "Utils.h"

/* --------------------- PROGRAM ROUTINES ------------------------ */

static void query(void);
static void run(const gchar *name, gint nparams, const GimpParam *param, gint *nreturn_vals, GimpParam **return_vals);
static void msx_ize(GimpDrawable *drawable);

/* --------------------- PROGRAM VARIABLES ------------------------ */

GimpPlugInInfo PLUG_IN_INFO={
	NULL,
	NULL,
	query,
	run
};
								 
/* --------------------- PROGRAM CODE ------------------------ */

MAIN()

static void query(void) {
	static GimpParamDef args[]={
		{GIMP_PDB_INT32, (char*) "run-mode", (char*) "Run mode"},
		{GIMP_PDB_IMAGE, (char*) "image", (char*) "Input image"},
		{GIMP_PDB_DRAWABLE, (char*) "drawable", (char*) "Input drawable"}
	};

	gimp_install_procedure(
		"plug-in-msxscreen",
		"MSX Screen",
		"Converts the image to the MSX appearance",
		"Weber Estevan Roder Kai",
		"Copyright Weber Estevan Roder Kai",
		"2019",
		"_MSX Screen",
		"RGB*",
		GIMP_PLUGIN,
		G_N_ELEMENTS(args),
		0,
		args,
		NULL
	);
	gimp_plugin_menu_register(
		"plug-in-msxscreen",
		"<Image>/Filters/MSX"
	);
}

static void run(const gchar *name, gint nparams, const GimpParam *param, gint *nreturn_vals, GimpParam **return_vals) {
	static GimpParam values[1];
	GimpRunMode run_mode;
	GimpDrawable *drawable;

	/* Getting run_mode */
	run_mode=(GimpRunMode) param[0].data.d_int32;

	/*  Get the specified drawable  */
	drawable=gimp_drawable_get(param[2].data.d_drawable);

	/* Set status text */
	gimp_progress_init("MSXScreen...");
	
	/* Do the work */
	msx_ize(drawable);

	/* Unset status text */
	gimp_progress_end();

	/* Setting mandatory output values */
	*nreturn_vals=1;
	*return_vals=values;
	values[0].type=GIMP_PDB_STATUS;
	values[0].data.d_status=GIMP_PDB_SUCCESS;

	/* Dump the result back */
	gimp_displays_flush();
	gimp_drawable_detach(drawable);
}

/* --------------------- MSX CODE ------------------------ */

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

/* threshold maps */
#define map(x) (x)
static const int map8[8*8] = {
map( 0), map(48), map(12), map(60), map( 3), map(51), map(15), map(63),
map(32), map(16), map(44), map(28), map(35), map(19), map(47), map(31),
map( 8), map(56), map( 4), map(52), map(11), map(59), map( 7), map(55),
map(40), map(24), map(36), map(20), map(43), map(27), map(39), map(23),
map( 2), map(50), map(14), map(62), map( 1), map(49), map(13), map(61),
map(34), map(18), map(46), map(30), map(33), map(17), map(45), map(29),
map(10), map(58), map( 6), map(54), map( 9), map(57), map( 5), map(53),
map(42), map(26), map(38), map(22), map(41), map(25), map(37), map(21)
};

static const int map4[4*4] = {
map( 0), map( 8), map( 2), map(10),
map(12), map( 4), map(14), map( 6),
map( 3), map(11), map( 1), map( 9),
map(15), map( 7), map(13), map( 5)
};

static const int map2[2*2] = {
map( 0), map( 2),
map( 3), map( 1)
};
#undef map

static int MSX_Colors1[MSXNCOLORS][4]={ /* R,G,B for each MSX color (MSX order) */
	//I've created the palette using values from VDP TMS9928 datasheet with R-Y = 0.80 in color 9 to avoid overflow.
	//But the colors weren't like my childhood memories...
	//In our TV we used to adjust brightness and contrast, so I did this as well...
	//Added -0.25 to each RGB and multiplied to 1.34...
	//Well done!
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

class BufferRGB {
	public:
		BufferRGB(int x, int y);
		void setRGB(int x, int y, double R, double G, double B);
		void getRGB(int x, int y, double &R, double &G, double &B);
		void getSize(int &width, int &height);
		void getLAB(int x, int y, double &L, double &A, double &B);
	private:
		int width, height;
		std::vector< std::vector< std::vector< double > > > buffer;
};

BufferRGB::BufferRGB(int width, int height) {
	this->width = width;
	this->height = height;
	// Set up sizes. (HEIGHT x WIDTH)
	buffer.resize(this->width);
	for (int x = 0; x < this->width; x++) {
		buffer[x].resize(this->height);
		for (int y = 0; y < this->height; y++) {
			buffer[x][y].resize(3);
		}
	}
}

void BufferRGB::setRGB(int x, int y, double R, double G, double B) {
	buffer[x][y][0] = R;
	buffer[x][y][1] = G;
	buffer[x][y][2] = B;
}

void BufferRGB::getRGB(int x, int y, double &R, double &G, double &B) {
	R = buffer[x][y][0];
	G = buffer[x][y][1];
	B = buffer[x][y][2];
}

void BufferRGB::getSize(int &width, int &height) {
	width = this->width;
	height = this->height;
}

void BufferRGB::getLAB(int x, int y, double &L, double &A, double &B) {
	ColorSpace::Rgb input(buffer[x][y][0], buffer[x][y][1], buffer[x][y][2]);
	ColorSpace::Lab output;
	input.To<ColorSpace::Lab>(&output);
	L = output.l;
	A = output.a;
	B = output.b;
}

class BufferMSX {
	public:
		BufferMSX(int width, int height);
		void setMSX(int x, int y, int cormsx);
		void getRGB(int x, int y, double &R, double &G, double &B);
		void getLAB(int x, int y, double &L, double &A, double &B);
	private:
		int width, height;
		std::vector< std::vector< int > > buffer;
};

BufferMSX::BufferMSX(int width, int height) {
	this->width = width;
	this->height = height;
	// Set up sizes. (HEIGHT x WIDTH)
	buffer.resize(this->width);
	for (int x = 0; x < this->width; x++) {
		buffer[x].resize(height);
	}
}

void BufferMSX::setMSX(int x, int y, int cormsx) {
	buffer[x][y] = cormsx;
}

void BufferMSX::getRGB(int x, int y, double &R, double &G, double &B) {
	R = (double) MSX_Colors1[buffer[x][y]][0];
	G = (double) MSX_Colors1[buffer[x][y]][1];
	B = (double) MSX_Colors1[buffer[x][y]][2];
}

void BufferMSX::getLAB(int x, int y, double &L, double &A, double &B) {
	ColorSpace::Rgb input((double) MSX_Colors1[buffer[x][y]][0], (double) MSX_Colors1[buffer[x][y]][1], (double) MSX_Colors1[buffer[x][y]][2]);
	ColorSpace::Lab output;
	input.To<ColorSpace::Lab>(&output);
	L = output.l;
	A = output.a;
	B = output.b;
}

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

Bloco::Bloco(int width, int height, double limit) {
	this->width = width;
	this->height = height;
	this->limit = limit;
	// Set up sizes. (HEIGHT x WIDTH)
	bufferORIG.resize(this->width);
	bufferNOVO.resize(this->width);
	bufferMSXORIG.resize(this->width);
	bufferMSXNOVO.resize(this->width);
	for (int x = 0; x < this->width; x++) {
		bufferORIG[x].resize(this->height);
		bufferNOVO[x].resize(this->height);
		bufferMSXORIG[x].resize(this->height);
		bufferMSXNOVO[x].resize(this->height);
		for (int y = 0; y < this->height; y++) {
			bufferORIG[x][y].resize(4);
			bufferNOVO[x][y].resize(4);
		}
	}
}

void Bloco::setORIG(int x, int y, double R, double G, double B, double pos) {
	bufferORIG[x][y][0] = R;
	bufferORIG[x][y][1] = G;
	bufferORIG[x][y][2] = B;
	bufferORIG[x][y][3] = pos;
}

void Bloco::ProcessaBloco(std::vector< CorPossivel > &corespossiveis) {
	this->corespossiveis = &corespossiveis;
	std::map< int, InfoCores > mapacores;
	CopiaNovo();
	AchaCoresMaisProximas(false);
	ContaCores(mapacores);
	bool refaz = mapacores.size() >= 3;
	while (refaz) {
		AchaCoresMixar(mapacores);
		MixaCores(mapacores);
		AchaCoresMaisProximas(false);
		ContaCores(mapacores);
		refaz = mapacores.size() >= 3;
	}
	CopiaNovoMSX();
	//Tenta dither
	AchaCoresMaisProximas(true);
	ContaCores(mapacores);
	if (mapacores.size() <= 2) {
		CopiaNovoMSX();
	}
}

void Bloco::AchaCoresMaisProximas(bool comdither) {
	for (int x = 0; x < this->width; x++) {
		for (int y = 0; y < this->height; y++) {
			int cor1, cor2;
			double proporcao;
			double R, G, B, pos;
			R = bufferNOVO[x][y][0];
			G = bufferNOVO[x][y][1];
			B = bufferNOVO[x][y][2];
			pos = bufferNOVO[x][y][3];
			
			ColorSpace::Rgb c(R, G, B);
			bool primeiracor = true;
			double distmenor;
			
			if (comdither) {
				for (std::vector<CorPossivel>::iterator coratual = (*corespossiveis).begin() ; coratual != (*corespossiveis).end(); coratual++) {
					ColorSpace::Rgb d((*coratual).R, (*coratual).G, (*coratual).B);
					double distatual = ColorSpace::Cie2000Comparison::Compare(&c, &d);
					bool trocacor = false;
					if (primeiracor) {
						trocacor = true;
						primeiracor = false;
					} else if (distatual < distmenor) {
						trocacor = true;
					}
					if (trocacor) {
						distmenor = distatual;
						cor1 = (*coratual).cor1;
						cor2 = (*coratual).cor2;
						
						//Calcula proporcao
						double NY1;
						NY1 = MSX_Colors1[cor1][3];

						double NY2;
						NY2 = MSX_Colors1[cor2][3];
					
						if (NY1 == NY2) { //in case of 0 length line
							proporcao = 0.5;
						} else {
							double NY = 0.299 * R + 0.587 * G + 0.114 * B;
							proporcao = (NY - NY1) / (NY2 - NY1);
							if (proporcao <= 0.0) {
								proporcao = 0.0;
							}
							else if (proporcao >= 1.0) {
								proporcao = 1.0;
							}
						}
					};
				}
				
				int cormsx = (proporcao * this->limit) < pos ? cor1 : cor2;
				bufferMSXORIG[x][y] = cormsx;
				
			} else {
				for (int coratual = 1; coratual < MSXNCOLORS; coratual++) {
					ColorSpace::Rgb d(MSX_Colors1[coratual][0], MSX_Colors1[coratual][1], MSX_Colors1[coratual][2]);
					double distatual = ColorSpace::Cie2000Comparison::Compare(&c, &d);
					bool trocacor = false;
					if (primeiracor) {
						trocacor = true;
						primeiracor = false;
					} else if (distatual < distmenor) {
						trocacor = true;
					}
					if (trocacor) {
						distmenor = distatual;
						cor1 = coratual;
					};
				}
				
				bufferMSXORIG[x][y] = cor1;
			}
			
		}
	}
}

void Bloco::getMSX(int x, int y, int &cormsx) {
	cormsx = bufferMSXNOVO[x][y];
}

void Bloco::ContaCores(std::map< int, InfoCores > &mapacores) {
	mapacores.clear();
	for (int x = 0; x < this->width; x++) {
		for (int y = 0; y < this->height; y++) {
			std::pair< int, int > ponto = std::make_pair(x, y);
			int cormsx = bufferMSXORIG[x][y];
			
			// Searching element in std::map by key.
			std::map< int, InfoCores >::iterator coratual = mapacores.find(cormsx);
			
			if(coratual == mapacores.end()) {
				InfoCores  infocor;
				infocor.quantidade = 1;
				infocor.pontos.push_back(ponto);
				mapacores.insert(std::make_pair(cormsx, infocor));
			} else {
				(*coratual).second.quantidade++;
				(*coratual).second.pontos.push_back(ponto);
			}
		}
	}
}

void Bloco::CopiaNovo() {
	for (int x = 0; x < this->width; x++) {
		for (int y = 0; y < this->height; y++) {
			bufferNOVO[x][y][0] = bufferORIG[x][y][0];
			bufferNOVO[x][y][1] = bufferORIG[x][y][1];
			bufferNOVO[x][y][2] = bufferORIG[x][y][2];
			bufferNOVO[x][y][3] = bufferORIG[x][y][3];
		}
	}
}

void Bloco::AchaCoresMixar(std::map< int, InfoCores > &mapacores) {
	double R, G, B;
	for (std::map< int, InfoCores >::iterator grupoatual = mapacores.begin() ; grupoatual != mapacores.end(); grupoatual++) {
		R = 0.0;
		G = 0.0;
		B = 0.0;
		
		//Mixa cores
		for (std::vector< std::pair< int, int > >::iterator pontoatual = (*grupoatual).second.pontos.begin() ; pontoatual != (*grupoatual).second.pontos.end(); pontoatual++) {
			R = R + pow(bufferORIG[(*pontoatual).first][(*pontoatual).second][0], 2.0);
			G = G + pow(bufferORIG[(*pontoatual).first][(*pontoatual).second][1], 2.0);
			B = B + pow(bufferORIG[(*pontoatual).first][(*pontoatual).second][2], 2.0);
		}
		
		R = sqrt(R / (*grupoatual).second.quantidade);
		G = sqrt(G / (*grupoatual).second.quantidade);
		B = sqrt(B / (*grupoatual).second.quantidade);
		
		(*grupoatual).second.R = R;
		(*grupoatual).second.G = G;
		(*grupoatual).second.B = B;
	}
	
	//Calcula distancia
	double R1, G1, B1, R2, G2, B2;
	bool primeiracor = true;
	double distmenor;
	std::map< int, InfoCores >::iterator grupo1;
	std::map< int, InfoCores >::iterator grupo2;
	
	std::map< int, InfoCores >::iterator grupoatual1 = mapacores.begin();
	std::map< int, InfoCores >::iterator grupoatual2 = grupoatual1;
	grupoatual2++;
	bool acabou = false;

	while (!acabou) {
		R1 = (*grupoatual1).second.R;
		G1 = (*grupoatual1).second.G;
		B1 = (*grupoatual1).second.B;
		R2 = (*grupoatual2).second.R;
		G2 = (*grupoatual2).second.G;
		B2 = (*grupoatual2).second.B;
		
		ColorSpace::Rgb c(R1, G1, B1);
		ColorSpace::Rgb d(R2, G2, B2);
		double distatual = ColorSpace::Cie2000Comparison::Compare(&c, &d);
		
		bool trocacor = false;
		if (primeiracor) {
			trocacor = true;
			primeiracor = false;
		} else if (distatual < distmenor) {
			trocacor = true;
		}
		if (trocacor) {
			distmenor = distatual;
			grupo1 = grupoatual1;
			grupo2 = grupoatual2;
		};

		grupoatual2++;
		//Passei do fim
		if (grupoatual2 == mapacores.end()) {
			//Próximo grupo
			grupoatual1++;
			grupoatual2 = grupoatual1;
			grupoatual2++;
			//É o último
			if (grupoatual2 == mapacores.end()) {
				acabou = true;
			}
		}
	}
	(*grupo1).second.pontos.insert( (*grupo1).second.pontos.end(), (*grupo2).second.pontos.begin(), (*grupo2).second.pontos.end() );
	(*grupo1).second.quantidade = (*grupo1).second.quantidade + (*grupo2).second.quantidade;
	int chave = (*grupo2).first;
	mapacores.erase(chave);
}

void Bloco::MixaCores(std::map< int, InfoCores > &mapacores) {
	double R, G, B;
	for (std::map< int, InfoCores >::iterator grupoatual = mapacores.begin() ; grupoatual != mapacores.end(); grupoatual++) {
		R = 0.0;
		G = 0.0;
		B = 0.0;
		
		//Mixa cor
		for (std::vector< std::pair< int, int > >::iterator pontoatual = (*grupoatual).second.pontos.begin() ; pontoatual != (*grupoatual).second.pontos.end(); pontoatual++) {
			R = R + pow(bufferORIG[(*pontoatual).first][(*pontoatual).second][0], 2.0);
			G = G + pow(bufferORIG[(*pontoatual).first][(*pontoatual).second][1], 2.0);
			B = B + pow(bufferORIG[(*pontoatual).first][(*pontoatual).second][2], 2.0);
		}
		
		R = sqrt(R / (*grupoatual).second.quantidade);
		G = sqrt(G / (*grupoatual).second.quantidade);
		B = sqrt(B / (*grupoatual).second.quantidade);
			
		//Copia cor
		for (std::vector< std::pair< int, int > >::iterator pontoatual = (*grupoatual).second.pontos.begin() ; pontoatual != (*grupoatual).second.pontos.end(); pontoatual++) {
			bufferNOVO[(*pontoatual).first][(*pontoatual).second][0] = R;
			bufferNOVO[(*pontoatual).first][(*pontoatual).second][1] = G;
			bufferNOVO[(*pontoatual).first][(*pontoatual).second][2] = B;
		}
	}
}

void Bloco::CopiaNovoMSX() {
	for (int x = 0; x < this->width; x++) {
		for (int y = 0; y < this->height; y++) {
			bufferMSXNOVO[x][y] = bufferMSXORIG[x][y];
		}
	}
}

class MSXConvert {
	public:
		MSXConvert();
		void Convert(BufferRGB &bufferin, BufferRGB &bufferout, std::function<void(double)> AtualizaProgresso);
	private:
		int chunkV, chunksV, chunkH, chunksH;
		std::vector< CorPossivel > corespossiveis;
		void PreencheCoresPossiveis();
		void ArmazenaCor(int cor1, int cor2);
};

MSXConvert::MSXConvert() {
	PreencheCoresPossiveis();
}

void MSXConvert::Convert(BufferRGB &bufferin, BufferRGB &bufferout, std::function<void(double)> AtualizaProgresso) {
	int progresso = 0;
	int width, height;
	bufferin.getSize(width, height);
	
	//Cria msx
	BufferMSX buffermsx(width, height);

	chunksH = width / WIDTHATTR + 1;
	chunksV = height / HEIGHTATTR + 1;

	chunkV = 0;
	bool acabouV = false;
	//Para cada grupo vertical
	while (not acabouV) {
		chunkH = 0;
		bool acabouH = false;
		//Para cada grupo horizontal
		while (not acabouH) {
			
			Bloco bloco(WIDTHATTR, HEIGHTATTR, (double) MSXTDITHER * MSXTDITHER);
			//Preenche Bloco
			for (int y = 0; y <= (HEIGHTATTR - 1); y++) {
				int j = chunkV * HEIGHTATTR + y;
				for (int x = 0; x <= (WIDTHATTR - 1); x++) {
					int i = chunkH * WIDTHATTR + x;
					if ((i < width) && (j < height)) {
						double R, G, B;
						bufferin.getRGB(i, j, R, G, B);
						int map_value;
						if (MSXTDITHER == 2) {
							map_value = map2[((j % 2) * 2) + (i % 2)];
						} else if (MSXTDITHER == 4) {
							map_value = map4[((j % 4) * 4) + (i % 4)];
						} else if (MSXTDITHER == 8) {
							map_value = map8[((j % 8) * 8) + (i % 8)];
						}
						bloco.setORIG(x, y, R, G, B, map_value);
					}
				}
			}
			
			//Acha cores com dither
			bloco.ProcessaBloco(corespossiveis);
			
			//Preenche MSX
			for (int y = 0; y <= (HEIGHTATTR - 1); y++) {
				int j = chunkV * HEIGHTATTR + y;
				for (int x = 0; x <= (WIDTHATTR - 1); x++) {
					int i = chunkH * WIDTHATTR + x;
					if ((i < width) && (j < height)) {
						int cormsx;
						bloco.getMSX(x, y, cormsx);
						buffermsx.setMSX(i, j, cormsx);
					}
				}
			}

			//Atualiza Progresso
			progresso++;
			AtualizaProgresso((double)progresso / (double)(chunksV * chunksH));
			
			//atualiza coluna
			chunkH++;
			if (chunkH >= chunksH) {
				acabouH = true;
			};
		};
		//atualiza linha
		chunkV++;
		if (chunkV >= chunksV) {
			acabouV = true;
		};
	};

	//Copia buffermsx em bufferout
	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			double R, G, B;
			buffermsx.getRGB(i, j, R, G, B);
			bufferout.setRGB(i, j, R, G, B);
		}
	}	
}

void MSXConvert::PreencheCoresPossiveis() {
	//Processa grupos
	for (int grupoatual1 = 0; grupoatual1 < MSXNGRUPOS; grupoatual1++) {
		for (int grupoatual2 = 0; grupoatual2 < MSXNGRUPOS; grupoatual2++) {
			bool processar = false;
			//Dentro do grupo
			if (grupoatual1 == grupoatual2) {
				processar = true;
			}
			//Sem cores x cores
			else
			if (grupoatual1 == 0) {
				processar = true;
			}
			//Entre grupos
			else
			if ((grupoatual2 == (grupoatual1 +1)) || (grupoatual1 == MSXNGRUPOS) &&  (grupoatual2 == 1)) {
				processar = true;
			}
			if (processar) {
				int coratual1 = 0;
				int coratual2 = 0;
				bool temnogrupo = true;
				while (temnogrupo) {
					int cor1 = MSX_Groups[grupoatual1][coratual1];
					int cor2 = MSX_Groups[grupoatual2][coratual2];
					if ((cor1 != 0) && (cor2 != 0)) {
						int NY1 = MSX_Colors1[cor1][3];
						int NY2 = MSX_Colors1[cor2][3];
						if (std::abs(NY2 - NY1) <= MSXDMANUAIS) {
							ArmazenaCor(cor1, cor2);
						}
					}
					bool podeavancar1 = true;
					bool podeavancar2 = true;
					if (coratual1 == 2) {
						podeavancar1 = false;
					}
					if (coratual2 == 2) {
						podeavancar2 = false;
					}
					int coratual3 = coratual1 + 1;
					int coratual4 = coratual2 + 1;
					int cor3 = MSX_Groups[grupoatual1][coratual1];
					if (cor3 == 0) {
						podeavancar1 = false;
					}
					int cor4 = MSX_Groups[grupoatual2][coratual2];
					if (cor4 == 0) {
						podeavancar2 = false;
					}
					if (!podeavancar1 && !podeavancar2) {
						temnogrupo = false;
					} else if (podeavancar1 && !podeavancar2) {
						coratual1++;
					} else if (!podeavancar1 && podeavancar2) {
						coratual2++;
					} else {
						int NY3 = MSX_Colors1[cor3][3];
						int NY4 = MSX_Colors1[cor4][3];
						if (NY3 <= NY4) {
							coratual1++;
						} else {
							coratual2++;
						}
					}
				}
			}
		}
	}
}

void MSXConvert::ArmazenaCor(int cor1, int cor2) {
	double NR1, NG1, NB1, NY1;
	NR1 = MSX_Colors1[cor1][0];
	NG1 = MSX_Colors1[cor1][1];
	NB1 = MSX_Colors1[cor1][2];
	NY1 = MSX_Colors1[cor1][3];
	
	CorPossivel corpossivel;
	if (cor1 == cor2) {
		corpossivel.R = NR1;
		corpossivel.G = NG1;
		corpossivel.B = NB1;
		corpossivel.cor1 = cor1;
		corpossivel.cor2 = cor2;
	} else {
		double NR2, NG2, NB2, NY2;
		NR2 = MSX_Colors1[cor2][0];
		NG2 = MSX_Colors1[cor2][1];
		NB2 = MSX_Colors1[cor2][2];
		NY2 = MSX_Colors1[cor2][3];
		
		//Preenche
		corpossivel.R = sqrt((pow(NR1, 2.0) + pow(NR2, 2.0)) / 2);
		corpossivel.G = sqrt((pow(NG1, 2.0) + pow(NG2, 2.0)) / 2);
		corpossivel.B = sqrt((pow(NB1, 2.0) + pow(NB2, 2.0)) / 2);
		if (NY1 <= NY2) {
			corpossivel.cor1 = cor1;
			corpossivel.cor2 = cor2;
		} else {
			corpossivel.cor1 = cor2;
			corpossivel.cor2 = cor1;
		}
	}
	//Armazena
	corespossiveis.push_back(corpossivel);
}

class ObjGimp {
	public:
		void Convert(GimpDrawable *drawable);
		void AtualizaProgresso(double progresso);
	private:
		//Image info
		gint x1, y1, x2, y2, width, height, channels;
};

void ObjGimp::Convert(GimpDrawable *drawable) {
	AtualizaProgresso(0.0);
	
	/* Gets upper left and lower right coordinates, and layers number in the image. Right lower is not in the selection */
	gimp_drawable_mask_bounds(drawable->drawable_id, &x1, &y1, &x2, &y2);
	width = x2 - x1;
	height = y2 - y1;
	channels = gimp_drawable_bpp(drawable->drawable_id);
	if (channels >= 3) {
		/* Allocate a big enough tile cache */
		gimp_tile_cache_ntiles(2 * (drawable->width / gimp_tile_width() + 1));

		/* Initialises two PixelRgns, one to read original data,
		* and the other to write output data. That second one will
		* be merged at the end by the call to
		* gimp_drawable_merge_shadow() */
		GimpPixelRgn rgn_in, rgn_out;
		gimp_pixel_rgn_init(&rgn_in, drawable, x1, y1, width, height, FALSE, FALSE);
		gimp_pixel_rgn_init(&rgn_out, drawable, x1, y1, width, height, TRUE, TRUE);
		
		BufferRGB bufferinrgb(width, height);

		//Fill bufferinrgb from bufferin
		{
			//Buffer
			guchar *bufferin;
			bufferin = g_new(guchar, width * 1 * channels);
			gint x, y;
			for (gint y = 0; y < height; y++) {
				/* Dump in one line */
				gimp_pixel_rgn_get_rect(&rgn_in, bufferin, x1, y1 + y, width, 1);
				for (gint x = 0; x < width; x++) {
					gint offset = x * channels;
					bufferinrgb.setRGB((int) x, (int) y, (double) bufferin[offset + 0], (double) bufferin[offset + 1], (double) bufferin[offset + 2]);
				}
			}
			g_free(bufferin);
		}
		
		/* Do work */
		MSXConvert msxconvert;
		BufferRGB bufferoutrgb(width, height);
		std::function<void(double)> myAtualizaProgresso = std::bind(&ObjGimp::AtualizaProgresso, this, std::placeholders::_1);
		msxconvert.Convert(bufferinrgb, bufferoutrgb, myAtualizaProgresso);
		
		//Fill bufferout from bufferoutrgb 
		{
			//Buffer
			guchar *bufferout;
			bufferout = g_new(guchar, width * 1 * channels);
			gint x, y;
			for (gint y = 0; y < height; y++) {
				for (gint x = 0; x < width; x++) {
					double R, G, B;
					bufferoutrgb.getRGB((int) x, (int) y, R, G, B);
					gint offset = x * channels;
					bufferout[offset + 0] = (guchar) R;
					bufferout[offset + 1] = (guchar) G;
					bufferout[offset + 2] = (guchar) B;
					//Alpha channel
					if (channels==4)bufferout[offset + 3] = (guchar) 255;
				}
				/* Dump out one line */
				gimp_pixel_rgn_set_rect(&rgn_out, bufferout, x1, y1 + y, width, 1);
			}
			g_free(bufferout);
		}

		/*  Update the modified region */
		gimp_drawable_flush(drawable);
		gimp_drawable_merge_shadow(drawable->drawable_id,TRUE);
		gimp_drawable_update(drawable->drawable_id,x1,y1,width,height);
	}
}

void ObjGimp::AtualizaProgresso(double progresso) {
	gimp_progress_update((gdouble) progresso);
}

static void msx_ize(GimpDrawable *drawable) {
	//std::cout << "\n";
	ObjGimp objGimp;
	objGimp.Convert(drawable);
}
