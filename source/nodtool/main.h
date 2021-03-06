// Author:	supadupaplex
// License:	BSD-3-Clause (check out license.txt)

//
// This file contains all definitions and declarations
//

#ifndef MAIN_H
#define MAIN_H

////////// Includes //////////
#include <stdio.h>		// puts(), printf(), sscanf(), snprintf(), rename(), remove()
#include <string.h>		// strcpy(), strcat(), strlen(), strtok(), strncpy(), memset()
#include <malloc.h>		// malloc(), free()
#include <stdlib.h>		// exit()
#include <math.h>		// round(), sqrt(), ceil()
#include <ctype.h>		// tolower()

////////// Definitions //////////
#define PROG_TITLE "\nPS2 HL NOD tool v0.9\n"
#define PROG_INFO "\
Developed by supadupaplex, 2018-2021\n\
License: BSD-3-Clause (check out license.txt)\n\
\n\
How to use: \n\
1) Windows explorer - drag and drop *.nod file on nodtool.exe\n\
2) Command line\\Batch - nodtool [file_name]\n\
\tCheck file: nodtool test [file_name]\n\
\n\
For more info check out readme.txt\n\
"

// Node format specificators
enum eNodFormats
{
	NOD_FORMAT_PC,
	NOD_FORMAT_PS2,
	NOD_ERR_VERSION,
	NOD_ERR_UNKNOWN
};

// Proper node graph version
#define NOD_SUPPORTED_VESION		16

// PS2 extra bytes in CNode structure (took those from PS2's c0a0.nod)
#define PS2_CN_EXTRA1	0x00000000
#define PS2_CN_EXTRA2	0x00000807

// Sizes of structures inside node file
#define SZ_CGRAPH		8396
#define SZ_PC_CNODE		88
#define SZ_PS2_CNODE	96
#define SZ_CLINK		24
#define SZ_DIST_INFO	16

////////// Typedefs //////////
#include "types.h"

////////// Functions //////////
#include "fops.h"

////////// Structures //////////
#pragma pack(1)				// Fix unwanted 0x00 bytes in structure
struct sCGraph
{
	uchar Somedata1[12];

	uint pNodes;
	uint pLinkPool;
	uint pRouteInfo;

	int NodeCount;
	int LinkCount;
	int RouteCount;

	uchar SomeData2[8344];

	uint pHashLinks;
	int HashCount;

	uchar SomeData3[8];
};

#pragma pack(1)				// Fix unwanted 0x00 bytes in structure
struct sCNode_PC
{
	uchar SomeData[SZ_PC_CNODE];
};

#pragma pack(1)				// Fix unwanted 0x00 bytes in structure
struct sCNode_PS2
{
	sCNode_PC CNode;
	ulong ExtraField1;
	ulong ExtraField2;
};

#pragma pack(1)				// Fix unwanted 0x00 bytes in structure
struct sCLink
{
	uchar SomeData[SZ_CLINK];
};

#pragma pack(1)				// Fix unwanted 0x00 bytes in structure
struct sDIST_INFO
{
	uchar SomeData[SZ_DIST_INFO];
};

#pragma pack(1)				// Fix unwanted 0x00 bytes in structure
struct sNodeGraph
{
	int Version;			// Should be 16
	sCGraph CGraph;
	sCNode_PS2 * CNodes;
	sCLink * CLinks;
	sDIST_INFO * DistInfo;
	char * Routes;
	short * Hashes;

	void Init()
	{
		// Clear memory
		memset(this, 0x00, sizeof(this));

		// Initialize pointers
		CNodes = NULL;
		CLinks = NULL;
		DistInfo = NULL;
		Routes = NULL;
		Hashes = NULL;
	}

	void Deinit()
	{
		// Free memory
		if (CNodes != NULL)		free(CNodes);
		if (CLinks != NULL)		free(CLinks);
		if (DistInfo != NULL)	free(DistInfo);
		if (Routes != NULL)		free(Routes);
		if (Hashes != NULL)		free(Hashes);
	}

	int LoadAndCheckHeader(FILE **ptrFile)
	{
		// Load header
		FileReadBlock(ptrFile, this, 0, sizeof(Version) + sizeof(sCGraph));

		// Calculate base size
		ulong BaseSize =	sizeof(Version) +
							sizeof(sCGraph) +
							sizeof(sCLink) * CGraph.LinkCount +
							sizeof(sDIST_INFO) * CGraph.NodeCount +
							sizeof(char) * CGraph.RouteCount +
							sizeof(short) * CGraph.HashCount;

		// Check file
		if (Version == NOD_SUPPORTED_VESION)
		{
			if (FileSize(ptrFile) == (BaseSize + sizeof(sCNode_PC) * CGraph.NodeCount))
			{
				return NOD_FORMAT_PC;
			}
			else if (FileSize(ptrFile) == (BaseSize + sizeof(sCNode_PS2) * CGraph.NodeCount))
			{
				return NOD_FORMAT_PS2;
			}
			else
			{
				return NOD_ERR_UNKNOWN;
			}
		}
		else
		{
			return NOD_ERR_VERSION;
		}
	}

