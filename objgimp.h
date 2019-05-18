#ifndef OBJGIMP_H
#define OBJGIMP_H

#include <libgimp/gimp.h>

#include <vector>

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

class ObjSpecific;

class ObjGimp {
	public:
		void Convert(GimpDrawable *drawable, ObjSpecific &objspecific);
		void AtualizaProgresso(double progresso);
	private:
		//Image info
		gint x1, y1, x2, y2, width, height, channels;
};

class ObjSpecific {
	public:
		virtual void Convert(BufferRGB &bufferin, BufferRGB &bufferout, ObjGimp &objectgimp) {};
};

#endif // OBJGIMP_H
