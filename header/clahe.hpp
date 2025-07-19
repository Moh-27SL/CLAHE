#ifndef CLAHE_HPP_INCLUDED
#define CLAHE_HPP_INCLUDED

#include "png.h"


bool clahe(PngImage img, int tile_length, double  clip_factor, double blend_factor);


#endif // CLAHE_HPP_INCLUDED
