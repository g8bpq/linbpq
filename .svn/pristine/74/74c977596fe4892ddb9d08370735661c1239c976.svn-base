//------------------------------------------
//  PNGFILE.H -- Header File for pngfile.c
//------------------------------------------
//#define byte	short
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void PngFileInitialize (HWND hwnd) ;
BOOL PngFileOpenDlg (HWND hwnd, PTSTR pstrFileName, PTSTR pstrTitleName) ;
BOOL PngFileSaveDlg (HWND hwnd, PTSTR pstrFileName, PTSTR pstrTitleName) ;

BOOL PngLoadImage (PTSTR pstrFileName, png_byte **ppbImageData, 
                   int *piWidth, int *piHeight, int *piChannels, png_color *pBkgColor);
BOOL PngSaveImage (PTSTR pstrFileName, png_byte *pDiData,
                   int iWidth, int iHeight, png_color BkgColor);

#if defined(PNG_NO_STDIO)
static void png_read_data(png_structp png_ptr, png_bytep data, png_size_t length);
static void png_write_data(png_structp png_ptr, png_bytep data, png_size_t length);
static void png_flush(png_structp png_ptr);
#endif

