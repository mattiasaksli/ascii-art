#define NOMINMAX

#include <iostream>
#include <cmath>
#include <string>
#include <limits>
#include <opencv2/opencv.hpp>
#include <opencv2/core/utils/logger.hpp>
#include <Windows.h>

using namespace cv;

const std::string ASCII_MATRIX = "`^\",:;Il!i~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$";

std::string getImageFilename(int argc, char* argv[])
{
	std::string filename;

	if (argc > 1)
	{
		filename = argv[1];
	}
	else
	{
		std::cout << "Enter the image filename: ";
		std::cin >> filename;
		std::cout << std::endl;
	}

	return filename;
}


void resizeImage(Mat& image, int argc, char* argv[])
{
	unsigned short resizeScalar;

	if (argc > 2)
	{
		resizeScalar = std::stoi(argv[2]);

		if (resizeScalar > 100 || resizeScalar < 0)
		{
			std::cerr << "Invalid image scaling value: " << resizeScalar << "! Using original image size." << std::endl;
			return;
		}
	}
	else
	{
		std::cout << "What percentage should the image be downscaled to? (Enter 100 to use the original size): ";

		while (!(std::cin >> resizeScalar)
			   || resizeScalar > 100
			   || resizeScalar < 0)
		{
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			std::cerr << "Please enter a number between 0 and 100: ";
		}

		std::cout << std::endl;
	}

	if (resizeScalar < 100 && resizeScalar > 0)
	{
		double scalar = resizeScalar / 100.0;
		resize(image, image, Size(), scalar, scalar, INTER_AREA);
	}
}

float sRGBtoLinearRGB(float colorChannel)
{
	// Send this function a decimal sRGB gamma encoded color value
	// between 0.0 and 1.0, and it returns a linearized value.

	if (colorChannel <= 0.04045f)
	{
		return colorChannel / 12.92f;
	}

	return pow(((colorChannel + 0.055f) / 1.055f), 2.4f);
}

float luminanceToPerceivedBrightness(float luminance)
{
	// Send this function a luminance value between 0.0 and 1.0,
	// and it returns L* which is "perceptual lightness"

	if (luminance <= (216.0f / 24389.0f))
	{       // The CIE standard states 0.008856 but 216/24389 is the intent for 0.008856451679036
		return luminance * (24389.0f / 27.0f);  // The CIE standard states 903.3, but 24389/27 is the intent, making 903.296296296296296
	}

	return pow(luminance, (1.0f / 3.0f)) * 116.0f - 16.0f;
}


/**
* Implementation from https://stackoverflow.com/a/56678483
*
* @returns float in range [0, 1]
*/
float calculatePixelBrightness(short r, short g, short b)
{
	float decimalR = r / 255.0f;
	float decimalG = g / 255.0f;
	float decimalB = b / 255.0f;

	// Average the RGB values
	//return (decimalR + decimalG + decimalB) / 3.0f;

	// Average the min and max values of RGB
	//float colors[3] = {decimalR, decimalG, decimalB};
	//return (*(std::max_element(std::begin(colors), std::end(colors))) + *(std::min_element(std::begin(colors), std::end(colors)))) / 2.0f;

	// Weighted average luminosity version
	float luminance = (0.2126f * sRGBtoLinearRGB(decimalR) + 0.7152f * sRGBtoLinearRGB(decimalG) + 0.0722f * sRGBtoLinearRGB(decimalB));
	return luminanceToPerceivedBrightness(luminance) / 100.0f;
}

char imagePixelToAsciiChar(const Mat& image, int i, int j)
{
	Vec3b bgrPixel = image.at<Vec3b>(i, j);

	float pixelBrightness = calculatePixelBrightness(bgrPixel[2], bgrPixel[1], bgrPixel[0]);

	// Linear interpolation of [0, 1] brightness to ASCII character
	unsigned int asciiMatrixIndex = static_cast<unsigned int>(round(ASCII_MATRIX.size() * pixelBrightness));

	return ASCII_MATRIX[asciiMatrixIndex];
}

void printAsciiImage(const Mat& image, int width, int height, short widthRatio = 1, short heightRatio = 1)
{
	std::string asciiRow = "";

	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			char asciiChar = imagePixelToAsciiChar(image, i, j);

			// Font width correction
			for (short k = 0; k < widthRatio; k++)
			{
				asciiRow += asciiChar;
			}
		}

		// Font height correction
		for (short j = 0; j < heightRatio; j++)
		{
			std::cout << asciiRow << std::endl;
		}

		asciiRow = "";
	}
}

