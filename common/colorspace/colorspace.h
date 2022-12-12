/** 
 * @author Pascal Getreuer 2005-2010 <getreuer@gmail.com>
 */

// s. cpp file for details

// modified mihail.isakov@gmail.com
// C++ static class

#ifndef A_COLORSPACE_H
#define A_COLORSPACE_H

class ColorSpace_
{
public:
	ColorSpace_()  {}
	~ColorSpace_() {}
	static void Rgb2Yuv(double *Y, double *U, double *V, double R, double G, double B);
	static void Yuv2Rgb(double *R, double *G, double *B, double Y, double U, double V);
	static void Rgb2Ycbcr(double *Y, double *Cb, double *Cr, double R, double G, double B);
	static void Ycbcr2Rgb(double *R, double *G, double *B, double Y, double Cb, double Cr);
	static void Rgb2Jpegycbcr(double *R, double *G, double *B, double Y, double Cb, double Cr);
	static void Jpegycbcr2Rgb(double *R, double *G, double *B, double Y, double Cb, double Cr);
	static void Rgb2Ypbpr(double *Y, double *Pb, double *Pr, double R, double G, double B);
	static void Ypbpr2Rgb(double *R, double *G, double *B, double Y, double Pb, double Pr);
	static void Rgb2Ydbdr(double *Y, double *Db, double *Dr, double R, double G, double B);
	static void Ydbdr2Rgb(double *R, double *G, double *B, double Y, double Db, double Dr);
	static void Rgb2Yiq(double *Y, double *I, double *Q, double R, double G, double B);
	static void Yiq2Rgb(double *R, double *G, double *B, double Y, double I, double Q);
	static void Rgb2Hsv(double *H, double *S, double *V, double R, double G, double B);
	static void Hsv2Rgb(double *R, double *G, double *B, double H, double S, double V);
	static void Rgb2Hsl(double *H, double *S, double *L, double R, double G, double B);
	static void Hsl2Rgb(double *R, double *G, double *B, double H, double S, double L);
	static void Rgb2Hsi(double *H, double *S, double *I, double R, double G, double B);
	static void Hsi2Rgb(double *R, double *G, double *B, double H, double S, double I);
	static void Rgb2Xyz(double *X, double *Y, double *Z, double R, double G, double B);
	static void Xyz2Rgb(double *R, double *G, double *B, double X, double Y, double Z);
	static void Xyz2Lab(double *L, double *a, double *b, double X, double Y, double Z);
	static void Lab2Xyz(double *X, double *Y, double *Z, double L, double a, double b);
	static void Xyz2Luv(double *L, double *u, double *v, double X, double Y, double Z);
	static void Luv2Xyz(double *X, double *Y, double *Z, double L, double u, double v);
	static void Xyz2Lch(double *L, double *C, double *H, double X, double Y, double Z);
	static void Lch2Xyz(double *X, double *Y, double *Z, double L, double C, double H);
	static void Xyz2Cat02lms(double *L, double *M, double *S, double X, double Y, double Z);
	static void Cat02lms2Xyz(double *X, double *Y, double *Z, double L, double M, double S);
	static void Rgb2Lab(double *L, double *a, double *b, double R, double G, double B);
	static void Lab2Rgb(double *R, double *G, double *B, double L, double a, double b);
	static void Rgb2Luv(double *L, double *u, double *v, double R, double G, double B);
	static void Luv2Rgb(double *R, double *G, double *B, double L, double u, double v);
	static void Rgb2Lch(double *L, double *C, double *H, double R, double G, double B);
	static void Lch2Rgb(double *R, double *G, double *B, double L, double C, double H);
	static void Rgb2Cat02lms(double *L, double *M, double *S, double R, double G, double B);
	static void Cat02lms2Rgb(double *R, double *G, double *B, double L, double M, double S);
};

#endif  // A_COLORSPACE_H
