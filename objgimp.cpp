#include "objgimp.h"

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
		BufferRGB bufferoutrgb(width, height);
		this->p_objspecific.Convert(bufferinrgb, bufferoutrgb, (*this));
		
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