PCONSOLE_FONT_INFOEX saveCurrentConsoleFontInfo(const HANDLE& consoleOutputHandle)
{
	PCONSOLE_FONT_INFOEX lpConsoleCurrentFontEx = new CONSOLE_FONT_INFOEX();
	lpConsoleCurrentFontEx->cbSize = sizeof(CONSOLE_FONT_INFOEX);
	GetCurrentConsoleFontEx(consoleOutputHandle, 0, lpConsoleCurrentFontEx);

	return lpConsoleCurrentFontEx;
}

PCONSOLE_SCREEN_BUFFER_INFO saveCurrentConsoleScreenBufferInfo(const HANDLE& consoleOutputHandle)
{
	PCONSOLE_SCREEN_BUFFER_INFO savedConsoleScreenBufferInfo = new CONSOLE_SCREEN_BUFFER_INFO();
	GetConsoleScreenBufferInfo(consoleOutputHandle, savedConsoleScreenBufferInfo);

	return savedConsoleScreenBufferInfo;
}

void setFontToConsolasWithSize(const HANDLE& consoleOutputHandle, short x, short y)
{
	PCONSOLE_FONT_INFOEX lpConsoleCurrentFontEx = new CONSOLE_FONT_INFOEX();

	lpConsoleCurrentFontEx->cbSize = sizeof(CONSOLE_FONT_INFOEX);
	GetCurrentConsoleFontEx(consoleOutputHandle, 0, lpConsoleCurrentFontEx);
	swprintf_s(lpConsoleCurrentFontEx->FaceName, L"Consolas");
	lpConsoleCurrentFontEx->dwFontSize.X = x;
	lpConsoleCurrentFontEx->dwFontSize.Y = y;
	SetCurrentConsoleFontEx(consoleOutputHandle, false, lpConsoleCurrentFontEx);

	delete lpConsoleCurrentFontEx;
}

void setConsoleSize(const HANDLE& consoleOutputHandle, short width, short height, short currentConsoleBufferHeight)
{
	SMALL_RECT windowSize;
	windowSize.Left = 0;
	windowSize.Top = 0;
	windowSize.Right = width - 1;
	windowSize.Bottom = height - 1;

	COORD bufferSize;
	bufferSize.X = width;
	if (currentConsoleBufferHeight < 2 * height)
	{
		bufferSize.Y = 2 * height;
	}

	SetConsoleScreenBufferSize(consoleOutputHandle, bufferSize);
	SetConsoleWindowInfo(consoleOutputHandle, true, &windowSize);
}

void restoreConsoleSettings(const HANDLE& consoleOutputHandle, PCONSOLE_FONT_INFOEX& savedConsoleFont, PCONSOLE_SCREEN_BUFFER_INFO& savedConsoleBufferInfo)
{
	SetCurrentConsoleFontEx(consoleOutputHandle, false, savedConsoleFont);
	SetConsoleScreenBufferSize(consoleOutputHandle, savedConsoleBufferInfo->dwSize);
	SetConsoleWindowInfo(consoleOutputHandle, true, &(savedConsoleBufferInfo->srWindow));

	delete savedConsoleFont;
	delete savedConsoleBufferInfo;
}

int main(int argc, char* argv[])
{
	// Disable OpenCV logging
	utils::logging::setLogLevel(utils::logging::LogLevel::LOG_LEVEL_SILENT);

	HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);

	// Save console settings
	PCONSOLE_FONT_INFOEX savedConsoleFont = saveCurrentConsoleFontInfo(out);
	PCONSOLE_SCREEN_BUFFER_INFO savedConsoleBufferInfo = saveCurrentConsoleScreenBufferInfo(out);

	setFontToConsolasWithSize(out, 8, 16);

	// Image input
	std::string filename = getImageFilename(argc, argv);
	Mat image = imread(filename);

	if (image.empty())
	{
		std::cerr << "Image not found!" << std::endl;
		system("pause");
		return EXIT_FAILURE;
	}

	resizeImage(image, argc, argv);

	int width = image.cols;
	int height = image.rows;

	std::cout << std::endl;
	std::cout << "Image size: " << width << " x " << height << std::endl;

	setFontToConsolasWithSize(out, 8, 8);
	setConsoleSize(out, 2 * width + 3, height + 5, savedConsoleBufferInfo->dwSize.Y);

	printAsciiImage(image, width, height, 2);

	system("pause");

	restoreConsoleSettings(out, savedConsoleFont, savedConsoleBufferInfo);

	return EXIT_SUCCESS;
}
