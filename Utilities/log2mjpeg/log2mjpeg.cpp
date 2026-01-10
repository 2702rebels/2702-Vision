#include <fstream>
#include <vector>
#include <iostream>

#include <opencv2/imgcodecs.hpp>

#include "VisionLog.h"
#include "Bayer2RGB.h"
#include <filesystem>
#include "Bayer2Grey.h"

enum pmode { pmcolor, pmcolor2, pmgrey, pmgrey2, pmbayer, pmgreen, pmred, pmblue, pminput };

namespace fs = std::filesystem;

pmode procmode = pminput;

void Bayer2Red( cv::Mat &matRed, cv::Mat const &matBayer )
{
	assert( matRed.rows == matBayer.rows / 2 && matRed.cols == matBayer.cols / 2 );
	assert( matRed.type() == CV_8UC1 && matBayer.type() == CV_8UC1 );
	assert( (matBayer.rows & 1) == 0 && (matBayer.cols & 1) == 0 );

	for( int r = 0; r < matRed.rows; r++ ) {
		uint8_t const *pu8Bayer = matBayer.ptr( r * 2 ) + 1;
		uint8_t *pu8Red = matRed.ptr( r );
		for( int c = 0; c < matRed.cols; c++ ) {
			*pu8Red++ = *pu8Bayer;
			pu8Bayer += 2;
		}
	}
}

void Bayer2Blue( cv::Mat &matBlue, cv::Mat const &matBayer )
{
	assert( matBlue.rows == matBayer.rows / 2 && matBlue.cols == matBayer.cols / 2 );
	assert( matBlue.type() == CV_8UC1 && matBayer.type() == CV_8UC1 );
	assert( (matBayer.rows & 1) == 0 && (matBayer.cols & 1) == 0 );

	for( int r = 0; r < matBlue.rows; r++ ) {
		uint8_t const *pu8Bayer = matBayer.ptr( r * 2 + 1 );
		uint8_t *pu8Blue = matBlue.ptr( r );
		for( int c = 0; c < matBlue.cols; c++ ) {
			*pu8Blue++ = *pu8Bayer;
			pu8Bayer += 2;
		}
	}
}

void Bayer2Green( cv::Mat &matGreen, cv::Mat const &matBayer )
{
	assert( matGreen.rows == matBayer.rows / 2 && matGreen.cols == matBayer.cols / 2 );
	assert( matGreen.type() == CV_8UC1 && matBayer.type() == CV_8UC1 );
	assert( (matBayer.rows & 1) == 0 && (matBayer.cols & 1) == 0 );

	int nColsPlus1 = matBayer.cols + 1; 
	for( int r = 0; r < matGreen.rows; r++ ) {
		uint8_t const *pu8Bayer = matBayer.ptr( r * 2 );
		uint8_t *pu8Green = matGreen.ptr( r );
		for( int c = 0; c < matGreen.cols; c++ ) {
			*pu8Green++ = (pu8Bayer[0] + pu8Bayer[nColsPlus1]) /2;
			pu8Bayer += 2;
		}
	}
}

void Usage()
{
	std::cerr << "Usage: log2mjpeg [-colour|colour2|grey|grey2|bayer|red|green|blue|input] infile outdir\n";
	std::cerr << "      colour     -> RGB\n";
	std::cerr << "      colour2    -> RGB 1/2 resolution\n";
	std::cerr << "      grey       -> GreyScale\n";
	std::cerr << "      grey2      -> GreyScale 1/2 resolution\n";
	std::cerr << "      beyer      -> Raw Bayer frame\n";
	std::cerr << "      red        -> Extracts the red pixels\n";
	std::cerr << "      green      -> Extracts the green pixels\n";
	std::cerr << "      blue       -> Extracts the blue pixels\n";
	exit(1);
}

int main( int argc, char **argv )
{
	char const *pszIn = 0;
	char const *pszOut = 0;
	for( int i = 1; i < argc; i++ ) {
		if( argv[i][0] == '-' ) {
			if( !strcmp( argv[i], "-colour" ) )
				procmode = pmcolor;
			else if( !strcmp( argv[i], "-colour2" ) )
				procmode = pmcolor2;
			else if( !strcmp( argv[i], "-grey" ) )
				procmode = pmgrey;
			else if( !strcmp( argv[i], "-grey2" ) )
				procmode = pmgrey2;
			else if( !strcmp( argv[i], "-bayer" ) )
				procmode = pmbayer;
			else if( !strcmp( argv[i], "-red" ) )
				procmode = pmred;
			else if( !strcmp( argv[i], "-green" ) )
				procmode = pmgreen;
			else if( !strcmp( argv[i], "-blue" ) )
				procmode = pmblue;
			else if( !strcmp( argv[i], "-input" ) )
				procmode = pminput;
			else
				Usage();
		}
		else if( !pszIn )
			pszIn = argv[i];
		else if( !pszOut )
			pszOut = argv[i];
		else
			Usage();
	}
	if( !pszOut ) {
		Usage();
		return 1;
	}

	VisionLogReader vl( pszIn );
	if(!fs::exists(pszOut)){
		fs::create_directories(pszOut);
	}

	int nSeq = 0;
	std::optional<cv::Mat> img;
	while( img = vl.TryReadFrame()) {
		if (!img.has_value()) {
			break;
		}
		cv::Mat imgOut;
		switch( procmode ) {
		case pminput:
			imgOut = img.value();
			break;
		case pmgrey:
			{
				cv::Mat matGrey( img.value().rows, img.value().cols, CV_8UC1 );
				Bayer2Grey( matGrey, img.value() );
				imgOut = matGrey;
			}
			break;
		case pmgrey2:
			{
				cv::Mat matGrey( img.value().rows/2, img.value().cols/2, CV_8UC1 );
				Bayer2HalfGrey( matGrey, img.value() );
				imgOut = matGrey;
			}
			break;
		case pmbayer:
			imgOut = img.value();
			break;
		case pmred:
			{
				cv::Mat matRed( img.value().rows/2, img.value().cols/2, CV_8UC1 );
				Bayer2Red( matRed, img.value() );
				imgOut = matRed;
			}
			break;
		case pmgreen:
			{
				cv::Mat matGreen( img.value().rows/2, img.value().cols/2, CV_8UC1 );
				Bayer2Green( matGreen, img.value() );
				imgOut = matGreen;
			}
			break;
		case pmblue:
			{
				cv::Mat matBlue( img.value().rows/2, img.value().cols/2, CV_8UC1 );
				Bayer2Blue( matBlue, img.value() );
				imgOut = matBlue;
			}
			break;
		case pmcolor2:
			{
				cv::Mat matRGB( img.value().rows/2, img.value().cols/2, CV_8UC3 );
				Bayer2HalfRGB( matRGB, img.value() );
				imgOut = matRGB;
			}
			break;
		case pmcolor:
		default:
			{
				cv::Mat matRGB( img.value().rows, img.value().cols, CV_8UC3 );
				Bayer2RGB( matRGB, img.value() );
				imgOut = matRGB;
			}
			break;
		}
		std::vector<uchar> vJPEG;
		cv::imencode( ".jpg", imgOut, vJPEG );
		std::ofstream outfile(std::string(pszOut) + "/" + std::to_string(nSeq) + ".jpg");
		outfile.write( (const char *)vJPEG.data(), vJPEG.size() );
		nSeq++;
	}
	return 0;
}
