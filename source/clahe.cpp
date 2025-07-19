#include <iostream>
#include <vector>
#include "../header/png.h"

template <typename T>
T clamp(T val, T low, T high)
{
    return (val < low) ? low : (val > high) ? high : val;
}

bool clahe_gray(PngImage img, int tile_length, double clip_factor);

bool clahe(PngImage img, int tile_length, double clip_factor, double blend_factor)
{
    if(img.data == nullptr || img.channels > 3)
        return false;

    if(img.channels == 1)
        return clahe_gray(img, tile_length, clip_factor);

    int numTilesX = (img.width + tile_length - 1) / tile_length;
    int numTilesY = (img.height + tile_length - 1) / tile_length;
    int totalTiles = numTilesX * numTilesY;

    double alpha = clamp(blend_factor, 0.0, 1.0); // Ensure alpha is between 0 and 1

    int clip_limit;
    if(clip_factor > 0)
    {
        clip_limit = tile_length * tile_length / clip_factor;
        if(clip_limit < 4)
            clip_limit = 4;
    }
    else
        clip_limit = tile_length * tile_length;

    std::vector<std::vector<std::vector<int>>> Histogram(
        totalTiles, std::vector<std::vector<int>>(img.channels, std::vector<int>(256, 0)));

    std::vector<std::vector<std::vector<double>>> cumilativeDist(
        totalTiles, std::vector<std::vector<double>>(img.channels, std::vector<double>(256, 0.0)));

    std::vector<std::vector<std::vector<double>>> HistoEqua(
        totalTiles, std::vector<std::vector<double>>(img.channels, std::vector<double>(256, 0)));

    // Loop over the tiles
    for (int TileR = 0; TileR < numTilesY; TileR++){
        for (int TileC = 0; TileC < numTilesX; TileC++)
        {
            int tileIndex = TileR * numTilesX + TileC;

            int startY = TileR * tile_length;
            int startX = TileC * tile_length;
            int tileHeight = std::min(tile_length, img.height - startY);
            int tileWidth = std::min(tile_length, img.width - startX);
            int tileSize = tileHeight * tileWidth;

            int total_excess[3] = {0};

            // Loop over pixels inside this tile to get histogram
            for (int y = 0; y < tileHeight; ++y)
            {
                for (int x = 0; x < tileWidth; ++x)
                {
                    int globalY = startY + y;
                    int globalX = startX + x;
                    int index = (globalY * img.width + globalX) * img.channels;

                    for (int c = 0; c < img.channels; ++c)
                    {
                        unsigned char value = img.data[index + c];
                        if (Histogram[tileIndex][c][value] < clip_limit)
                            Histogram[tileIndex][c][value]++;
                        else
                            total_excess[c]++;
                    }
                }
            }

            // Redistribute the excess over the bins
            for (int c = 0; c < img.channels; ++c)
            {
                double add = (double)total_excess[c] / 256.0;
                double accumulator = 0.0;

                for (int i = 0; i < 256; ++i)
                {
                    accumulator += add;
                    int to_add = (int)(accumulator + 0.5);
                    Histogram[tileIndex][c][i] += to_add;
                    accumulator -= to_add;
                }
            }

            // Cumulative distribution for the tile
            for (int c = 0; c < img.channels; ++c)
            {
                cumilativeDist[tileIndex][c][0] = (double)Histogram[tileIndex][c][0] / tileSize;

                for (int i = 1; i < 256; ++i) {
                    cumilativeDist[tileIndex][c][i] = cumilativeDist[tileIndex][c][i - 1] +
                                                    (double)Histogram[tileIndex][c][i] / tileSize;
                }
            }

            // Histogram equalization
            for (int c = 0; c < img.channels; ++c)
            {
                for (int i = 0; i < 256; ++i) {
                    HistoEqua[tileIndex][c][i] = (alpha * cumilativeDist[tileIndex][c][i] * i +
                                                (1 - alpha) * i);
                }
            }
        }
    }

    // Interpolation and mapping the equalized values to the image with improved smoothing
    for (int y = 0; y < img.height; ++y)
    {
        for (int x = 0; x < img.width; ++x)
        {
            double gx = (double)x / tile_length - 0.5;
            double gy = (double)y / tile_length - 0.5;

            int x1 = clamp((int)(gx), 0, numTilesX - 1);
            int y1 = clamp((int)(gy), 0, numTilesY - 1);
            int x2 = clamp(x1 + 1, 0, numTilesX - 1);
            int y2 = clamp(y1 + 1, 0, numTilesY - 1);

            double dx = gx - x1;
            double dy = gy - y1;

            int index = (y * img.width + x) * img.channels;

            for (int c = 0; c < img.channels; ++c)
            {
                int val = img.data[index + c];

                int tile00 = y1 * numTilesX + x1;
                int tile10 = y1 * numTilesX + x2;
                int tile01 = y2 * numTilesX + x1;
                int tile11 = y2 * numTilesX + x2;

                // Enhanced bilinear interpolation with weighted smoothing
                double top = (1 - dx) * HistoEqua[tile00][c][val] + dx * HistoEqua[tile10][c][val];
                double bottom = (1 - dx) * HistoEqua[tile01][c][val] + dx * HistoEqua[tile11][c][val];
                double interpolated = (1 - dy) * top + dy * bottom;

                // Apply a small smoothing factor to reduce blockiness
                double smooth_factor = 0.1; // Adjustable, reduces edge artifacts
                interpolated = (1 - smooth_factor) * interpolated + smooth_factor * val;

                // Clamp and assign
                interpolated = clamp(interpolated, 0.0, 255.0);
                img.data[index + c] = static_cast<unsigned char>(interpolated + 0.5);
            }
        }
    }
    return true;
}

