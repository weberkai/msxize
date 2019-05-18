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
	
	CC=g++ LIBS="-lm" CFLAGS="-std=c++11 -O3 -omsxize" gimptool-2.0 --install "ColorSpace.cpp Comparison.cpp Conversion.cpp objgimp.cpp objmsx.cpp msxize.cpp"

	If there is any package missing (typically libgimp-dev), you will be warned at that call.
*/

#include "msxize.h"

/* --------------------- GIMP CODE ------------------------ */

static void query(void);
static void run(const gchar *name, gint nparams, const GimpParam *param, gint *nreturn_vals, GimpParam **return_vals);

GimpPlugInInfo PLUG_IN_INFO={
	NULL,
	NULL,
	query,
	run
};

MAIN()

static void query(void) {
	static GimpParamDef args[]={
		{GIMP_PDB_INT32, (char*) "run-mode", (char*) "Run mode"},
		{GIMP_PDB_IMAGE, (char*) "image", (char*) "Input image"},
		{GIMP_PDB_DRAWABLE, (char*) "drawable", (char*) "Input drawable"}
	};

	gimp_install_procedure(
		"plug-in-msxize",
		"MSXize",
		"Converts the image to the MSX appearance",
		"Weber Estevan Roder Kai",
		"Copyright Weber Estevan Roder Kai",
		"2019",
		"_MSXize",
		"RGB*",
		GIMP_PLUGIN,
		G_N_ELEMENTS(args),
		0,
		args,
		NULL
	);
	gimp_plugin_menu_register(
		"plug-in-msxize",
		"<Image>/Filters/MSXize"
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
	gimp_progress_init("MSXize...");
	
	/* Do the work */
	MSXConvert msxconvert;
	ObjGimp objGimp;
	objGimp.Convert(drawable, msxconvert);

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
