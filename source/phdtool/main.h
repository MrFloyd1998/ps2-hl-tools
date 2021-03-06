// Author:	supadupaplex
// License:	BSD-3-Clause (check out license.txt)

//
// This file contains all definitions and declarations
//

#ifndef MAIN_H
#define MAIN_H

////////// Includes //////////
#include <stdio.h>		// puts(), printf(), snprintf()
#include <string.h>		// strcpy(), strcat(), strlen(), strncpy()
#include <malloc.h>		// malloc(), free()
#include <stdlib.h>		// exit()
#include <math.h>		// floor(), round()
#include <ctype.h>		// tolower()

////////// Definitions //////////
#define PROG_TITLE "\nPS2 HL decal (PHD) tool v1.3\n"
#define PROG_INFO "\
Developed by supadupaplex, 2017-2021\n\
License: BSD-3-Clause (check out license.txt)\n\
Zlib library is used to perform deflate/inflate operations\n\
\n\
How to use:\n\
1) Windows explorer - drag and drop PS2 HL Decal or PNG file on phdtool.exe\n\
2) Command line/Batch - phdtool [file_name]\n\
Optional: convert to PNG - phdtool topng [phd_file_name]\n\
\n\
For more info check out readme.txt\n\
"

#define PSI_MIN_DIMENSION 8
#define EIGHT_BIT_PALETTE_ELEMENTS_COUNT 256

// Image tyes
#define PSI_UNKNOWN 0
#define PSI_INDEXED 2
#define PSI_RGBA 5

////////// Zlib stuff //////////
#include "zlib.h"
#include <assert.h>

////////// Typedefs //////////
#include "types.h"

////////// Functions //////////

// File operations
#include "fops.h"

// PNG Functions
#include "pngtool.h"

////////// Structures //////////

// PS2 HL Decal header
#pragma pack(1)
struct sPHDHeader
{
	char Signature[64];		// Filled with zeroes

	void Update()
	{
		memset(this->Signature, 0x00, sizeof(sPHDHeader));
	}

	void UpdateFromFile(FILE ** ptrFile)
	{
		// Copy header from file to this structure
		FileReadBlock(ptrFile, this, 0, sizeof(sPHDHeader));
	}

	bool Check()
	{
		for (ulong i = 0; i < sizeof(sPHDHeader); i++)
		{
			if (this->Signature[i] != 0x00)
				return false;
		}

		return true;
	}
};

// *.psi image header
#pragma pack(1)
struct sPSIHeader
{
	char Name[16];		// Internal name
	uchar Magic[3];		// Filled with zeroes in most cases
	uchar MIPCount;		// Number of MIPs that present in image file (used in decals only)
	ulong Type;			// 2 - 8 bit indexed bitmap, 5 - 32 bit RGBA bitmap
	ushort Width;		// Texture width (in pixels)
	ushort Height;		// Textre Height (in pixels)
	ushort UpWidth;		// Upscale target: width (in pixels)
	ushort UpHeight;	// Upscale target: height (in pixels)

	void Update(const char * NewName, ushort NewWidth, ushort NewHeight, ulong NewType, uchar NewMIPCount)
	{
		// Clear this structure from garbage and leftovers
		memset(this, 0x00, sizeof(sPSIHeader));

		// Update fields
		snprintf(this->Name, sizeof(Name), "%s", NewName);
		this->MIPCount = NewMIPCount;
		this->Type = NewType;
		this->Width = NewWidth;
		this->Height = NewHeight;
		this->UpWidth = NewWidth;
		this->UpHeight = NewHeight;
	}

	void UpdateUpscaleTaget(ushort NewUpWidth, ushort NewUpHeight)
	{
		this->UpWidth = NewUpWidth;
		this->UpHeight = NewUpHeight;
	}

	void UpdateFromFile(FILE ** ptrFile, ulong Address)
	{
		// Copy texture header directly to this structure
		FileReadBlock(ptrFile, this, Address, sizeof(sPSIHeader));
	}

	uchar CheckType()
	{
		if (this->Type == PSI_INDEXED || this->Type == PSI_RGBA)
			return this->Type;
		else
			return PSI_UNKNOWN;
	}
};

// 8-bit *.bmp header
#pragma pack(1)				// Fix unwanted 0x00 bytes in structure
struct sBMPHeader
{
	char Signature1[2];		// "BM" Signature
	ulong FileSize;			// Total file size (in bytes)
	ulong Signature2;		// = 0
	ulong Offset;			// Offset
	ulong StructSize;		// = 0x28 for BMP version 3
	ulong Width;			// Picture Width (in pixels)
	ulong Height;			// Picture Height (in pixels)
	ushort Signature3;		// = 1 for BMP version 3
	ushort BitsPerPixel;	// How many bits per 1 pixel
	ulong Compression;		// Compression type
	ulong PixelDataSize;	// Size of bitmap
	ulong HorizontalPPM;	// Horizontal pixels per meter value
	ulong VerticalPPM;		// Vertical pixels per meter value
	ulong ColorTabSize;		// How many colors are present in color table
	ulong ColorTabAlloc;	// How many colors are actually used in color table

	void Update(unsigned long int Width, unsigned long int Height)		// Update all fields of BMP header
	{
		this->Signature1[0] = 'B';
		this->Signature1[1] = 'M';
		this->Signature2 = 0x00000000;
		this->Offset = 0x00000436;
		this->StructSize = 0x00000028;
		this->Signature3 = 0x0001;
		this->BitsPerPixel = 0x0008;
		this->Compression = 0x00000000;
		this->HorizontalPPM = 0x00000000;		// = 0 (not used)
		this->VerticalPPM = 0x00000000;			// = 0 (not used)
		this->ColorTabSize = 0x00000100;
		this->ColorTabAlloc = 0x00000100;
		this->Width = Width;
		this->Height = Height;
		this->PixelDataSize = Height * Width;
		this->FileSize = this->PixelDataSize + this->Offset;
	}

	void UpdateFromFile(FILE ** ptrFile)
	{
		// Copy header from file to this structure
		FileReadBlock(ptrFile, this, 0, sizeof(sBMPHeader));
	}

	bool Check()
	{
		if (this->Signature1[0] == 'B' && this->Signature1[1] == 'M' && this->Signature2 == 0 && this->Signature3 == 1 && this->BitsPerPixel == 8 && this->Compression == 0)
			return true;
		else
			return false;
	}
};

#endif // MAIN_H