bool clahe_gray(PngImage img, int tile_length, double clip_factor)
{
    if (img.data == nullptr || img.channels != 1)
        return false;

    int numTilesX = (img.width + tile_length - 1) / tile_length;
    int numTilesY = (img.height + tile_length - 1) / tile_length;
    int totalTiles = numTilesX * numTilesY;

    int clip_limit = (clip_factor > 0)
        ? std::max(4, static_cast<int>(tile_length * tile_length / clip_factor))
        : tile_length * tile_length;

    std::vector<std::vector<int>> Histogram(totalTiles, std::vector<int>(256, 0));
    std::vector<std::vector<double>> cumDist(totalTiles, std::vector<double>(256, 0.0));
    std::vector<std::vector<double>> Equalized(totalTiles, std::vector<double>(256, 0.0));

    for (int tileY = 0; tileY < numTilesY; ++tileY) {
        for (int tileX = 0; tileX < numTilesX; ++tileX) {
            int tileIdx = tileY * numTilesX + tileX;
            int startY = tileY * tile_length;
            int startX = tileX * tile_length;
            int tileH = std::min(tile_length, img.height - startY);
            int tileW = std::min(tile_length, img.width - startX);
            int tileSize = tileH * tileW;

            int excess = 0;
            for (int y = 0; y < tileH; ++y) {
                for (int x = 0; x < tileW; ++x) {
                    int px = (startY + y) * img.width + (startX + x);
                    int val = img.data[px];
                    if (Histogram[tileIdx][val] < clip_limit)
                        Histogram[tileIdx][val]++;
                    else
                        excess++;
                }
            }

            double redist = static_cast<double>(excess) / 256.0;
            double acc = 0.0;
            for (int i = 0; i < 256; ++i) {
                acc += redist;
                int extra = static_cast<int>(acc + 0.5);
                Histogram[tileIdx][i] += extra;
                acc -= extra;
            }

            cumDist[tileIdx][0] = static_cast<double>(Histogram[tileIdx][0]) / tileSize;
            for (int i = 1; i < 256; ++i)
                cumDist[tileIdx][i] = cumDist[tileIdx][i - 1] + static_cast<double>(Histogram[tileIdx][i]) / tileSize;

            for (int i = 0; i < 256; ++i)
                Equalized[tileIdx][i] = cumDist[tileIdx][i] * i;
        }
    }

    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            double gx = static_cast<double>(x) / tile_length - 0.5;
            double gy = static_cast<double>(y) / tile_length - 0.5;
            int x1 = clamp(static_cast<int>(gx), 0, numTilesX - 1);
            int y1 = clamp(static_cast<int>(gy), 0, numTilesY - 1);
            int x2 = clamp(x1 + 1, 0, numTilesX - 1);
            int y2 = clamp(y1 + 1, 0, numTilesY - 1);

            double dx = gx - x1;
            double dy = gy - y1;

            int tile00 = y1 * numTilesX + x1;
            int tile10 = y1 * numTilesX + x2;
            int tile01 = y2 * numTilesX + x1;
            int tile11 = y2 * numTilesX + x2;

            int idx = y * img.width + x;
            int val = img.data[idx];

            double top = (1 - dx) * Equalized[tile00][val] + dx * Equalized[tile10][val];
            double bot = (1 - dx) * Equalized[tile01][val] + dx * Equalized[tile11][val];
            double interp = (1 - dy) * top + dy * bot;

            img.data[idx] = static_cast<unsigned char>(clamp(interp + 0.5, 0.0, 255.0));
        }
    }

    return true;
}
