#include "image_scaler.h"
#include <cmath>

constexpr int GAMMASIZE = 1000;

static float togamma[256];
static unsigned char fromgamma[GAMMASIZE+24];


void scale_down_pre_calculate(
	float fx, unsigned int width, unsigned int new_width,
	int*& ixA, float*& dxA, int*& nrxA, float*& dxB)
{
	int nrcols = (int)ceil(fx + 1.0f);
	int nrx = 0, nrx1;
	float startx = 0.0f;
	float endx = fx;

	dxA = new float[nrcols * new_width];
	dxB = new float[new_width];
	ixA = new int[nrcols * new_width];
	nrxA = new int[new_width];

	for (int t1 = 0; t1 < new_width; t1++)
	{
		endx = endx < width + 1 ? endx : endx - width + 1;

		for (float x = startx; x < endx; x = floor(x + 1.0f))
		{
			ixA[nrx] = (int)x;

			if (endx - x > 1.0f)
				dxA[nrx] = floor(x + 1.0f) - x;
			else
				dxA[nrx] = endx - x;

			nrx++;
		}

		if (t1 > 0)
			nrxA[t1] = nrx - nrx1;
		else
			nrxA[t1] = nrx;

		nrx1 = nrx;

		dxB[t1] = endx - startx;

		startx = endx;
		endx += fx;
	}
}

void scale_down(float fx, float fy,
	unsigned char* buffer,
	unsigned int width, unsigned int height,
	unsigned char* new_buffer,
	unsigned int new_width, unsigned int new_height)
{
	int nrrows = (int)ceil(fy + 1.0f);
	unsigned char** rows = new unsigned char* [nrrows];
	float* dxA, * dxB, * dyA = new float[nrrows];
	int* ixA, * iyA = new int[nrrows], * nrxA;
	unsigned char* newline = new_buffer, * c1, * c2;
	float starty, endy = 0.0f, dy, diffY;
	int pos, nrx, nrx1, nry, t3, t6;
	float sum1, f, area;

	scale_down_pre_calculate(fx, width, new_width, ixA, dxA, nrxA, dxB);

	for (int t1 = 0; t1 < new_height; t1++)
	{
		pos = t1 * fy;
		t3 = pos + nrrows < height + 1 ? 0 : pos - height + 1;

		for (int t2 = 0; t2 < nrrows + t3; t2++)
		{
			rows[t2] = buffer + (pos + t2) * width;
		}

		starty = t1 * fy - pos;
		endy = (t1 + 1) * fy - pos + t3;
		diffY = endy - starty;
		nry = 0;

		for (float y = starty; y < endy; y = floor(y + 1.0f))
		{
			iyA[nry] = (int)y;
			if (endy - y > 1.0f)
				dyA[nry] = floor(y + 1.0f) - y;
			else
				dyA[nry] = endy - y;

			nry++;
		}

		nrx = 0;
		t6 = 0;

		for (int t2 = 0; t2 < new_width; t2++)
		{
			t3 = nrxA[t2];
			area = (1.0f / (dxB[t2] * diffY)) * GAMMASIZE;
			sum1 = 0.0f;

			for (int t4 = 0; t4 < nry; t4++)
			{
				c1 = rows[iyA[t4]];
				dy = dyA[t4];

				nrx1 = nrx;

				for (int t5 = 0; t5 < t3; t5++)
				{
					f = dxA[nrx1] * dy;
					c2 = c1 + ixA[nrx1];
					sum1 += togamma[c2[0]] * f;
					nrx1++;
				}
			}

			newline[t6] = fromgamma[(int)(sum1 * area)];

			nrx += t3;
			t6 += 1;
		}

		newline += new_width;
	}

	delete[] rows;
	delete nrxA;
	delete ixA;
	delete[] iyA;
	delete dxA;
	delete[] dyA;
	delete dxB;
}

void img::downscale(
	unsigned char * buffer,
	unsigned int width, unsigned int height, 
	unsigned char* new_buffer,
	unsigned int new_width, unsigned int new_height)
{
	auto fx = (float)width / new_width;
	auto fy = (float)height / new_height;

	for (int i1 = 0; i1 < 256; i1++)
		togamma[i1] = (float)pow(i1 / 255.0, 2.2);

	for (int i2 = 0; i2 < GAMMASIZE + 1; i2++)
		fromgamma[i2] = (unsigned char)(pow((double)i2 / GAMMASIZE, 1 / 2.2) * 255);


	if (fx > 1.0 || fy > 1.0) {
		scale_down(fx, fy, 
			buffer, width, height, 
			new_buffer, new_width, new_height);
		return;
	}
	
	// unsupported upscaler - do nothing
}