	int UpdateFromFile(FILE **ptrFile)
	{
		// Load and check header
		int Result = LoadAndCheckHeader(ptrFile);
		if (Result == NOD_ERR_VERSION || Result == NOD_ERR_UNKNOWN)
			return Result;

		// Allocate memory for structures //

		// Nodes
		if (CNodes != NULL)
			free(CNodes);
		CNodes = (sCNode_PS2 *)		calloc(sizeof(sCNode_PS2) * CGraph.NodeCount, 1);

		// Links
		if (CLinks != NULL)
			free(CLinks);
		CLinks = (sCLink *)			calloc(sizeof(sCLink) * CGraph.LinkCount, 1);

		// Dists
		if (DistInfo != NULL)
			free(DistInfo);
		DistInfo = (sDIST_INFO *)	calloc(sizeof(sDIST_INFO) * CGraph.NodeCount, 1);

		// Routes
		if (Routes != NULL)
			free(Routes);
		Routes = (char *)			calloc(sizeof(char) * CGraph.RouteCount, 1);

		// Hashes
		if (Hashes != NULL)
			free(Hashes);
		Hashes = (short *)			calloc(sizeof(short) * CGraph.HashCount, 1);

		// Check allocation
		if (CNodes == NULL || CLinks == NULL || DistInfo == NULL || Routes == NULL || Hashes == NULL)
		{
			UTIL_WAIT_KEY("Unable to allocate memory ...");
			exit(EXIT_FAILURE);
		}

		// Load structures //
		ulong Pointer = sizeof(int) + sizeof(sCGraph);

		// Nodes
		if (Result == NOD_FORMAT_PS2)
		{
			FileReadBlock(ptrFile, CNodes, Pointer, sizeof(sCNode_PS2) * CGraph.NodeCount);
			Pointer += sizeof(sCNode_PS2) * CGraph.NodeCount;
		}
		else if (Result == NOD_FORMAT_PC)
		{
			// Convert to PS2 format on the fly
			for (int i = 0; i < CGraph.NodeCount; i++)
			{
				FileReadBlock(ptrFile, &CNodes[i], Pointer, sizeof(sCNode_PC));
				Pointer += sizeof(sCNode_PC);

				CNodes[i].ExtraField1 = PS2_CN_EXTRA1;
				CNodes[i].ExtraField2 = PS2_CN_EXTRA2;
			}
		}

		// Links
		FileReadBlock(ptrFile, CLinks, Pointer, sizeof(sCLink) * CGraph.LinkCount);
		Pointer += sizeof(sCLink) * CGraph.LinkCount;

		// Dists
		FileReadBlock(ptrFile, DistInfo, Pointer, sizeof(sDIST_INFO) * CGraph.NodeCount);
		Pointer += sizeof(sDIST_INFO) * CGraph.NodeCount;

		// Routes
		FileReadBlock(ptrFile, Routes, Pointer, sizeof(char) * CGraph.RouteCount);
		Pointer += sizeof(char) * CGraph.RouteCount;
		
		// Hashes
		FileReadBlock(ptrFile, Hashes, Pointer, sizeof(short) * CGraph.HashCount);

		return Result;
	}

	void SaveToFile(FILE **ptrFile, eNodFormats Format)
	{
		// Save structures to file //
		ulong Pointer = 0;

		// Header
		FileWriteBlock(ptrFile, this, 0, sizeof(Version) + sizeof(sCGraph));
		Pointer += sizeof(int) + sizeof(sCGraph);

		// Nodes
		if (Format == NOD_FORMAT_PC)
		{
			// Convert to PC format on the fly
			for (int i = 0; i < CGraph.NodeCount; i++)
			{
				FileWriteBlock(ptrFile, &CNodes[i], sizeof(sCNode_PC));
				Pointer += sizeof(sCNode_PS2);
			}
		}
		else
		{
			FileWriteBlock(ptrFile, CNodes, sizeof(sCNode_PS2) * CGraph.NodeCount);
			Pointer += sizeof(sCNode_PS2) * CGraph.NodeCount;
		}

		// Links
		FileWriteBlock(ptrFile, CLinks, sizeof(sCLink) * CGraph.LinkCount);
		Pointer += sizeof(sCLink) * CGraph.LinkCount;

		// Dists
		FileWriteBlock(ptrFile, DistInfo, sizeof(sDIST_INFO) * CGraph.NodeCount);
		Pointer += sizeof(sDIST_INFO) * CGraph.NodeCount;

		// Routes
		FileWriteBlock(ptrFile, Routes, sizeof(char) * CGraph.RouteCount);
		Pointer += sizeof(char) * CGraph.RouteCount;

		// Hashes
		FileWriteBlock(ptrFile, Hashes, sizeof(short) * CGraph.HashCount);
	}
};

#endif // MAIN_H
