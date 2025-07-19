///Contrast Limited Adaptive Histogram Equalization (CLAHE)

///bool clahe(png image, tile size, clip factor, blend factor);

/*
step 1) divide image into tiles size (M x M) where (M) is a user parameter

/////Repeat Steps 2 To 5 For Each Tile/////

step 2) for the tile get the Histogram of each color
        and while doing so clip the excess and sum it to the total excess.
        (clip factor) is a user parameter (clip limit = tile height * tile width / clip factor)

step 3) after finishing the histogram for the tile add for each bin the excess per bin (usually bins = 256 for 8 bits)
        (excess per bin = total excess / number of bins)

step 4) get Cumulative Distribution for the tile
        (c(I) = c(I-1) + (1/N) * H(I), s.th. c(0) = H(0)/N, N = number of pixels per tile = tile height * tile width)

step 5) apply Histogram Equalization for the tile
        (HE(I) = alpha * c(I) + (1-alpha) * I, s.th. alpha is a blend parameter in range [0.0,1.0])

/////Interpolation Time for each tile//////

step 6)

 y
 ↑
 |   (0,1)          (1,1)
 |    HE10         HE11
 |      +------------+
 |      |            |
 |      |   (fx,fy)  |
 |      |            |
 |      +------------+
 |   (0,0)          (1,0)
 |    HE00         HE01
 +------------------------→ x

P[I] =
    (1 - fx) * (1 - fy) * HE00[I] +
    fx * (1 - fy) * HE01[I] +
    (1 - fx) * fy * HE10[I] +
    fx * fy * HE11[I];


step 7) enjoy :)
*/

#include <iostream>
#include "../header/clahe.hpp"

using namespace std;

int main()
{
    PngImage img = load_png("result/bom.png");

    clahe(img,32,10,0.6);
    save_png("result/bom_2.png",img);
    return 0;
}
