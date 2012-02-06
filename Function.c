//================================================================================================
// Copyright Nikon Corporation - All rights reserved
//
// View this file in a non-proportional font, tabs = 3
//================================================================================================

#include <Carbon/Carbon.h>

#ifdef _WINDOWS
	#include <io.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "maid3.h"
#include "maid3d1.h"
#include "CtrlSample.h"
//------------------------------------------------------------------------------------------------
//
SLONG CallMAIDEntryPoint( 
		LPNkMAIDObject	pObject,				// module, source, item, or data object
		ULONG				ulCommand,			// Command, one of eNkMAIDCommand
		ULONG				ulParam,				// parameter for the command
		ULONG				ulDataType,			// Data type, one of eNkMAIDDataType
		NKPARAM			data,					// Pointer or long integer
		LPNKFUNC			pfnComplete,		// Completion function, may be NULL
		NKREF				refComplete )		// Value passed to pfnComplete
{
	return (*(LPMAIDEntryPointProc)g_pMAIDEntryPoint)( 
						pObject, ulCommand, ulParam, ulDataType, data, pfnComplete, refComplete );
}
//------------------------------------------------------------------------------------------------
//
_BOOL Search_Module( void* Path )
{
#ifdef _WINDOWS
	char	TempPath[MAX_PATH];
	struct	_finddata_t c_file;
	long	hFile;

	// Search a module file in the current directory.
	GetCurrentDirectory( MAX_PATH - 11, TempPath );
	strcat( TempPath, "\\Type0001.md3" );
	if ( (hFile = _findfirst( TempPath, &c_file )) == -1L ) {
		return false;
	}
	strcpy( Path, TempPath );
#else
	UInt64 ProcessSN = kNoProcess;
	Str255 Name;
	FSSpec AppSpec;
	ProcessInfoRec stPIRec;
	stPIRec.processInfoLength = sizeof(ProcessInfoRec);
	stPIRec.processName = (unsigned char*)&Name;
	stPIRec.processAppSpec = &AppSpec;

	// Make FSSpec of current process.
	GetCurrentProcess( (ProcessSerialNumber*)&ProcessSN );
	GetProcessInformation( (ProcessSerialNumber*)&ProcessSN, &stPIRec );
	// Make FSSpec of Type0001 Module.
	FSMakeFSSpec( AppSpec.vRefNum, AppSpec.parID, "\pType0001 Module.bundle", (FSSpecPtr)Path );
#endif
	return true;
}
//------------------------------------------------------------------------------------------------
//
_BOOL Load_Module(const void* Path )
{
#ifdef _WINDOWS
	g_hInstModule = LoadLibrary( Path );

	if (g_hInstModule) {
		g_pMAIDEntryPoint = (BOOL_)GetProcAddress( g_hInstModule, "MAIDEntryPoint" );
		if ( g_pMAIDEntryPoint == NULL )
			puts( "MAIDEntryPoint cannot be found.\n" ); 
	} else {
		g_pMAIDEntryPoint = NULL;
		printf( "\"%s\" cannot be opened.\n", Path ); 
	}
	return (g_hInstModule != NULL) && (g_pMAIDEntryPoint != NULL);
#else
	Ptr	pCFragMain;
	FSRef fsRef;
	// Create FSRef from FSSpec
	if ( FSpMakeFSRef( (FSSpec*)Path, &fsRef ) != noErr ) {
		return FALSE;
	}
	// Create CFURLRef from FSRef
	CFURLRef urlRef = CFURLCreateFromFSRef( kCFAllocatorDefault, &fsRef );
	if ( urlRef == nil ) {
		return FALSE;
	}
	// Create CFByundle object from CFURLRef.
	CFBundleRef bundleRef = CFBundleCreate( kCFAllocatorDefault, urlRef );
	CFRelease( urlRef );
	if ( bundleRef == nil ) {
		return FALSE;
	}
	// Load and link dynamic CFBundle object 
	if ( !CFBundleLoadExecutable(bundleRef) ) {
		CFRelease( bundleRef );
		return FALSE;
	}
	// Get entry point from BundleRef
	pCFragMain = (Ptr)CFBundleGetFunctionPointerForName( bundleRef, CFSTR("MAIDEntryPoint") );
	if ( pCFragMain == nil ) {
		CFRelease( bundleRef );		
		return FALSE;
	}
	// Set the pointer for Maid entry point BOOL_ type variable
	//g_pMAIDEntryPoint = BOOL_( pCFragMain );	
	g_pMAIDEntryPoint = (LPMAIDEntryPointProc) pCFragMain;	
	return TRUE;
#endif
}
//------------------------------------------------------------------------------------------------
//
_BOOL Command_Open( NkMAIDObject* pParentObj, NkMAIDObject* pChildObj, ULONG ulChildID )
{
	SLONG lResult = CallMAIDEntryPoint( pParentObj, kNkMAIDCommand_Open, ulChildID, 
									kNkMAIDDataType_ObjectPtr, (NKPARAM)pChildObj, NULL, NULL );
	return lResult == kNkMAIDResult_NoError;
}
//------------------------------------------------------------------------------------------------
//
_BOOL Command_Close( LPNkMAIDObject pObject )
{
	SLONG nResult = CallMAIDEntryPoint( pObject, kNkMAIDCommand_Close, 0, 0, 0, NULL, NULL );

	return nResult == kNkMAIDResult_NoError;
}
//------------------------------------------------------------------------------------------------
//
_BOOL Close_Module( LPRefObj pRefMod )
{
	_BOOL bRet;
	LPRefObj pRefSrc, pRefItm, pRefDat;
	ULONG i, j, k;

	if(pRefMod->pObject != NULL)
	{
		for(i = 0; i < pRefMod->ulChildCount; i ++)
		{
			pRefSrc = GetRefChildPtr_Index( pRefMod, i );
			for(j = 0; j < pRefSrc->ulChildCount; j ++)
			{
				pRefItm = GetRefChildPtr_Index( pRefSrc, j );
				for(k = 0; k < pRefItm->ulChildCount; k ++)
				{
					pRefDat = GetRefChildPtr_Index( pRefItm, k );
					bRet = ResetProc( pRefDat );
					if ( bRet == false )	return false;
					bRet = Command_Close( pRefDat->pObject );
					if ( bRet == false )	return false;
					free(pRefDat->pObject);
					free(pRefDat->pCapArray);
					free(pRefDat);//
					pRefDat = NULL;//
				}
				bRet = ResetProc( pRefItm );
				if ( bRet == false )	return false;
				bRet = Command_Close( pRefItm->pObject );
				if ( bRet == false )	return false;
				free(pRefItm->pObject);
				free(pRefItm->pRefChildArray);
				free(pRefItm->pCapArray);
				free(pRefItm);//
				pRefItm = NULL;//
			}
			bRet = ResetProc( pRefSrc );
			if ( bRet == false )	return false;
			bRet = Command_Close( pRefSrc->pObject );
			if ( bRet == false )	return false;
			free(pRefSrc->pObject);
			free(pRefSrc->pRefChildArray);
			free(pRefSrc->pCapArray);
			free(pRefSrc);//
			pRefSrc = NULL;//
		}
		bRet = ResetProc( pRefMod );
		if ( bRet == false )	return false;
		bRet = Command_Close( pRefMod->pObject );
		if ( bRet == false )	return false;
		free(pRefMod->pObject);
		pRefMod->pObject = NULL;

		free(pRefMod->pRefChildArray);
		pRefMod->pRefChildArray = NULL;
		pRefMod->ulChildCount = 0;
	
		free(pRefMod->pCapArray);
		pRefMod->pCapArray = NULL;
		pRefMod->ulCapCount = 0;
	}
	return true;
}
//------------------------------------------------------------------------------------------------
//
void InitRefObj( LPRefObj pRef )
{
	pRef->pObject = NULL;
	pRef->lMyID = 0x8000;
	pRef->pRefParent = NULL;
	pRef->ulChildCount = 0;
	pRef->pRefChildArray = NULL;
	pRef->ulCapCount = 0;
	pRef->pCapArray = NULL;
}
//------------------------------------------------------------------------------------------------------------------------------------
// issue async command while wait for the CompletionProc called.
_BOOL IdleLoop( LPNkMAIDObject pObject, ULONG* pulCount, ULONG ulEndCount )
{
	_BOOL bRet = true;
	while( *pulCount < ulEndCount && bRet == true ) {
		bRet = Command_Async( pObject );
	#ifdef _WINDOWS
		Sleep(10);
	#else
		Boolean		gotEvent;
		EventRecord	theEvent;
		gotEvent = WaitNextEvent( highLevelEventMask, &theEvent, 1, NULL );
		if( gotEvent )
			AEProcessAppleEvent( &theEvent );
	#endif
	}
	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Wait for Apple event. On MacOSX, the event from camera is an Apple event. 
void WaitEvent()
{
	#ifdef _WINDOWS
		// Do nothing
	#else
		Boolean		gotEvent;
		EventRecord	theEvent;
		while(1)
		{
			gotEvent = WaitNextEvent( highLevelEventMask, &theEvent, 1, NULL );
			if( gotEvent )
				AEProcessAppleEvent( &theEvent );
			else
				break;
		}
	#endif
}
//------------------------------------------------------------------------------------------------------------------------------------
// enumerate capabilities belong to the object that 'pObject' points to.
_BOOL EnumCapabilities( LPNkMAIDObject pObject, ULONG* pulCapCount, LPNkMAIDCapInfo* ppCapArray, LPNKFUNC pfnComplete, NKREF refComplete )
{
	SLONG nResult;

	do {
 		// call the module to get the number of the capabilities.
		ULONG	ulCount = 0L;
		LPRefCompletionProc pRefCompletion;
		// This memory block is freed in the CompletionProc.
		pRefCompletion = (LPRefCompletionProc)malloc( sizeof(RefCompletionProc) );
		pRefCompletion->pulCount = &ulCount;
		pRefCompletion->pRef = NULL;
		nResult = CallMAIDEntryPoint(	pObject,
												kNkMAIDCommand_GetCapCount,
												0,
												kNkMAIDDataType_UnsignedPtr,
												(NKPARAM)pulCapCount,
												(LPNKFUNC)CompletionProc,
												(NKREF)pRefCompletion );
		IdleLoop( pObject, &ulCount, 1 );
 
 		if ( nResult == kNkMAIDResult_NoError )
 		{
 			// allocate memory for the capability array
 			*ppCapArray = (LPNkMAIDCapInfo)malloc( *pulCapCount * sizeof( NkMAIDCapInfo ) );
  
 			if ( *ppCapArray != NULL )
 			{
 				// call the module to get the capability array
   				ulCount = 0L;
				// This memory block is freed in the CompletionProc.
				pRefCompletion = (LPRefCompletionProc)malloc( sizeof(RefCompletionProc) );
				pRefCompletion->pulCount = &ulCount;
				pRefCompletion->pRef = NULL;
				nResult = CallMAIDEntryPoint(	pObject,
														kNkMAIDCommand_GetCapInfo,
														*pulCapCount,
														kNkMAIDDataType_CapInfoPtr,
														(NKPARAM)*ppCapArray,
														(LPNKFUNC)CompletionProc,
														(NKREF)pRefCompletion );
				IdleLoop( pObject, &ulCount, 1 );

 				if (nResult == kNkMAIDResult_BufferSize)
 				{
					free( *ppCapArray );
					*ppCapArray = NULL;
				}
			}
		}
	}
	// repeat the process if the number of capabilites changed between the two calls to the module
	while (nResult == kNkMAIDResult_BufferSize);

	// return TRUE if the capabilities were successfully enumerated
	return (nResult == kNkMAIDResult_NoError || nResult == kNkMAIDResult_Pending);
}
//------------------------------------------------------------------------------------------------------------------------------------
// enumerate child object
_BOOL EnumChildrten(LPNkMAIDObject pobject)
{
	SLONG nResult;
	ULONG	ulCount = 0L;
	LPRefCompletionProc pRefCompletion;
	pRefCompletion = (LPRefCompletionProc)malloc( sizeof(RefCompletionProc) );
	pRefCompletion->pulCount = &ulCount;
	pRefCompletion->pRef = NULL;
	nResult = CallMAIDEntryPoint(	pobject,
											kNkMAIDCommand_EnumChildren, 
											0,
											kNkMAIDDataType_Null,
											(NKPARAM)NULL,
											(LPNKFUNC)CompletionProc,
											(NKREF)pRefCompletion );
	IdleLoop( pobject, &ulCount, 1 );

	return ( nResult == kNkMAIDResult_NoError );
}
//------------------------------------------------------------------------------------------------------------------------------------
//
_BOOL Command_CapGetArray(LPNkMAIDObject pobject, ULONG ulParam, ULONG ulDataType, NKPARAM pData, LPNKFUNC pfnComplete, NKREF refComplete )
{
	SLONG nResult;
	ULONG	ulCount = 0L;
	LPRefCompletionProc pRefCompletion;
	pRefCompletion = (LPRefCompletionProc)malloc( sizeof(RefCompletionProc) );
	pRefCompletion->pulCount = &ulCount;
	pRefCompletion->pRef = NULL;
	nResult = CallMAIDEntryPoint(	pobject,
											kNkMAIDCommand_CapGetArray,
											ulParam,
											ulDataType,
											pData,
											(LPNKFUNC)CompletionProc,
											(NKREF)pRefCompletion );
	IdleLoop( pobject, &ulCount, 1 );

	return (nResult == kNkMAIDResult_NoError);
}
//------------------------------------------------------------------------------------------------------------------------------------
//
_BOOL Command_CapGetDefault(LPNkMAIDObject pobject, ULONG ulParam, ULONG ulDataType, NKPARAM pData, LPNKFUNC pfnComplete, NKREF refComplete )
{
	SLONG nResult;
	ULONG	ulCount = 0L;
	LPRefCompletionProc pRefCompletion;
	pRefCompletion = (LPRefCompletionProc)malloc( sizeof(RefCompletionProc) );
	pRefCompletion->pulCount = &ulCount;
	pRefCompletion->pRef = NULL;
	nResult = CallMAIDEntryPoint(	pobject,
											kNkMAIDCommand_CapGetDefault,
											ulParam,
											ulDataType,
											pData,
											(LPNKFUNC)CompletionProc,
											(NKREF)pRefCompletion );
	IdleLoop( pobject, &ulCount, 1 );

	return (nResult == kNkMAIDResult_NoError);
}
//------------------------------------------------------------------------------------------------------------------------------------
//
_BOOL Command_CapGet(LPNkMAIDObject pobject, ULONG ulParam, ULONG ulDataType, NKPARAM pData, LPNKFUNC pfnComplete, NKREF refComplete )
{
	SLONG nResult;
	ULONG	ulCount = 0L;
	LPRefCompletionProc pRefCompletion;
	pRefCompletion = (LPRefCompletionProc)malloc( sizeof(RefCompletionProc) );
	pRefCompletion->pulCount = &ulCount;
	pRefCompletion->pRef = NULL;
	nResult = CallMAIDEntryPoint(	pobject,
											kNkMAIDCommand_CapGet, 
											ulParam,
											ulDataType,
											pData,
											(LPNKFUNC)CompletionProc,
											(NKREF)pRefCompletion );
	IdleLoop( pobject, &ulCount, 1 );

	return ( nResult == kNkMAIDResult_NoError );
}
//------------------------------------------------------------------------------------------------------------------------------------
//
_BOOL Command_CapSet(LPNkMAIDObject pobject, ULONG ulParam, ULONG ulDataType, NKPARAM pData, LPNKFUNC pfnComplete, NKREF refComplete )
{
	SLONG nResult;
	ULONG	ulCount = 0L;
	LPRefCompletionProc pRefCompletion;
	pRefCompletion = (LPRefCompletionProc)malloc( sizeof(RefCompletionProc) );
	pRefCompletion->pulCount = &ulCount;
	pRefCompletion->pRef = NULL;
	nResult = CallMAIDEntryPoint(	pobject,
											kNkMAIDCommand_CapSet, 
											ulParam,
											ulDataType,
											pData,
											(LPNKFUNC)CompletionProc,
											(NKREF)pRefCompletion );
	IdleLoop( pobject, &ulCount, 1 );

	return ( nResult == kNkMAIDResult_NoError );
}
//------------------------------------------------------------------------------------------------------------------------------------
//
_BOOL Command_CapStart(LPNkMAIDObject pobject, ULONG ulParam, LPNKFUNC pfnComplete, NKREF refComplete)
{
	SLONG nResult = CallMAIDEntryPoint( pobject,
													kNkMAIDCommand_CapStart, 
													ulParam,
													kNkMAIDDataType_Null,
													(NKPARAM)NULL,
													pfnComplete,
													refComplete );
	return ( nResult == kNkMAIDResult_NoError || nResult == kNkMAIDResult_Pending );
}
//------------------------------------------------------------------------------------------------------------------------------------
//
_BOOL Command_Abort(LPNkMAIDObject pobject, LPNKFUNC pfnComplete, NKREF refComplete)
{
	SLONG lResult = CallMAIDEntryPoint( pobject,
													kNkMAIDCommand_Abort, 
													(ULONG)NULL,
													kNkMAIDDataType_Null,
													(NKPARAM)NULL,
													pfnComplete,
													refComplete );
	return lResult == kNkMAIDResult_NoError;
}
//------------------------------------------------------------------------------------------------------------------------------------
//
_BOOL Command_Async(LPNkMAIDObject pobject)
{
	SLONG lResult = CallMAIDEntryPoint( pobject,
										kNkMAIDCommand_Async,
										0,
										kNkMAIDDataType_Null,
										(NKPARAM)NULL,
										(LPNKFUNC)NULL,
										(NKREF)NULL );
	return( lResult == kNkMAIDResult_NoError || lResult == kNkMAIDResult_Pending );
}
//------------------------------------------------------------------------------------------------------------------------------------
//
_BOOL SelectSource( LPRefObj pRefObj, ULONG *pulSrcID )
{
	_BOOL	bRet;
	NkMAIDEnum	stEnum;
	char	buf[256];
	UWORD	wSel;
	ULONG	i;
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, kNkMAIDCapability_Children );
	if ( pCapInfo == NULL ) return false;

	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_Enum ) return false;
	// check if this capability suports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, kNkMAIDCapability_Children, kNkMAIDCapOperation_Get ) ) return false;

	bRet = Command_CapGet( pRefObj->pObject, kNkMAIDCapability_Children, kNkMAIDDataType_EnumPtr, (NKPARAM)&stEnum, NULL, NULL );
	if( bRet == false ) return false;

	// check the data of the capability.
	if ( stEnum.wPhysicalBytes != 4 ) return false;

	if ( stEnum.ulElements == 0 ) {
		printf( "There is no Source object.\n0. Exit\n>" );
		scanf( "%s", buf );
		return true;
	}

	// allocate memory for array data
	stEnum.pData = malloc( stEnum.ulElements * stEnum.wPhysicalBytes );
	if ( stEnum.pData == NULL ) return false;
	// get array data
	bRet = Command_CapGetArray( pRefObj->pObject, kNkMAIDCapability_Children, kNkMAIDDataType_EnumPtr, (NKPARAM)&stEnum, NULL, NULL );
	if( bRet == false ) {
		free( stEnum.pData );
		return false;
	}

	// show the list of selectable Sources
	for ( i = 0; i < stEnum.ulElements; i++ )
		printf( "%d. ID = %d\n", i + 1, ((ULONG*)stEnum.pData)[i] );

	if ( stEnum.ulElements == 1 )
		printf( "0. Exit\nSelect (1, 0)\n>" );
	else
		printf( "0. Exit\nSelect (1-%d, 0)\n>", stEnum.ulElements );

	scanf( "%s", buf );
	wSel = atoi( buf );

	if ( wSel > 0 && wSel <= stEnum.ulElements ) {
		*pulSrcID = ((ULONG*)stEnum.pData)[wSel - 1];
		free( stEnum.pData );
	} else {
		free( stEnum.pData );
		if ( wSel != 0 ) return false;
	}
	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------
//
_BOOL SelectItem( LPRefObj pRefObj, ULONG *pulItemID )
{
	_BOOL	bRet;
	NkMAIDEnum	stEnum;
	char	buf[256];
	UWORD	wSel;
	ULONG	i;
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, kNkMAIDCapability_Children );
	if ( pCapInfo == NULL ) return false;

	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_Enum ) return false;
	// check if this capability suports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, kNkMAIDCapability_Children, kNkMAIDCapOperation_Get ) ) return false;

	bRet = Command_CapGet( pRefObj->pObject, kNkMAIDCapability_Children, kNkMAIDDataType_EnumPtr, (NKPARAM)&stEnum, NULL, NULL );
	if( bRet == false ) return false;

	// check the data of the capability.
	if ( stEnum.ulElements == 0 ) {
		printf( "There is no item.\n" );
		return true;
	}

	// check the data of the capability.
	if ( stEnum.wPhysicalBytes != 4 ) return false;

	// allocate memory for array data
	stEnum.pData = malloc( stEnum.ulElements * stEnum.wPhysicalBytes );
	if ( stEnum.pData == NULL ) return false;
	// get array data
	bRet = Command_CapGetArray( pRefObj->pObject, kNkMAIDCapability_Children, kNkMAIDDataType_EnumPtr, (NKPARAM)&stEnum, NULL, NULL );
	if( bRet == false ) {
		free( stEnum.pData );
		return false;
	}

	// show the list of selectable Items
	for ( i = 0; i < stEnum.ulElements; i++ )
		printf( "%d. Internal ID = %08X\n", i + 1, ((ULONG*)stEnum.pData)[i] );

	if ( stEnum.ulElements == 0 )
		printf( "There is no Item object.\n0. Exit\n>" );
	else if ( stEnum.ulElements == 1 )
		printf( "0. Exit\nSelect (1, 0)\n>" );
	else
		printf( "0. Exit\nSelect (1-%d, 0)\n>", stEnum.ulElements );

	scanf( "%s", buf );
	wSel = atoi( buf );

	if ( wSel > 0 && wSel <= stEnum.ulElements ) {
		*pulItemID = ((ULONG*)stEnum.pData)[wSel - 1];
		free( stEnum.pData );
	} else {
		free( stEnum.pData );
		if ( wSel != 0 ) return false;
	}
	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------
//
_BOOL SelectData( LPRefObj pRefObj, ULONG *pulDataType )
{
	_BOOL	bRet;
	char	buf[256];
	UWORD	wSel;
	ULONG	ulDataTypes, i = 0, DataTypes[8];
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, kNkMAIDCapability_DataTypes );
	if ( pCapInfo == NULL ) return false;

	// check if this capability suports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, kNkMAIDCapability_DataTypes, kNkMAIDCapOperation_Get ) ) return false;

	bRet = Command_CapGet( pRefObj->pObject, kNkMAIDCapability_DataTypes, kNkMAIDDataType_UnsignedPtr, (NKPARAM)&ulDataTypes, NULL, NULL );
	if( bRet == false ) return false;

	// show the list of selectable Data type object.
	if ( ulDataTypes & kNkMAIDDataObjType_Image ) {
	
		// Preview image
		// If the Item is Raw of RAW+JPEG or Dust off data,
		// this Item may not support preview image,
		DataTypes[i++] = 100;	// 100 shows preview image temporary
		printf( "%d. Preview\n", i );
		
		DataTypes[i++] = kNkMAIDDataObjType_Image;
		printf( "%d. Image\n", i );
	}
	if ( ulDataTypes & kNkMAIDDataObjType_Thumbnail ) {
		DataTypes[i++] = kNkMAIDDataObjType_Thumbnail;
		printf( "%d. Thumbnail\n", i );
	}

	if ( i == 0 )
		printf( "There is no Data object.\n0. Exit\n>" );
	else if ( i == 1 )
		printf( "0. Exit\nSelect (1, 0)\n>" );
	else
		printf( "0. Exit\nSelect (1-%d, 0)\n>", i );

	scanf( "%s", buf );
	wSel = atoi( buf );

	if ( wSel > 0 && wSel <= i )
		*pulDataType = DataTypes[wSel - 1];

	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------
//
char* GetEnumString( ULONG ulCapID, ULONG ulValue, char *psString )
{
	switch ( ulCapID ) {
		case kNkMAIDCapability_FlashMode:
			switch( ulValue ){
				case kNkMAIDFlashMode_FrontCurtain:
					strcpy( psString, "Normal" );
					break;
				case kNkMAIDFlashMode_RearCurtain:
					strcpy( psString, "Rear-sync" );
					break;
				case kNkMAIDFlashMode_SlowSync:
					strcpy( psString, "Slow-sync" );
					break;
				case kNkMAIDFlashMode_RedEyeReduction:
					strcpy( psString, "Red Eye Reduction" );
					break;
				case kNkMAIDFlashMode_SlowSyncRedEyeReduction:
					strcpy( psString, "Slow-sync Red Eye Reduction" );
					break;
				case kNkMAIDFlashMode_SlowSyncRearCurtain:
					strcpy( psString, "SlowRear-sync" );
					break;
				default:
					sprintf( psString, "FlashMode %u", ulValue );
			}
			break;
		case kNkMAIDCapability_ExposureMode:
			switch( ulValue ){
				case kNkMAIDExposureMode_Program:
					strcpy( psString, "Program" );
					break;
				case kNkMAIDExposureMode_AperturePriority:
					strcpy( psString, "Aperture" );
					break;
				case kNkMAIDExposureMode_SpeedPriority:
					strcpy( psString, "Speed" );
					break;
				case kNkMAIDExposureMode_Manual:
					strcpy( psString, "Manual" );
					break;
				default:
					sprintf( psString, "ExposureMode %u", ulValue );
			}
			break;
		case kNkMAIDCapability_MeteringMode:
			switch( ulValue ){
				case kNkMAIDMeteringMode_Matrix:
					strcpy( psString, "Matrix" );
					break;
				case kNkMAIDMeteringMode_CenterWeighted:
					strcpy( psString, "CenterWeighted" );
					break;
				case kNkMAIDMeteringMode_Spot:
					strcpy( psString, "Spot" );
					break;
				default:
					sprintf( psString, "MeteringMode %u", ulValue );
			}
			break;
		case kNkMAIDCapability_FocusMode:
			switch( ulValue ){
				case kNkMAIDFocusMode_MF:
					strcpy( psString, "MF" );
					break;
				case kNkMAIDFocusMode_AFs:
					strcpy( psString, "AFs" );
					break;
				case kNkMAIDFocusMode_AFc:
					strcpy( psString, "AFc" );
					break;
				default:
					sprintf( psString, "FocusMode %u", ulValue );
			}
			break;
		case kNkMAIDCapability_FocusPreferredArea:
			sprintf( psString, "FocusPreferredArea :%u", ulValue );
			break;
		case kNkMAIDCapability_CurrentDirID:
			sprintf( psString, "0x%08x", ulValue );
			break;
		case kNkMAIDCapability_ColorAdjustment:
			switch( ulValue ){
				case eNkMAIDColorAdjustment_Minus9:
					strcpy( psString, "-9 deg" );
					break;
				case eNkMAIDColorAdjustment_Minus6:
					strcpy( psString, "-6 deg" );
					break;
				case eNkMAIDColorAdjustment_Minus3:
					strcpy( psString, "-3 deg" );
					break;
				case eNkMAIDColorAdjustment_0:
					strcpy( psString, " 0 deg" );
					break;
				case eNkMAIDColorAdjustment_Plus3:
					strcpy( psString, "+3 deg" );
					break;
				case eNkMAIDColorAdjustment_Plus6:
					strcpy( psString, "+6 deg" );
					break;
				case eNkMAIDColorAdjustment_Plus9:
					strcpy( psString, "+9 deg" );
					break;
				default:
					sprintf( psString, "ColorAdjustment %u", ulValue );
			}
			break;
		case kNkMAIDCapability_PictureControl:
			switch( ulValue ){
				case kNkMAIDPictureControl_Undefined:
					strcpy( psString, "Undefined" );
					break;
				case kNkMAIDPictureControl_Standard:
					strcpy( psString, "Standard" );
					break;
				case kNkMAIDPictureControl_Neutral:
					strcpy( psString, "Neutral" );
					break;
				case kNkMAIDPictureControl_Vivid:
					strcpy( psString, "Vivid" );
					break;
				case kNkMAIDPictureControl_Monochrome:
					strcpy( psString, "Monochrome" );
					break;
				case kNkMAIDPictureControl_Option1:
					strcpy( psString, "Option Picture Contol 1" );
					break;
				case kNkMAIDPictureControl_Option2:
					strcpy( psString, "Option Picture Contol 2" );
					break;
				case kNkMAIDPictureControl_Option3:
					strcpy( psString, "Option Picture Contol 3" );
					break;
				case kNkMAIDPictureControl_Option4:
					strcpy( psString, "Option Picture Contol 4" );
					break;
				case kNkMAIDPictureControl_Custom1:
				case kNkMAIDPictureControl_Custom2:
				case kNkMAIDPictureControl_Custom3:
				case kNkMAIDPictureControl_Custom4:
				case kNkMAIDPictureControl_Custom5:
				case kNkMAIDPictureControl_Custom6:
				case kNkMAIDPictureControl_Custom7:
				case kNkMAIDPictureControl_Custom8:
				case kNkMAIDPictureControl_Custom9:
					sprintf( psString, "Custom Picture Contol %d", ulValue-200 );
					break;
				default:
					sprintf( psString, "Picture Control %u", ulValue );
			}
			break;
		case kNkMAIDCapability_LiveViewImageZoomRate:
			switch( ulValue ){
				case kNkMAIDLiveViewImageZoomRate_All:
					strcpy( psString, "Full" );
					break;
				case kNkMAIDLiveViewImageZoomRate_25:
					strcpy( psString, "25%" );
					break;
				case kNkMAIDLiveViewImageZoomRate_33:
					strcpy( psString, "33%" );
					break;
				case kNkMAIDLiveViewImageZoomRate_50:
					strcpy( psString, "50%" );
					break;
				case kNkMAIDLiveViewImageZoomRate_66:
					strcpy( psString, "66%" );
					break;
				case kNkMAIDLiveViewImageZoomRate_100:
					strcpy( psString, "100%" );
					break;
				case kNkMAIDLiveViewImageZoomRate_200:
					strcpy( psString, "200%" );
					break;
				default:
					sprintf( psString, "LiveViewImageZoomRate %u", ulValue );
			}
			break;
		case kNkMAIDCapability_WBTuneColorTemp:
			sprintf( psString, "%u", ulValue );
			break;
		default:
			strcpy( psString, "Undefined String" ); 
	}
	return psString;
}
//------------------------------------------------------------------------------------------------------------------------------------
char*	GetUnsignedString( ULONG ulCapID, ULONG ulValue, char *psString )
{
	char buff[256];

	switch ( ulCapID )
	{
	case kNkMAIDCapability_MeteringMode:
		sprintf( buff, "%d : Matrix\n", kNkMAIDMeteringMode_Matrix );
		strcpy( psString, buff );
		sprintf( buff, "%d : CenterWeighted\n", kNkMAIDMeteringMode_CenterWeighted );
		strcat( psString, buff );
		sprintf( buff, "%d : Spot\n", kNkMAIDMeteringMode_Spot );
		strcat( psString, buff );
		break;
	case kNkMAIDCapability_FocusMode:
		sprintf( buff, "%d : MF\n", kNkMAIDFocusMode_MF );
		strcpy( psString, buff );
		sprintf( buff, "%d : AF-S\n", kNkMAIDFocusMode_AFs );
		strcat( psString, buff );
		sprintf( buff, "%d : AF-C\n", kNkMAIDFocusMode_AFc );
		strcat( psString, buff );
		break;
	case kNkMAIDCapability_RawJpegImageStatus:
		sprintf( buff, "%d : Single\n", eNkMAIDRawJpegImageStatus_Single );
		strcpy( psString, buff );
		sprintf( buff, "%d : Raw + Jpeg\n", eNkMAIDRawJpegImageStatus_RawJpeg );
		strcat( psString, buff );
		break;
	case kNkMAIDCapability_CameraType:
		sprintf( buff, "%d : D3\n", kNkMAIDCameraType_D3 );
		strcpy( psString, buff );
		sprintf( buff, "%d : D300\n", kNkMAIDCameraType_D300 );
		strcat( psString, buff );
		break;
	case kNkMAIDCapability_DataTypes:
		strcpy( psString, "\0" );
		if ( ulValue & kNkMAIDDataObjType_Image )
		{
			strcat( psString, "Image, " );
		}
		if ( ulValue & kNkMAIDDataObjType_Sound )
		{
			strcat( psString, "Sound, " );
		}
		if ( ulValue & kNkMAIDDataObjType_Video )
		{
			strcat( psString, "Video, " );
		}
		if ( ulValue & kNkMAIDDataObjType_Thumbnail )
		{
			strcat( psString, "Thumbnail, " );
		}
		if ( ulValue & kNkMAIDDataObjType_File )
		{
			strcat( psString, "File " );
		}
		strcat( psString, "\n" );
		break;
	case kNkMAIDCapability_ModuleType:
		sprintf( buff, "%d : Scanner\n", kNkMAIDModuleType_Scanner );
		strcpy( psString, buff );
		sprintf( buff, "%d : Camera\n", kNkMAIDModuleType_Camera );
		strcat( psString, buff );
		break;
	case kNkMAIDCapability_WBFluorescentType:
		sprintf( buff, "%d : Sodium-vapor lamps\n", kNkWBFluorescentType_SodiumVapor );
		strcpy( psString, buff );
		sprintf( buff, "%d : Warm-white fluorescent\n", kNkWBFluorescentType_WarmWhite );
		strcat( psString, buff );
		sprintf( buff, "%d : White fluorescent\n", kNkWBFluorescentType_White );
		strcat( psString, buff );
		sprintf( buff, "%d : Cool-white fluorescent\n", kNkWBFluorescentType_CoolWhite );
		strcat( psString, buff );
		sprintf( buff, "%d : Day white fluorescent\n", kNkWBFluorescentType_DayWhite );
		strcat( psString, buff );
		sprintf( buff, "%d : Daylight fluorescent\n", kNkWBFluorescentType_Daylight );
		strcat( psString, buff );
		sprintf( buff, "%d : High temp. mercury-vapor\n", kNkWBFluorescentType_HiTempMercuryVapor );
		strcat( psString, buff );
		break;
	case kNkMAIDCapability_LiveViewProhibit:
		strcpy( psString, "\0" );
		if ( ulValue & kNkMAIDLiveViewProhibit_ApertureRing )
		{
			strcat( psString, "ApertureRing, " );
		}
		if ( ulValue & kNkMAIDLiveViewProhibit_TTL )
		{
			strcat( psString, "TTL, " );
		}
		if ( ulValue & kNkMAIDLiveViewProhibit_Battery )
		{
			strcat( psString, "Battery, " );
		}
		if ( ulValue & kNkMAIDLiveViewProhibit_Mirrorup )
		{
			strcat( psString, "Mirrorup, " );
		}
		if ( ulValue & kNkMAIDLiveViewProhibit_Bulb )
		{
			strcat( psString, "Bulb, " );
		}
		if ( ulValue & kNkMAIDLiveViewProhibit_FEE )
		{
			strcat( psString, "FEE, " );
		}
		if ( ulValue & kNkMAIDLiveViewProhibit_Button )
		{
			strcat( psString, "Button, " );
		}
		if ( ulValue & kNkMAIDLiveViewProhibit_Sequence )
		{
			strcat( psString, "Sequence, " );
		}
		if ( ulValue & kNkMAIDLiveViewProhibit_CF )
		{
			strcat( psString, "CF, " );
		}
		strcat( psString, "\n" );
		break;
	case kNkMAIDCapability_LiveViewMode:
		sprintf( buff, "%d : Hand-held\n", kNkMAIDLiveViewMode_Handheld );
		strcpy( psString, buff );
		sprintf( buff, "%d : Tripod\n", kNkMAIDLiveViewMode_Tripod );
		strcat( psString, buff );
		break;
	case kNkMAIDCapability_LiveViewStatus:
		sprintf( buff, "%d : OFF\n", kNkMAIDLiveViewStatus_OFF );
		strcpy( psString, buff );
		sprintf( buff, "%d : ON\n", kNkMAIDLiveViewStatus_ON );
		strcat( psString, buff );
		break;
	default:
		psString = "";
		break; 
	}
	return psString;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Distribute the function according to array type.
_BOOL SetEnumCapability( LPRefObj pRefObj, ULONG ulCapID )
{
	_BOOL	bRet;
	NkMAIDEnum	stEnum;
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, ulCapID );
	if ( pCapInfo == NULL ) return false;

	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_Enum ) return false;
	// check if this capability suports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Get ) ) return false;

	bRet = Command_CapGet( pRefObj->pObject, ulCapID, kNkMAIDDataType_EnumPtr, (NKPARAM)&stEnum, NULL, NULL );
	if( bRet == false ) return false;

	switch ( stEnum.ulType ) {
		case kNkMAIDArrayType_Unsigned:
			return SetEnumUnsignedCapability( pRefObj, ulCapID, &stEnum );
			break;
		case kNkMAIDArrayType_PackedString:
			return SetEnumPackedStringCapability( pRefObj, ulCapID, &stEnum );
			break;
		case kNkMAIDArrayType_String:
			return SetEnumStringCapability( pRefObj, ulCapID, &stEnum );
			break;
		default:
			return false;
	}
}

//------------------------------------------------------------------------------------------------------------------------------------
// Show the current setting of a Enum(Unsigned Integer) type capability and set a value for it.
_BOOL SetEnumUnsignedCapability( LPRefObj pRefObj, ULONG ulCapID, LPNkMAIDEnum pstEnum )
{
	_BOOL	bRet;
	char	psString[32], buf[256];
	UWORD	wSel;
	ULONG	i;
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, ulCapID );
	if ( pCapInfo == NULL ) return false;

	// check the data of the capability.
	if ( pstEnum->wPhysicalBytes != 4 ) return false;

	// check if this capability has elements.
	if( pstEnum->ulElements == 0 )
	{
		// This capablity has no element and is not available.
		printf( "There is no element in this capability. Enter '0' to exit.\n>" );
		scanf( "%s", buf );
		return true;
	}

	// allocate memory for array data
	pstEnum->pData = malloc( pstEnum->ulElements * pstEnum->wPhysicalBytes );
	if ( pstEnum->pData == NULL ) return false;
	// get array data
	bRet = Command_CapGetArray( pRefObj->pObject, ulCapID, kNkMAIDDataType_EnumPtr, (NKPARAM)pstEnum, NULL, NULL );
	if( bRet == false ) {
		free( pstEnum->pData );
		return false;
	}

	// show selectable items for this capability and current setting
	printf( "[%s]\n", pCapInfo->szDescription );
	
	for ( i = 0; i < pstEnum->ulElements; i++ )
		printf( "%2d. %s\n", i + 1, GetEnumString( ulCapID, ((ULONG*)pstEnum->pData)[i], psString ) );
	printf( "Current Setting: %d\n", pstEnum->ulValue + 1 );

	// check if this capability suports CapSet operation.
	if ( CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Set ) ) {
		// This capablity can be set.
		printf( "Input new value\n>" );
		scanf( "%s", buf );
		wSel = atoi( buf );
		if ( wSel > 0 && wSel <= pstEnum->ulElements ) {
			pstEnum->ulValue = wSel - 1;
			// send the selected number
			bRet = Command_CapSet( pRefObj->pObject, ulCapID, kNkMAIDDataType_EnumPtr, (NKPARAM)pstEnum, NULL, NULL );
			// This statement can be changed as follows.
			//bRet = Command_CapSet( pRefObj->pObject, ulCapID, kNkMAIDDataType_Unsigned, (NKPARAM)pstEnum->ulValue, NULL, NULL );
			if( bRet == false ) {
				free( pstEnum->pData );
				return false;
			}
		}
	} else {
		// This capablity is read-only.
		printf( "This value cannot be changed. Enter '0' to exit.\n>" );
		scanf( "%s", buf );
	}
	free( pstEnum->pData );
	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Show the current setting of a Enum(Packed String) type capability and set a value for it.
_BOOL SetEnumPackedStringCapability( LPRefObj pRefObj, ULONG ulCapID, LPNkMAIDEnum pstEnum)
{
	_BOOL	bRet;
	char	*psStr, buf[256];
	UWORD	wSel;
	ULONG	i, ulCount = 0;
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, ulCapID );
	if ( pCapInfo == NULL ) return false;

	// check the data of the capability.
	if ( pstEnum->wPhysicalBytes != 1 ) return false;

	// check if this capability has elements.
	if( pstEnum->ulElements == 0 )
	{
		// This capablity has no element and is not available.
		printf( "There is no element in this capability. Enter '0' to exit.\n>" );
		scanf( "%s", buf );
		return true;
	}

	// allocate memory for array data
	pstEnum->pData = malloc( pstEnum->ulElements * pstEnum->wPhysicalBytes );
	if ( pstEnum->pData == NULL ) return false;
	// get array data
	bRet = Command_CapGetArray( pRefObj->pObject, ulCapID, kNkMAIDDataType_EnumPtr, (NKPARAM)pstEnum, NULL, NULL );
	if( bRet == false ) {
		free( pstEnum->pData );
		return false;
	}

	// show selectable items for this capability and current setting
	printf( "[%s]\n", pCapInfo->szDescription );
	for ( i = 0; i < pstEnum->ulElements; ) {
		psStr = (char*)((ULONG)pstEnum->pData + i);
		printf( "%2d. %s\n", ++ulCount, psStr );
		i += strlen( psStr ) + 1;
	}
	printf( "Current Setting: %d\n", pstEnum->ulValue + 1 );

	// check if this capability suports CapSet operation.
	if ( CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Set ) ) {
		// This capablity can be set.
		printf( "Input new value\n>" );
		scanf( "%s", buf );
		wSel = atoi( buf );
		if ( wSel > 0 && wSel <= pstEnum->ulElements ) {
			pstEnum->ulValue = wSel - 1;
			// send the selected number
			bRet = Command_CapSet( pRefObj->pObject, ulCapID, kNkMAIDDataType_EnumPtr, (NKPARAM)pstEnum, NULL, NULL );
			// This statement can be changed as follows.
			//bRet = Command_CapSet( pRefObj->pObject, ulCapID, kNkMAIDDataType_Unsigned, (NKPARAM)pstEnum->ulValue, NULL, NULL );
			if( bRet == false ) {
				free( pstEnum->pData );
				return false;
			}
		}
	} else {
		// This capablity is read-only.
		printf( "This value cannot be changed. Enter '0' to exit.\n>" );
		scanf( "%s", buf );
	}
	free( pstEnum->pData );
	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Show the current setting of a Enum(String Integer) type capability and set a value for it.
_BOOL SetEnumStringCapability( LPRefObj pRefObj, ULONG ulCapID, LPNkMAIDEnum pstEnum )
{
	_BOOL	bRet;
	char	buf[256];
	UWORD	wSel;
	ULONG	i;
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, ulCapID );
	if ( pCapInfo == NULL ) return false;

	// check the data of the capability.
	if ( pstEnum->wPhysicalBytes != 256 ) return false;

	// check if this capability has elements.
	if( pstEnum->ulElements == 0 )
	{
		// This capablity has no element and is not available.
		printf( "There is no element in this capability. Enter '0' to exit.\n>" );
		scanf( "%s", buf );
		return true;
	}

	// allocate memory for array data
	pstEnum->pData = malloc( pstEnum->ulElements * pstEnum->wPhysicalBytes );
	if ( pstEnum->pData == NULL ) return false;
	// get array data
	bRet = Command_CapGetArray( pRefObj->pObject, ulCapID, kNkMAIDDataType_EnumPtr, (NKPARAM)pstEnum, NULL, NULL );
	if( bRet == false ) {
		free( pstEnum->pData );
		return false;
	}

	// show selectable items for this capability and current setting
	printf( "[%s]\n", pCapInfo->szDescription );
	for ( i = 0; i < pstEnum->ulElements; i++ )
		printf( "%2d. %s\n", i + 1, ((NkMAIDString*)pstEnum->pData)[i].str );
	printf( "Current Setting: %2d\n", pstEnum->ulValue + 1 );

	// check if this capability suports CapSet operation.
	if ( CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Set ) ) {
		// This capablity can be set.
		printf( "Input new value\n>" );
		scanf( "%s", buf );
		wSel = atoi( buf );
		if ( wSel > 0 && wSel <= pstEnum->ulElements ) {
			pstEnum->ulValue = wSel - 1;
			// send the selected number
			bRet =Command_CapSet( pRefObj->pObject, ulCapID, kNkMAIDDataType_EnumPtr, (NKPARAM)pstEnum, NULL, NULL );
			// This statement can be changed as follows.
			//bRet = Command_CapSet( pRefObj->pObject, ulCapID, kNkMAIDDataType_Unsigned, (NKPARAM)pstEnum->ulValue, NULL, NULL );
			if( bRet == false ) {
				free( pstEnum->pData );
				return false;
			}
		}
	} else {
		// This capablity is read-only.
		printf( "This value cannot be changed. Enter '0' to exit.\n>" );
		scanf( "%s", buf );
	}
	free( pstEnum->pData );
	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Show the current setting of a Integer type capability and set a value for it.
_BOOL SetIntegerCapability( LPRefObj pRefObj, ULONG ulCapID )
{
	_BOOL	bRet;
	SLONG	lValue;
	char	buf[256];
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, ulCapID );
	if ( pCapInfo == NULL ) return false;

	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_Integer ) return false;
	// check if this capability suports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Get ) ) return false;

	bRet = Command_CapGet( pRefObj->pObject, ulCapID, kNkMAIDDataType_IntegerPtr, (NKPARAM)&lValue, NULL, NULL );
	if( bRet == false ) return false;

	// show current value of this capability
	printf( "[%s]\n", pCapInfo->szDescription );
	printf( "Current Value: %d\n", lValue );

	if ( CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Set ) ) {
		// This capablity can be set.
		printf( "Input new value\n>" );
		scanf( "%s", buf );
		lValue = atol( buf );
		bRet = Command_CapSet( pRefObj->pObject, ulCapID, kNkMAIDDataType_Integer, (NKPARAM)lValue, NULL, NULL );
		if( bRet == false ) return false;
	} else {
		// This capablity is read-only.
		printf( "This value cannot be changed. Enter '0' to exit.\n>" );
		scanf( "%s", buf );
	}
	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Show the current setting of a Unsigned Integer type capability and set a value for it.
_BOOL SetUnsignedCapability( LPRefObj pRefObj, ULONG ulCapID )
{
	_BOOL	bRet;
	ULONG	ulValue;
	char	buf[256];
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, ulCapID );
	if ( pCapInfo == NULL ) return false;

	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_Unsigned ) return false;
	// check if this capability suports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Get ) ) return false;

	bRet = Command_CapGet( pRefObj->pObject, ulCapID, kNkMAIDDataType_UnsignedPtr, (NKPARAM)&ulValue, NULL, NULL );
	if( bRet == false ) return false;
	// show current value of this capability
	printf( "[%s]\n", pCapInfo->szDescription );
	printf( "%s", GetUnsignedString( ulCapID, ulValue, buf ) );
	printf( "Current Value: %d\n", ulValue );

	if ( CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Set ) ) {
		// This capablity can be set.
		printf( "Input new value\n>" );
		scanf( "%s", buf );
		ulValue = atol( buf );
		bRet = Command_CapSet( pRefObj->pObject, ulCapID, kNkMAIDDataType_Unsigned, (NKPARAM)ulValue, NULL, NULL );
		if( bRet == false ) return false;
	} else {
		// This capablity is read-only.
		printf( "This value cannot be changed. Enter '0' to exit.\n>" );
		scanf( "%s", buf );
	}
	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Show the current setting of a Float type capability and set a value for it.
_BOOL SetFloatCapability( LPRefObj pRefObj, ULONG ulCapID )
{
	_BOOL	bRet;
	double	lfValue;
	char	buf[256];
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, ulCapID );
	if ( pCapInfo == NULL ) return false;

	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_Float ) return false;
	// check if this capability suports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Get ) ) return false;

	bRet = Command_CapGet( pRefObj->pObject, ulCapID, kNkMAIDDataType_FloatPtr, (NKPARAM)&lfValue, NULL, NULL );
	if( bRet == false ) return false;
	// show current value of this capability
	printf( "[%s]\n", pCapInfo->szDescription );
	printf( "Current Value: %f\n", lfValue );

	if ( CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Set ) ) {
		// This capablity can be set.
		printf( "Input new value\n>" );
		scanf( "%s", buf );
		lfValue = atof( buf );
		bRet = Command_CapSet( pRefObj->pObject, ulCapID, kNkMAIDDataType_FloatPtr, (NKPARAM)&lfValue, NULL, NULL );
		if( bRet == false ) return false;
	} else {
		// This capablity is read-only.
		printf( "This value cannot be changed. Enter '0' to exit.\n>" );
		scanf( "%s", buf );
	}
	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Show the current setting of a String type capability and set a value for it.
_BOOL SetStringCapability( LPRefObj pRefObj, ULONG ulCapID )
{
	_BOOL	bRet;
	NkMAIDString	stString;
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, ulCapID );
	if ( pCapInfo == NULL ) return false;

	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_String ) return false;
	// check if this capability suports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Get ) ) return false;

	bRet = Command_CapGet( pRefObj->pObject, ulCapID, kNkMAIDDataType_StringPtr, (NKPARAM)&stString, NULL, NULL );
	if( bRet == false ) return false;
	// show current value of this capability
	printf( "[%s]\n", pCapInfo->szDescription );
	printf( "Current String: %s\n", stString.str );

	if ( CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Set ) ) {
		// This capablity can be set.
		printf( "Input new string\n>" );
#ifdef _WINDOWS
		rewind(stdin);		// clear stdin
#else		
		gets( (char*)stString.str );
#endif
		gets( (char*)stString.str );
		bRet = Command_CapSet( pRefObj->pObject, ulCapID, kNkMAIDDataType_StringPtr, (NKPARAM)&stString, NULL, NULL );
		if( bRet == false ) return false;
	} else {
		// This capablity is read-only.
		printf( "This value cannot be changed. Enter '0' to exit.\n>" );
		scanf( "%s", stString.str );
	}
	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Show the current setting of a Size type capability and set a value for it.
_BOOL SetSizeCapability( LPRefObj pRefObj, ULONG ulCapID )
{
	_BOOL	bRet;
	NkMAIDSize	stSize;
	char	buf[256];
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, ulCapID );
	if ( pCapInfo == NULL ) return false;

	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_Size ) return false;
	// check if this capability suports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Get ) ) return false;

	bRet = Command_CapGet( pRefObj->pObject, ulCapID, kNkMAIDDataType_SizePtr, (NKPARAM)&stSize, NULL, NULL );
	if( bRet == false ) return false;
	// show current value of this capability
	printf( "[%s]\n", pCapInfo->szDescription );
	printf( "Current Size: Width = %d    Height = %d\n", stSize.w, stSize.h );

	if ( CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Set ) ) {
		// This capablity can be set.
		printf( "Input Width\n>" );
		scanf( "%s", buf );
		stSize.w = atol( buf );
		printf( "Input Height\n>" );
		scanf( "%s", buf );
		stSize.h = atol( buf );
		bRet = Command_CapSet( pRefObj->pObject, ulCapID, kNkMAIDDataType_SizePtr, (NKPARAM)&stSize, NULL, NULL );
		if( bRet == false ) return false;
	} else {
		// This capablity is read-only.
		printf( "This value cannot be changed. Enter '0' to exit.\n>" );
		scanf( "%s", buf );
	}
	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Show the current setting of a DateTime type capability and set a value for it.
_BOOL SetDateTimeCapability( LPRefObj pRefObj, ULONG ulCapID )
{
	_BOOL	bRet;
	NkMAIDDateTime	stDateTime;
	char	buf[256];
	UWORD	wValue;
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, ulCapID );
	if ( pCapInfo == NULL ) return false;

	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_DateTime ) return false;
	// check if this capability suports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Get ) ) return false;

	bRet = Command_CapGet( pRefObj->pObject, ulCapID, kNkMAIDDataType_DateTimePtr, (NKPARAM)&stDateTime, NULL, NULL );
	if( bRet == false ) return false;
	// show current value of this capability
	printf( "[%s]\n", pCapInfo->szDescription );
	printf( "Current DateTime: %d/%02d/%4d %d:%02d:%02d\n",
		stDateTime.nMonth + 1, stDateTime.nDay, stDateTime.nYear, stDateTime.nHour, stDateTime.nMinute, stDateTime.nSecond );

	if ( CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Set ) ) {
		// This capablity can be set.
		printf( "Input Month(1-12) or Cancel:'c'\n>" );
		scanf( "%s", buf );
		if ( *buf == 'c' || *buf == 'C') return true;
		wValue = atoi( buf );
		if ( wValue >= 1 && wValue <= 12 )
			stDateTime.nMonth = wValue - 1;

		printf( "Input Day(1-31) or Cancel:'c'\n>" );
		scanf( "%s", buf );
		if ( *buf == 'c' || *buf == 'C') return true;
		wValue = atoi( buf );
		if ( wValue >= 1 && wValue <= 31 )
			stDateTime.nDay = wValue;

		printf( "Input Year(4 digits) or Cancel:'c'\n>" );
		scanf( "%s", buf );
		if ( *buf == 'c' || *buf == 'C') return true;
		wValue = atoi( buf );
		if ( wValue > 0 )
			stDateTime.nYear = wValue;

		printf( "Input Hour(0-23) or Cancel:'c'\n>" );
		scanf( "%s", buf );
		if ( *buf == 'c' || *buf == 'C') return true;
		wValue = atoi( buf );
		if ( wValue >= 0 && wValue <= 23 )
			stDateTime.nHour = wValue;

		printf( "Input Minute(0-59) or Cancel:'c'\n>" );
		scanf( "%s", buf );
		if ( *buf == 'c' || *buf == 'C') return true;
		wValue = atoi( buf );
		if ( wValue >= 0 && wValue <= 59 )
			stDateTime.nMinute = wValue;

		printf( "Input Second(0-59) or Cancel:'c'\n>" );
		scanf( "%s", buf );
		if ( *buf == 'c' || *buf == 'C') return true;
		wValue = atoi( buf );
		if ( wValue >= 0 && wValue <= 59 )
			stDateTime.nSecond = wValue;

		bRet = Command_CapSet( pRefObj->pObject, ulCapID, kNkMAIDDataType_DateTimePtr, (NKPARAM)&stDateTime, NULL, NULL );
		if( bRet == false ) return false;
	} else {
		// This capablity is read-only.
		printf( "This value cannot be changed. Enter '0' to exit.\n>" );
		scanf( "%s", buf );
	}
	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Show the current setting of a Boolean type capability and set a value for it.
_BOOL SetBoolCapability( LPRefObj pRefObj, ULONG ulCapID )
{
	_BOOL	bRet;
	BYTE	bFlag;
	char	buf[256];
	UWORD	wSel;
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, ulCapID );
	if ( pCapInfo == NULL ) return false;

	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_Boolean ) return false;
	// check if this capability suports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Get ) ) return false;

	bRet = Command_CapGet( pRefObj->pObject, ulCapID, kNkMAIDDataType_BooleanPtr, (NKPARAM)&bFlag, NULL, NULL );
	if( bRet == false ) return false;
	// show current setting of this capability
	printf( "[%s]\n", pCapInfo->szDescription );
	printf( "1. On      2. Off\n" );
	printf( "Current Setting: %d\n", bFlag ? 1 : 2 );

	if ( CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Set ) ) {
		// This capablity can be set.
		printf( "Input '1' or '2'\n>" );
		scanf( "%s", buf );
		wSel = atoi( buf );
		if ( (wSel == 1) || (wSel == 2) ) {
			bFlag = (wSel == 1) ? true : false;
			bRet = Command_CapSet( pRefObj->pObject, ulCapID, kNkMAIDDataType_Boolean, (NKPARAM)bFlag, NULL, NULL );
			if( bRet == false ) return false;
		}
	} else {
		// This capablity is read-only.
		printf( "This value cannot be changed. Enter '0' to exit.\n>" );
		scanf( "%s", buf );
	}
	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Show the current setting of a Range type capability and set a value for it.
_BOOL SetRangeCapability( LPRefObj pRefObj, ULONG ulCapID )
{
	_BOOL	bRet;
	NkMAIDRange	stRange;
	double	lfValue;
	char	buf[256];
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, ulCapID );
	if ( pCapInfo == NULL ) return false;

	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_Range ) return false;
	// check if this capability suports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Get ) ) return false;

	bRet = Command_CapGet( pRefObj->pObject, ulCapID, kNkMAIDDataType_RangePtr, (NKPARAM)&stRange, NULL, NULL );
	if( bRet == false ) return false;
	// show current value of this capability
	printf( "[%s]\n", pCapInfo->szDescription );
	
	// check if this capability suports CapSet operation.
	if ( CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Set ) ) {
		// This capablity can be set.
		if ( stRange.ulSteps == 0 ) {
			// the value of this capability is set to 'lfValue' directly
			printf( "Current Value: %f  (Max: %f  Min: %f)\n", stRange.lfValue, stRange.lfUpper, stRange.lfLower );
			printf( "Input new value.\n>" );
			scanf( "%s", buf );
			stRange.lfValue = atof( buf );
		} else {
			// the value of this capability is calculated from 'ulValueIndex'
			lfValue = stRange.lfLower + stRange.ulValueIndex * (stRange.lfUpper - stRange.lfLower) / (stRange.ulSteps - 1);
			printf( "Current Value: %f  (Max: %f  Min: %f)\n", lfValue, stRange.lfUpper, stRange.lfLower );
			printf( "Input new value.\n>" );
			scanf( "%s", buf );
			lfValue = atof( buf );
			stRange.ulValueIndex = (ULONG)((lfValue - stRange.lfLower) * (stRange.ulSteps - 1) / (stRange.lfUpper - stRange.lfLower));
		}
		bRet = Command_CapSet( pRefObj->pObject, ulCapID, kNkMAIDDataType_RangePtr, (NKPARAM)&stRange, NULL, NULL );
		if( bRet == false ) return false;
	} else {
		// This capablity is read-only.
		if ( stRange.ulSteps == 0 ) {
			// the value of this capability is set to 'lfValue' directly
			lfValue = stRange.lfValue;
		} else {
			// the value of this capability is calculated from 'ulValueIndex'
			lfValue = stRange.lfLower + stRange.ulValueIndex * (stRange.lfUpper - stRange.lfLower) / (stRange.ulSteps - 1);
		}
		printf( "Current Value: %f  (Max: %f  Min: %f)\n", lfValue, stRange.lfUpper, stRange.lfLower );
		printf( "This value cannot be changed. Enter '0' to exit.\n>" );
		scanf( "%s", buf );
	}
	return true;
}

//------------------------------------------------------------------------------------------------------------------------------------
// Set White balance preset data.
_BOOL SetWBPresetDataCapability( LPRefObj pRefSrc )
{
	char	buf[256], filename[256];
	NkMAIDWBPresetData	stPresetData;
	LPNkMAIDCapInfo		pCapInfo = NULL;
	FILE	*stream;
	int		count = 0;
	ULONG   ulTotal = 0;
	char	*ptr = NULL;
	_BOOL	bRet;

	strcpy( filename, "PresetData.jpg" );

	while (1)
	{
		// Preset Number
		printf( "\nSelect Preset Number(1-5, 0)\n");
		printf( " 1. d-0\n");
		printf( " 2. d-1\n");
		printf( " 3. d-2\n");
		printf( " 4. d-3\n");
		printf( " 5. d-4\n");
		printf( " 0. Exit\n>" );
		scanf( "%s", buf );
		stPresetData.ulPresetNumber = atoi( buf );
		if (stPresetData.ulPresetNumber == 0) return true; //Exit
		if ( 1 > stPresetData.ulPresetNumber || stPresetData.ulPresetNumber > 5 ) 
		{
			printf("Invalid Preset Number.\n");
			continue;
		}
		break;
	}
	stPresetData.ulPresetNumber -= 1;// PresetNo start from 0.

	// Preset gain
	printf( "\nSet preset gain value by decimal, or Exit(0).\n>" );
	scanf( "%s", buf );
	stPresetData.ulPresetGain = atoi( buf );
	if (stPresetData.ulPresetGain == 0) return true; //Exit


	// Check operations
	pCapInfo = GetCapInfo( pRefSrc, kNkMAIDCapability_WBPresetData );
	if ( pCapInfo == NULL ) return false;

	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_Generic ) return false;
	// check if this capability suports CapSet operation.
	if ( !CheckCapabilityOperation( pRefSrc, kNkMAIDCapability_WBPresetData, kNkMAIDCapOperation_Set ) ) return false;

	// Read preset data from file.
	if ( (stream = fopen(filename, "rb") ) == NULL ) 
	{
		printf( "\nfile open error.\n" );
		return false;
	}

	// Max preset data size id 13312
	// Allocate memory for preset data.
	ptr = (char*)malloc(14000);
	stPresetData.pThumbnailData = ptr;
	while ( !feof( stream ) )
	{
		// Read file until eof.
		count = fread( ptr, sizeof( char ), 100, stream );
		if ( ferror( stream ) )
		{
			printf( "\nfile read error.\n" );
			fclose( stream );
			free( stPresetData.pThumbnailData );
			return false;
		}
		/* Total up actual bytes read */
		ulTotal += count;
		ptr += count;
		if (13312 < ulTotal)
		{

			printf( "\nThe size of \"PresetData.jpg\" is over 13312 byte.\n" );
			fclose( stream );
			free( stPresetData.pThumbnailData );
			return false;
		}
	}
	stPresetData.ulThumbnailSize = ulTotal;

	// Set preset data.
	bRet = Command_CapSet( pRefSrc->pObject, kNkMAIDCapability_WBPresetData, kNkMAIDDataType_GenericPtr, (NKPARAM)&(stPresetData), NULL, NULL );
	
	fclose( stream );
	free( stPresetData.pThumbnailData );

	return bRet;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Get preview image Low/Normal
_BOOL GetPreviewImageCapability( LPRefObj pRefSrc, ULONG ulItmID )
{
	UWORD	wSel;
	char	buf[256], HeaderFileName[256], ImageFileName[256];
	FILE*	hFileHeader = NULL;		// Preview Image file name
	FILE*	hFileImage = NULL;		// Preview header file name
	ULONG	ulHeaderSize = 32;		//The header size of PreviewImage
	NkMAIDArray	stArray;
	int i = 0;
	unsigned char* pucData = NULL;	// Preview data pointer
	_BOOL	bRet = true;
	
	do {
		memset( &stArray, 0, sizeof(NkMAIDArray) );

		// Select Low or Normal
		printf( "\nSelect Preview quality(1-2, 0)\n");
		printf( " 1. Low\n" );
		printf( " 2. Normal\n" );
		printf( " 0. Exit\n>" );
		scanf( "%s", buf );
		wSel = atoi( buf );
	
		switch( wSel )
		{
		case 1:// GetPreviewImageLow
		case 2:// GetPreviewImageNormal
			// Set Preview ID
			bRet = Command_CapSet( pRefSrc->pObject, kNkMAIDCapability_CurrentPreviewID, kNkMAIDDataType_Unsigned, (NKPARAM)ulItmID, NULL, NULL );
			if (bRet)
			{
				if (wSel == 1 ) 
				{	// Preview Low
					bRet = GetArrayCapability( pRefSrc, kNkMAIDCapability_GetPreviewImageLow, &stArray );
				}
				else
				{	// Preview Normal
					bRet = GetArrayCapability( pRefSrc, kNkMAIDCapability_GetPreviewImageNormal, &stArray );
				}
				if ( bRet == false ) break;
				
				// create file name
				while( true )
				{
					sprintf( HeaderFileName, "Preview%03d_H.%s", ++i, "dat" );
					sprintf( ImageFileName, "Preview%03d.%s", i, "jpg" );
					if ( (hFileHeader = fopen(HeaderFileName, "r") ) != NULL ||
						    (hFileImage  = fopen(ImageFileName, "r") )  != NULL    )
					{
						// this file name is already used.
						if (hFileHeader)
						{
							fclose( hFileHeader );
							hFileHeader = NULL;
						}
						if (hFileImage)
						{
							fclose( hFileImage );
							hFileImage = NULL;		
						}
					}	 
					else
					{
						break;
					}
				}
				
				// open file
				hFileHeader = fopen( HeaderFileName, "wb" );
				if ( hFileHeader == NULL )
				{
					printf("file open error.\n");
					break;
				}				
				hFileImage = fopen( ImageFileName, "wb" );
				if ( hFileImage == NULL )
				{
					printf("file open error.\n");
					break;
				}
				
				// Get data pointer
				pucData = (unsigned char*)stArray.pData;

				// write file
				if ( hFileHeader && hFileImage )
				{
					fwrite( pucData, 1, ulHeaderSize, hFileHeader );
					fwrite( pucData+ulHeaderSize, 1, (stArray.ulElements-ulHeaderSize), hFileImage );
					printf("\n%s was saved.\n", HeaderFileName);
					printf("%s was saved.\n", ImageFileName);
				}				
			}
			break;
		default:
			wSel = 0; // Exit
		}

		if ( bRet == false ) {
			printf( "An Error occured.\n" );
		}
		
		// close file
		if (hFileHeader)
		{
			fclose( hFileHeader );
			hFileHeader = NULL;
		}
		if (hFileImage)
		{
			fclose( hFileImage );
			hFileImage = NULL;		
		}

		// free alloc data
		if ( stArray.pData != NULL )
		{
			free( stArray.pData );
			stArray.pData = NULL;
		}

	} while( wSel != 0 );

	return true;
}

//------------------------------------------------------------------------------------------------------------------------------------
//Delete Dram Image
_BOOL DeleteDramCapability( LPRefObj pRefItem, ULONG ulItmID )
{
	LPRefObj	pRefSrc = (LPRefObj)pRefItem->pRefParent;
	LPRefObj	pRefDat = NULL;
	_BOOL	bRet = true;
	NkMAIDCallback	stProc;
	LPRefDataProc	pRefDeliver;
	LPRefCompletionProc	pRefCompletion;
	ULONG	ulCount = 0L;


	// 1. Open ImageObject
	pRefDat = GetRefChildPtr_ID( pRefItem, kNkMAIDDataObjType_Image );
	if ( pRefDat == NULL )
	{
		// Create Image object and RefSrc structure.
		if ( AddChild( pRefItem, kNkMAIDDataObjType_Image ) == false )
		{
			printf("Image object can't be opened.\n");
			return false;
		}
		pRefDat = GetRefChildPtr_ID( pRefItem, kNkMAIDDataObjType_Image );
	}

	// 2. Set DataProc function
	// 2-1. set reference from DataProc
	pRefDeliver = (LPRefDataProc)malloc( sizeof(RefDataProc) );// this block will be freed in CompletionProc.
	pRefDeliver->pBuffer = NULL;
	pRefDeliver->ulOffset = 0L;
	pRefDeliver->ulTotalLines = 0L;
	pRefDeliver->lID = pRefItem->lMyID;
	// 2-2. set reference from CompletionProc
	pRefCompletion = (LPRefCompletionProc)malloc( sizeof(RefCompletionProc) );// this block will be freed in CompletionProc.
	pRefCompletion->pulCount = &ulCount;
	pRefCompletion->pRef = pRefDeliver;
	// 2-3. set reference from DataProc
	stProc.pProc = (LPNKFUNC)DataProc;
	stProc.refProc = (NKREF)pRefDeliver;
	// 2-4. set DataProc as data delivery callback function
	if( CheckCapabilityOperation( pRefDat, kNkMAIDCapability_DataProc, kNkMAIDCapOperation_Set ) )
	{
		bRet = Command_CapSet( pRefDat->pObject, kNkMAIDCapability_DataProc, kNkMAIDDataType_CallbackPtr, (NKPARAM)&stProc, NULL, NULL );
		if ( bRet == false ) return false;
	} 
	else
	{
		return false;
	}
		
	// 3. Acquire image
	bRet = Command_CapStart( pRefDat->pObject, kNkMAIDCapability_Acquire, (LPNKFUNC)CompletionProc, (NKREF)pRefCompletion );
	if ( bRet == false ) return false;
	
	// 4. Async command
	bRet = Command_Async( pRefDat->pObject );
	if ( bRet == false ) return false;

	// 5. Abort
	bRet = Command_Abort( pRefDat->pObject, NULL, NULL);
	if ( bRet == false ) return false;
	
	// 6. Set Preview ID
	bRet = Command_CapSet( pRefSrc->pObject, kNkMAIDCapability_CurrentPreviewID, kNkMAIDDataType_Unsigned, (NKPARAM)ulItmID, NULL, NULL );
	if ( bRet == false ) return false;

	// 7. Delete DRAM
	bRet = Command_CapStart( pRefSrc->pObject, kNkMAIDCapability_DeleteDramImage, NULL, NULL );
	if ( bRet == false ) return false;

	// 8. Reset DataProc
	bRet = Command_CapSet( pRefDat->pObject, kNkMAIDCapability_DataProc, kNkMAIDDataType_Null, (NKPARAM)NULL, NULL, NULL );
	if ( bRet == false ) return false;

	printf("\nInternal ID [0x%08X] was deleted.\n", ulItmID );
	// Upper function to close ItemObject. 
	g_bFileRemoved = true;
	// progress proc flag reset 
	g_bFirstCall = true;

	// 9. Close ImageObject
	bRet = RemoveChild( pRefItem, kNkMAIDDataObjType_Image );


	return bRet;
}

//------------------------------------------------------------------------------------------------------------------------------------
// Get Live view image
_BOOL GetLiveViewImageCapability( LPRefObj pRefSrc )
{
	char	HeaderFileName[256], ImageFileName[256];
	FILE*	hFileHeader = NULL;		// LiveView Image file name
	FILE*	hFileImage = NULL;		// LiveView header file name
	ULONG	ulHeaderSize = 64;		//The header size of LiveView
	NkMAIDArray	stArray;
	int i = 0;
	unsigned char* pucData = NULL;	// LiveView data pointer
	_BOOL	bRet = true;


	memset( &stArray, 0, sizeof(NkMAIDArray) );		
	
	bRet = GetArrayCapability( pRefSrc, kNkMAIDCapability_GetLiveViewImage, &stArray );
	if ( bRet == false ) return false;
		
	// create file name
	while( true )
	{
		sprintf( HeaderFileName, "LiveView%03d_H.%s", ++i, "dat" );
		sprintf( ImageFileName, "LiveView%03d.%s", i, "jpg" );
		if ( (hFileHeader = fopen(HeaderFileName, "r") ) != NULL ||
				(hFileImage  = fopen(ImageFileName, "r") )  != NULL )
		{
			// this file name is already used.
			if (hFileHeader)
			{
				fclose( hFileHeader );
				hFileHeader = NULL;
			}
			if (hFileImage)
			{
				fclose( hFileImage );
				hFileImage = NULL;		
			}
		}	 
		else
		{
			break;
		}
	}
		
	// open file
	hFileHeader = fopen( HeaderFileName, "wb" );
	if ( hFileHeader == NULL )
	{
		printf("file open error.\n");
		return false;
	}				
	hFileImage = fopen( ImageFileName, "wb" );
	if ( hFileImage == NULL )
	{
		fclose( hFileHeader );
		printf("file open error.\n");
		return false;
	}
	
	// Get data pointer
	pucData = (unsigned char*)stArray.pData;

	// write file
	fwrite( pucData, 1, ulHeaderSize, hFileHeader );
	fwrite( pucData+ulHeaderSize, 1, (stArray.ulElements-ulHeaderSize), hFileImage );
	printf("\n%s was saved.\n", HeaderFileName);
	printf("%s was saved.\n", ImageFileName);
		
	// close file
	fclose( hFileHeader );
	fclose( hFileImage );
	free( stArray.pData );

	return true;
}

//------------------------------------------------------------------------------------------------------------------------------------
// Set/Get PictureControlDataCapability
_BOOL PictureControlDataCapability( LPRefObj pRefSrc )
{	
	char buf[256], filename[256];
	NkMAIDPicCtrlData stPicCtrlData;
	ULONG	ulSel, ulSubSel;
	_BOOL	bRet = true;

	strcpy( filename, "PicCtrlData.dat" );
	// sub command loop
	do {
		memset( &stPicCtrlData, 0, sizeof(NkMAIDPicCtrlData) );

		printf( "\nSelect (1-2, 0)\n" );
		printf( " 1. Set Picture Control data the file named \"PicCtrlData.dat\"\n" );
		printf( " 2. Get Picture Control data\n" );
		printf( " 0. Exit\n>" );
		scanf( "%s", buf );
		ulSel = atol( buf );
		switch( ulSel )
		{
			case 1://Set Picture Control data 
			{
				printf( "\nSelect Picture Control(1-17, 0)\n");
				printf( " 1. Standard                    2. Neutral\n");
				printf( " 3. Vivid                       4. Monochrome\n");
				printf( " 5. Option Picture Contol 1     6. Option Picture Contol 2 \n");
				printf( " 7. Option Picture Contol 3     8. Option Picture Contol 4 \n");
				printf( " 9. Custom Picture Contol 1    10. Custom Picture Contol 2 \n");
				printf( "11. Custom Picture Contol 3    12. Custom Picture Contol 4 \n");
				printf( "13. Custom Picture Contol 5    14. Custom Picture Contol 6 \n");
				printf( "15. Custom Picture Contol 7    16. Custom Picture Contol 8 \n");
				printf( "17. Custom Picture Contol 9\n");
				printf( " 0. Exit\n>" );
				scanf( "%s", buf );
				ulSubSel = atoi( buf );
				if ( ulSubSel == 0 ) break; //Exit
				if ( ulSubSel < 1 || ulSubSel > 17 ) 
				{
					printf("Invalid Picture Control\n");
					break;
				}

				if ( ulSubSel >= 5 && ulSubSel <= 8 )
				{
					ulSubSel += 96; // Option 101 - 104
				}
				else
				if ( ulSubSel >= 9 )
				{
					ulSubSel += 192; // Custom 201 - 209
				}
				// set target Picture Control
				stPicCtrlData.ulPicCtrlItem = ulSubSel;

				// initial registration is not supported about 1-4, 101-104 
				if ( (stPicCtrlData.ulPicCtrlItem >= 1 && stPicCtrlData.ulPicCtrlItem <= 4) 
					 || (stPicCtrlData.ulPicCtrlItem >= 101 && stPicCtrlData.ulPicCtrlItem <= 104) )
				{
					printf( "\nSelect ModifiedFlag (1, 0)\n");
					printf( " 1. edit\n");
					printf( " 0. Exit\n>" );
					scanf( "%s", buf );
					ulSubSel = atoi( buf );
					if ( ulSubSel == 0 ) break; // Exit
					if ( ulSubSel < 1 || ulSubSel > 1)
					{
						printf("Invalid ModifiedFlag\n");
						break;
					}
					// set Modification flas
					stPicCtrlData.bModifiedFlag = true;
				}
				else
				{
					printf( "\nSelect ModifiedFlag (1-2, 0)\n");
					printf( " 1. initial registration          2. edit\n");
					printf( " 0. Exit\n>" );
					scanf( "%s", buf );
					ulSubSel = atoi( buf );
					if ( ulSubSel == 0 ) break; // Exit
					if ( ulSubSel < 1 || ulSubSel > 2)
					{
						printf("Invalid ModifiedFlag\n");
						break;
					}
					// set Modification flas
					stPicCtrlData.bModifiedFlag = ( ulSubSel == 1 ) ? false : true;
				}

				bRet = SetPictureControlDataCapability( pRefSrc, &stPicCtrlData, filename );
				break;

			case 2://Get Picture Control data
				printf( "\nSelect Picture Control(1-17, 0)\n");
				printf( " 1. Standard                    2. Neutral\n");
				printf( " 3. Vivid                       4. Monochrome\n");
				printf( " 5. Option Picture Contol 1     6. Option Picture Contol 2 \n");
				printf( " 7. Option Picture Contol 3     8. Option Picture Contol 4 \n");
				printf( " 9. Custom Picture Contol 1    10. Custom Picture Contol 2 \n");
				printf( "11. Custom Picture Contol 3    12. Custom Picture Contol 4 \n");
				printf( "13. Custom Picture Contol 5    14. Custom Picture Contol 6 \n");
				printf( "15. Custom Picture Contol 7    16. Custom Picture Contol 8 \n");
				printf( "17. Custom Picture Contol 9\n");
				printf( " 0. Exit\n>" );
				scanf( "%s", buf );
				ulSubSel = atoi( buf );
				if ( ulSubSel == 0 ) break; //Exit
				if ( ulSubSel < 1 || ulSubSel > 17 ) 
				{
					printf("Invalid Picture Control\n");
					break;
				}

				if ( ulSubSel >= 5 && ulSubSel <= 8 )
				{
					ulSubSel += 96; // Option 101 - 104
				}
				else
				if ( ulSubSel >= 9 )
				{
					ulSubSel += 192; // Custom 201 - 209
				}
				// set target Picture Control
				stPicCtrlData.ulPicCtrlItem = ulSubSel;

				bRet = GetPictureControlDataCapability( pRefSrc, &stPicCtrlData );
				break;
			default:
				ulSel = 0;
			}
		}
		if ( bRet == false ) 
		{
			printf( "An Error occured. \n" );
		}
	} while( ulSel > 0 );

	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Set PictureControlDataCapability
_BOOL SetPictureControlDataCapability( LPRefObj pRefObj, NkMAIDPicCtrlData* pPicCtrlData, char* filename )
{
	_BOOL	bRet = TRUE;
	FILE	*stream;
	int		count = 0;
	ULONG   ulTotal = 0;
	char	*ptr = NULL;

	LPNkMAIDCapInfo	pCapInfo = GetCapInfo( pRefObj, kNkMAIDCapability_PictureControlData );
	if ( pCapInfo == NULL ) return false;

	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_Generic ) return false;
	// check if this capability suports CapSet operation.
	if ( !CheckCapabilityOperation( pRefObj, kNkMAIDCapability_PictureControlData, kNkMAIDCapOperation_Set ) ) return false;

	// Read preset data from file.
	if ( (stream = fopen(filename, "rb") ) == NULL ) 
	{
		printf( "\nfile open error.\n" );
		return false;
	}

	// Max Picture Control data size is 609
	// Allocate memory for Picture Control data.
	pPicCtrlData->pData = (char*)malloc(609);
	ptr = (char*)pPicCtrlData->pData;
	while ( !feof( stream ) )
	{
		// Read file until eof.
		count = fread( ptr, sizeof( char ), 100, stream );
		if ( ferror( stream ) )
		{
			printf( "\nfile read error.\n" );
			fclose( stream );
			free( pPicCtrlData->pData );
			return false;
		}
		/* Total count up actual bytes read */
		ulTotal += count;
		ptr += count;
		if (609 < ulTotal)
		{

			printf( "\nThe size of \"PicCtrlData.dat\" is over 609 byte.\n" );
			fclose( stream );
			free( pPicCtrlData->pData );
			return false;
		}
	}
	pPicCtrlData->ulSize = ulTotal;

	// Set Picture Control data.
	bRet = Command_CapSet( pRefObj->pObject, kNkMAIDCapability_PictureControlData, kNkMAIDDataType_GenericPtr, (NKPARAM)pPicCtrlData, NULL, NULL );
	if( bRet == false ) 
	{
		printf( "\nFailed in setting Picture Contol Data.\n" );
	}
	
	fclose( stream );
	free( pPicCtrlData->pData );

	return bRet;
}

//------------------------------------------------------------------------------------------------------------------------------------
// Get PictureControlDataCapability
_BOOL GetPictureControlDataCapability( LPRefObj pRefObj, NkMAIDPicCtrlData* pPicCtrlData )
{
	_BOOL	bRet = TRUE;
	FILE	*stream = NULL;
	unsigned char* pucData = NULL;	// Picture Control Data pointer

	LPNkMAIDCapInfo	pCapInfo = GetCapInfo( pRefObj, kNkMAIDCapability_PictureControlData );
	if ( pCapInfo == NULL ) return false;

	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_Generic ) return false;
	// check if this capability suports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, kNkMAIDCapability_PictureControlData, kNkMAIDCapOperation_Get ) ) return false;

	// Max Picture Control data size is 609
	// Allocate memory for Picture Control data.
	pPicCtrlData->ulSize = 609;
	pPicCtrlData->pData = (char*)malloc(609);

	// Get Picture Control data.
	bRet = Command_CapGet( pRefObj->pObject, kNkMAIDCapability_PictureControlData, kNkMAIDDataType_GenericPtr, (NKPARAM)pPicCtrlData, NULL, NULL );
	if( bRet == false ) 
	{
		printf( "\nFailed in getting Picture Control Data.\n" );
		free( pPicCtrlData->pData );
		return false;
	}

	// Save to file
	// open file
	stream = fopen( "PicCtrlData.dat", "wb" );
	if ( stream == NULL )
	{
		printf( "\nfile open error.\n" );
		free( pPicCtrlData->pData );
		return false;
	}				
		
	// Get data pointer
	pucData = (unsigned char*)pPicCtrlData->pData;

	// write file
	fwrite( pucData, 1, pPicCtrlData->ulSize, stream );
	printf("\nPicCtrlData.dat was saved.\n");
	
	// close file
	fclose( stream );
	free( pPicCtrlData->pData );

	return true;
}

//------------------------------------------------------------------------------------------------------------------------------------
// Get PictureControlInfoCapability
_BOOL GetPictureControlInfoCapability( LPRefObj pRefSrc )
{
	char buf[256], filename[256];
	NkMAIDGetPicCtrlInfo stPicCtrlInfo;
	ULONG	ulSel;
	LPNkMAIDCapInfo	pCapInfo = NULL;
	FILE* stream = NULL;
	unsigned char* pucData = NULL;	// Picture Control Info pointer
	_BOOL	bRet = true;


	strcpy( filename, "PicCtrlInfo.dat" );

	memset( &stPicCtrlInfo, 0, sizeof(NkMAIDGetPicCtrlInfo) );

	printf( "\nSelect Picture Control(1-17, 0)\n");
	printf( " 1. Standard                    2. Neutral\n");
	printf( " 3. Vivid                       4. Monochrome\n");
	printf( " 5. Option Picture Contol 1     6. Option Picture Contol 2 \n");
	printf( " 7. Option Picture Contol 3     8. Option Picture Contol 4 \n");
	printf( " 9. Custom Picture Contol 1    10. Custom Picture Contol 2 \n");
	printf( "11. Custom Picture Contol 3    12. Custom Picture Contol 4 \n");
	printf( "13. Custom Picture Contol 5    14. Custom Picture Contol 6 \n");
	printf( "15. Custom Picture Contol 7    16. Custom Picture Contol 8 \n");
	printf( "17. Custom Picture Contol 9\n");
	printf( " 0. Exit\n>" );
	scanf( "%s", buf );
	ulSel = atoi( buf );
	if ( ulSel == 0 ) return true; // Exit
	if ( ulSel < 1 || ulSel > 17 ) 
	{
		printf("Invalid Picture Control\n");
		return false;
	}

	if ( ulSel >= 5 && ulSel <= 8 )
	{
		ulSel += 96; // Option 101 - 104
	}
	else
	if ( ulSel >= 9 )
	{
		ulSel += 192; // Custom 201 - 209
	}
	// set target Picture Control
	stPicCtrlInfo.ulPicCtrlItem = ulSel;


	pCapInfo = GetCapInfo( pRefSrc, kNkMAIDCapability_GetPicCtrlInfo );
	if ( pCapInfo == NULL ) return false;

	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_Generic ) return false;
	// check if this capability suports CapGet operation.
	if ( !CheckCapabilityOperation( pRefSrc, kNkMAIDCapability_GetPicCtrlInfo, kNkMAIDCapOperation_Get ) ) return false;

	// Max Picture Control info size is 48
	// Allocate memory for Picture Control info.
	stPicCtrlInfo.ulSize = 48;
	stPicCtrlInfo.pData = (char*)malloc(48);

	// Get Picture Control info.
	bRet = Command_CapGet( pRefSrc->pObject, kNkMAIDCapability_GetPicCtrlInfo, kNkMAIDDataType_GenericPtr, (NKPARAM)&(stPicCtrlInfo), NULL, NULL );
	if( bRet == false ) 
	{
		free( stPicCtrlInfo.pData );
		return false;
	}

	// Save to file
	// open file
	stream = fopen( "PicCtrlInfo.dat", "wb" );
	if ( stream == NULL )
	{
		printf( "\nfile open error.\n" );
		free( stPicCtrlInfo.pData );
		return false;
	}				
	
	// Get data pointer
	pucData = (unsigned char*)stPicCtrlInfo.pData;

	// write file
	fwrite( pucData, 1, stPicCtrlInfo.ulSize, stream );
	printf("\nPicCtrlInfo.dat was saved.\n");
	
	// close file
	fclose( stream );
	free( stPicCtrlInfo.pData );

	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Delete Custom Picture Control
_BOOL DeleteCustomPictureControlCapability( LPRefObj pRefSrc )
{
	ULONG	ulSel = 0;
	_BOOL	bRet;
	ULONG	ulValue;
	char	buf[256];


	printf( "\nSelect Custom Picture Control to delete.(1-9, 0)\n");

	printf( "1. Custom Picture Contol 1\n");
	printf( "2. Custom Picture Contol 2\n");
	printf( "3. Custom Picture Contol 3\n");
	printf( "4. Custom Picture Contol 4\n");
	printf( "5. Custom Picture Contol 5\n");
	printf( "6. Custom Picture Contol 6\n");
	printf( "7. Custom Picture Contol 7\n");
	printf( "8. Custom Picture Contol 8\n");
	printf( "9. Custom Picture Contol 9\n");
	printf( "0. Exit\n>" );
	scanf( "%s", buf );

	ulSel = atoi( buf );
	if ( ulSel == 0 ) return true; // Exit
	if ( ulSel < 1 || ulSel > 9 ) 
	{
		printf("Invalid Custom Picture Control\n");
		return false;
	}
	ulSel += 200;		// Custom 201 - 209
	ulValue = ulSel;	// Set Custom Picture Control to delete

	bRet = Command_CapSet( pRefSrc->pObject, kNkMAIDCapability_DeleteCustomPictureControl, kNkMAIDDataType_Unsigned, (NKPARAM)ulValue, NULL, NULL );
	return bRet;
}
//------------------------------------------------------------------------------------------------------------------------------------
// read the array data from the camera and display it on the screen
_BOOL ShowArrayCapability( LPRefObj pRefObj, ULONG ulCapID )
{
	_BOOL	bRet = true;
	NkMAIDArray	stArray;
	ULONG	ulSize, i, j;
	LPNkMAIDCapInfo	pCapInfo = GetCapInfo( pRefObj, ulCapID );
	if ( pCapInfo == NULL ) return false;

	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_Array ) return false;
	// check if this capability suports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Get ) ) return false;
	bRet = Command_CapGet( pRefObj->pObject, ulCapID, kNkMAIDDataType_ArrayPtr, (NKPARAM)&stArray, NULL, NULL );
	if( bRet == false ) return false;

	ulSize = stArray.ulElements * stArray.wPhysicalBytes;
	// allocate memory for array data
	stArray.pData = malloc( ulSize );
	if ( stArray.pData == NULL ) return false;
	// get array data
	bRet = Command_CapGetArray( pRefObj->pObject, ulCapID, kNkMAIDDataType_ArrayPtr, (NKPARAM)&stArray, NULL, NULL );
	if( bRet == false ) {
		free( stArray.pData );
		return false;
	}

	// show selectable items for this capability and current setting
	printf( "[%s]\n", pCapInfo->szDescription );
	for ( i = 0, j = 0; i*16+j < ulSize; i++ ) {
		for ( ; j < 16 && i*16+j < ulSize; j++ ) {
			printf( " %02X", ((UCHAR*)stArray.pData)[i*16+j] );
		}
		j = 0;
		printf( "\n" );
	}

	if ( stArray.pData != NULL )
		free( stArray.pData );
	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------
// read the array data from the camera and save it on a storage(hard drive)
//	for kNkMAIDCapability_GetPreviewImageLow, 
//  for kNkMAIDCapability_GetPreviewImageNormal
//  for kNkMAIDCapability_GetLiveViewImage
_BOOL GetArrayCapability( LPRefObj pRefObj, ULONG ulCapID, LPNkMAIDArray pstArray )
{
	_BOOL	bRet = true;
	LPNkMAIDCapInfo	pCapInfo = GetCapInfo( pRefObj, ulCapID );
	if ( pCapInfo == NULL ) return false;

	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_Array ) return false;
	// check if this capability suports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Get ) ) return false;
	bRet = Command_CapGet( pRefObj->pObject, ulCapID, kNkMAIDDataType_ArrayPtr, (NKPARAM)pstArray, NULL, NULL );
	if( bRet == false ) return false;

	// check if this capability suports CapGetArray operation.
	if ( !CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_GetArray ) ) return false;
	// allocate memory for array data
	pstArray->pData = malloc( pstArray->ulElements * pstArray->wPhysicalBytes );
	if ( pstArray->pData == NULL ) return false;
	// get array data
	bRet = Command_CapGetArray( pRefObj->pObject, ulCapID, kNkMAIDDataType_ArrayPtr, (NKPARAM)pstArray, NULL, NULL );
	if( bRet == false ) {
		free( pstArray->pData );
		pstArray->pData = NULL;
		return false;
	}

	// show selectable items for this capability and current setting
	printf( "[%s]\n", pCapInfo->szDescription );

	// Do not free( pstArray->pData )
	// Upper class use pstArray->pData to save file.
	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------
// load the array data from a storage and send it to the camera
_BOOL LoadArrayCapability( LPRefObj pRefObj, ULONG ulCapID, char* filename )
{
	_BOOL	bRet = true;
	NkMAIDArray	stArray;
	FILE *stream;
	LPNkMAIDCapInfo	pCapInfo = GetCapInfo( pRefObj, ulCapID );
	if ( pCapInfo == NULL ) return false;

	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_Array ) return false;
	// check if this capability suports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Get ) ) return false;
	bRet = Command_CapGet( pRefObj->pObject, ulCapID, kNkMAIDDataType_ArrayPtr, (NKPARAM)&stArray, NULL, NULL );
	if( bRet == false ) return false;

	// allocate memory for array data
	stArray.pData = malloc( stArray.ulElements * stArray.wPhysicalBytes );
	if ( stArray.pData == NULL ) return false;

	// show selectable items for this capability and current setting
	printf( "[%s]\n", pCapInfo->szDescription );

	if ( (stream = fopen( filename, "rb" ) ) == NULL) {
		printf( "file not found\n" );
		if ( stArray.pData != NULL )
			free( stArray.pData );
		return false;
	}
	fread( stArray.pData, 1, stArray.ulElements * stArray.wPhysicalBytes, stream );
	fclose( stream );

	// check if this capability suports CapSet operation.
	if ( CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Set ) ) {
		// set array data
		bRet = Command_CapSet( pRefObj->pObject, ulCapID, kNkMAIDDataType_ArrayPtr, (NKPARAM)&stArray, NULL, NULL );
		if( bRet == false ) {
			free( stArray.pData );
			return false;
		}
	}
	if ( stArray.pData != NULL )
		free( stArray.pData );
	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------
// make look up table and send it to module.
_BOOL SetNewLut( LPRefObj pRefSrc )
{
	_BOOL	bRet;
	NkMAIDArray stArray;
	double	lfGamma, dfMaxvalue;
	ULONG	i, ulLUTDimSize, ulPlaneCount;
	char	buf[256];

	printf( "Gamma >");
	scanf( "%s", buf );
	lfGamma = atof( buf );

	bRet = Command_CapGet( pRefSrc->pObject, kNkMAIDCapability_Lut, kNkMAIDDataType_ArrayPtr, (NKPARAM)&stArray, NULL, NULL );
	if( bRet == false ) return false;
	stArray.pData = malloc( stArray.ulElements * stArray.wPhysicalBytes );
	if( stArray.pData == NULL ) return false;

	ulLUTDimSize = stArray.ulDimSize1;
	ulPlaneCount = stArray.ulDimSize2;
	// If the array is one dimension, ulDimSize2 is 0. So the ulPlaneCount should be set 1.
	if ( ulPlaneCount == 0 ) ulPlaneCount = 1;

	dfMaxvalue = (double)( pow( 2, stArray.wLogicalBits ) - 1);

	// Make first plane of LookUp Table
	if(stArray.wPhysicalBytes == 1) {
		for( i = 0; i < ulLUTDimSize; i++)
			((UCHAR*)stArray.pData)[i] = (UCHAR)( pow( ((double)i / ulLUTDimSize), 1.0 / lfGamma ) * dfMaxvalue + 0.5 ); 
	} else if(stArray.wPhysicalBytes == 2) {
		for( i = 0; i < ulLUTDimSize; i++)
			((UWORD*)stArray.pData)[i] = (UWORD)( pow( ((double)i / ulLUTDimSize), 1.0 / lfGamma ) * dfMaxvalue + 0.5 ); 
	} else {
		free(stArray.pData);
		return false;
	}
	// Copy from first plane to second and third... plane.
	for( i = 1; i < ulPlaneCount; i++)
		memcpy( (LPVOID)((ULONG)stArray.pData + i * ulLUTDimSize * stArray.wPhysicalBytes), stArray.pData, ulLUTDimSize * stArray.wPhysicalBytes );

	// check if this capability suports CapSet operation.
	if ( CheckCapabilityOperation( pRefSrc, kNkMAIDCapability_Lut, kNkMAIDCapOperation_Set ) ) {
		// Send look up table
		bRet = Command_CapSet( pRefSrc->pObject, kNkMAIDCapability_Lut, kNkMAIDDataType_ArrayPtr, (NKPARAM)&stArray, NULL, NULL );
		if( bRet == false ) {
			free( stArray.pData );
			return false;
		}
	}
	free(stArray.pData);
	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------
//
_BOOL IssueProcess( LPRefObj pRefSrc, ULONG ulCapID )
{
	LPNkMAIDObject pSourceObject = pRefSrc->pObject;
	LPNkMAIDCapInfo pCapInfo;
	ULONG	ulCount = 0L;
	_BOOL bRet;
	LPRefCompletionProc pRefCompletion;
	pRefCompletion = (LPRefCompletionProc)malloc( sizeof(RefCompletionProc) );
	pRefCompletion->pulCount = &ulCount;
	pRefCompletion->pRef = NULL;

	// Confirm whether this capability is supported or not.
	pCapInfo =	GetCapInfo( pRefSrc, ulCapID );
	// check if the CapInfo is available.
	if ( pCapInfo == NULL ) return false;

	printf( "[%s]\n", pCapInfo->szDescription );

	// Start the process
	bRet = Command_CapStart( pSourceObject, ulCapID, (LPNKFUNC)CompletionProc, (NKREF)pRefCompletion );
	if ( bRet == false ) return false;
	// Wait for end of the process and issue Command_Async.
	IdleLoop( pSourceObject, &ulCount, 1 );

	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------
//
_BOOL IssueProcessSync( LPRefObj pRefSrc, ULONG ulCapID )
{
	LPNkMAIDObject pSourceObject = pRefSrc->pObject;
	LPNkMAIDCapInfo pCapInfo;
	ULONG	ulCount = 0L;
	_BOOL bRet;
	// Confirm whether this capability is supported or not.
	pCapInfo =	GetCapInfo( pRefSrc, ulCapID );
	// check if the CapInfo is available.
	if ( pCapInfo == NULL ) return false;

	printf( "[%s]\n", pCapInfo->szDescription );

	// Start the process
	bRet = Command_CapStart( pSourceObject, ulCapID, NULL, NULL );
	if ( bRet == false ) return false;

	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------
//
_BOOL IssueAcquire( LPRefObj pRefDat )
{
	_BOOL	bRet;
	LPRefObj	pRefItm = (LPRefObj)pRefDat->pRefParent;
	LPRefObj	pRefSrc = (LPRefObj)pRefItm->pRefParent;
	NkMAIDCallback	stProc;
	LPRefDataProc	pRefDeliver;
	LPRefCompletionProc	pRefCompletion;
	ULONG	ulCount = 0L;

	// set reference from DataProc
	pRefDeliver = (LPRefDataProc)malloc( sizeof(RefDataProc) );// this block will be freed in CompletionProc.
	pRefDeliver->pBuffer = NULL;
	pRefDeliver->ulOffset = 0L;
	pRefDeliver->ulTotalLines = 0L;
	pRefDeliver->lID = pRefItm->lMyID;
	// set reference from CompletionProc
	pRefCompletion = (LPRefCompletionProc)malloc( sizeof(RefCompletionProc) );// this block will be freed in CompletionProc.
	pRefCompletion->pulCount = &ulCount;
	pRefCompletion->pRef = pRefDeliver;
	// set reference from DataProc
	stProc.pProc = (LPNKFUNC)DataProc;
	stProc.refProc = (NKREF)pRefDeliver;

	// set DataProc as data delivery callback function
	if( CheckCapabilityOperation( pRefDat, kNkMAIDCapability_DataProc, kNkMAIDCapOperation_Set ) ) {
		bRet = Command_CapSet( pRefDat->pObject, kNkMAIDCapability_DataProc, kNkMAIDDataType_CallbackPtr, (NKPARAM)&stProc, NULL, NULL );
		if ( bRet == false ) return false;
	} else
		return false;

	// start getting an image
	bRet = Command_CapStart( pRefDat->pObject, kNkMAIDCapability_Acquire, (LPNKFUNC)CompletionProc, (NKREF)pRefCompletion );
	if ( bRet == false ) return false;
	IdleLoop( pRefDat->pObject, &ulCount, 1 );

	// reset DataProc
	bRet = Command_CapSet( pRefDat->pObject, kNkMAIDCapability_DataProc, kNkMAIDDataType_Null, (NKPARAM)NULL, NULL, NULL );
	if ( bRet == false ) return false;

	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------
//
_BOOL IssueThumbnail( LPRefObj pRefSrc )
{
	_BOOL	bRet;
	LPRefObj	pRefItm, pRefDat;
	NkMAIDCallback	stProc;
	LPRefDataProc	pRefDeliver;
	LPRefCompletionProc	pRefCompletion;
	ULONG	ulItemID, ulFinishCount = 0L;
	ULONG	i, j;
	NkMAIDEnum	stEnum;
	LPNkMAIDCapInfo	pCapInfo;
	
	pCapInfo = GetCapInfo( pRefSrc, kNkMAIDCapability_Children );
	// check if the CapInfo is available.
	if ( pCapInfo == NULL )	return false;

	// check if this capability suports CapGet operation.
	if ( !CheckCapabilityOperation( pRefSrc, kNkMAIDCapability_Children, kNkMAIDCapOperation_Get ) ) return false;
	bRet = Command_CapGet( pRefSrc->pObject, kNkMAIDCapability_Children, kNkMAIDDataType_EnumPtr, (NKPARAM)&stEnum, NULL, NULL );
	if ( bRet == false ) return false;

	// If the source object has no item, it does nothing and returns soon.
	if ( stEnum.ulElements == 0 )	return true;

	// allocate memory for array data
	stEnum.pData = malloc( stEnum.ulElements * stEnum.wPhysicalBytes );
	if ( stEnum.pData == NULL ) return false;
	// get array data
	bRet = Command_CapGetArray( pRefSrc->pObject, kNkMAIDCapability_Children, kNkMAIDDataType_EnumPtr, (NKPARAM)&stEnum, NULL, NULL );
	if ( bRet == false ) {
		free( stEnum.pData );
		return false;
	}

	// Open all thumbnail objects in the current directory.
	for ( i = 0; i < stEnum.ulElements; i++ ) {
		ulItemID = ((ULONG*)stEnum.pData)[i];
		pRefItm = GetRefChildPtr_ID( pRefSrc, ulItemID );
		if ( pRefItm == NULL ) {
			// open the item object
			bRet = AddChild( pRefSrc, ulItemID );
			if ( bRet == false ) {
				free( stEnum.pData );
				return false;
			}
			pRefItm = GetRefChildPtr_ID( pRefSrc, ulItemID );
		}
		if ( pRefItm != NULL ) {
			pRefDat = GetRefChildPtr_ID( pRefItm, kNkMAIDDataObjType_Thumbnail );
			if ( pRefDat == NULL ) {
				// open the thumbnail object
				bRet = AddChild( pRefItm, kNkMAIDDataObjType_Thumbnail );
				if ( bRet == false ) {
					free( stEnum.pData );
					return false;
				}
				pRefDat = GetRefChildPtr_ID( pRefItm, kNkMAIDDataObjType_Thumbnail );
			}
		}
	}
	free ( stEnum.pData );

	// set NkMAIDCallback structure for DataProc
	stProc.pProc = (LPNKFUNC)DataProc;

	// acquire all thumbnail images.
	for ( i = 0; i < pRefSrc->ulChildCount; i++ ) {
		pRefItm = GetRefChildPtr_Index( pRefSrc, i );
		pRefDat = GetRefChildPtr_ID( pRefItm, kNkMAIDDataObjType_Thumbnail );

		if ( pRefDat != NULL ) {
			// set RefDeliver structure refered in DataProc
			pRefDeliver = (LPRefDataProc)malloc( sizeof(RefDataProc) );// this block will be freed in CompletionProc.
			pRefDeliver->pBuffer = NULL;
			pRefDeliver->ulOffset = 0L;
			pRefDeliver->ulTotalLines = 0L;
			pRefDeliver->lID = pRefItm->lMyID;

			// set DataProc as data delivery callback function
			stProc.refProc = (NKREF)pRefDeliver;
			if( CheckCapabilityOperation( pRefDat, kNkMAIDCapability_DataProc, kNkMAIDCapOperation_Set ) ) {
				bRet = Command_CapSet( pRefDat->pObject, kNkMAIDCapability_DataProc, kNkMAIDDataType_CallbackPtr, (NKPARAM)&stProc, NULL, NULL );
				if ( bRet == false ) return false;
			} else
				return false;

			pRefCompletion = (LPRefCompletionProc)malloc( sizeof(RefCompletionProc) );// this block will be freed in CompletionProc.

			// Set RefCompletion structure refered from CompletionProc.
			pRefCompletion->pulCount = &ulFinishCount;
			pRefCompletion->pRef = pRefDeliver;

			// Starting Acquire Thumbnail
			bRet = Command_CapStart( pRefDat->pObject, kNkMAIDCapability_Acquire, (LPNKFUNC)CompletionProc, (NKREF)pRefCompletion );
			if ( bRet == false ) return false;
		} else {
			// This item doesn't have a thumbnail, so we count up ulFinishCount.
			ulFinishCount++;
		}

		// Send Async command to all DataObjects that have started acquire command.
		for ( j = 0; j <= i; j++ ) {
			bRet = Command_Async( GetRefChildPtr_ID(GetRefChildPtr_Index(pRefSrc, j ), kNkMAIDDataObjType_Thumbnail)->pObject );
			if ( bRet == false ) return false;
		}
	}

	// Send Async command to all DataObjects, untill all scanning complete.
	while ( ulFinishCount < pRefSrc->ulChildCount ) {
		for ( j = 0; j < pRefSrc->ulChildCount; j++ ) {
			bRet = Command_Async( GetRefChildPtr_ID(GetRefChildPtr_Index( pRefSrc, j), kNkMAIDDataObjType_Thumbnail )->pObject );
			if ( bRet == false ) return false;
		}
	}

	// Close all item objects(include image and thumbnail object).
	while ( pRefSrc->ulChildCount > 0 ) {
		pRefItm = GetRefChildPtr_Index( pRefSrc, 0 );
		ulItemID = pRefItm->lMyID;
		// reset DataProc
		bRet = Command_CapSet( pRefDat->pObject, kNkMAIDCapability_DataProc, kNkMAIDDataType_Null, (NKPARAM)NULL, NULL, NULL );
		if ( bRet == false ) return false;
		bRet = RemoveChild( pRefSrc, ulItemID );
		if ( bRet == false ) return false;
	}

	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------
// get pointer to CapInfo, the capability ID of that is 'ulID'
LPNkMAIDCapInfo GetCapInfo(LPRefObj pRef, ULONG ulID)
{
	ULONG i;
	LPNkMAIDCapInfo pCapInfo;

	if (pRef == NULL)
		return NULL;
	for ( i = 0; i < pRef->ulCapCount; i++ ){
		pCapInfo = (LPNkMAIDCapInfo)( (ULONG)pRef->pCapArray + i * sizeof(NkMAIDCapInfo) );
		if ( pCapInfo->ulID == ulID )
			break;
	}
	if ( i < pRef->ulCapCount )
		return pCapInfo;
	else
		return NULL;
}
//------------------------------------------------------------------------------------------------------------------------------------
//
_BOOL CheckCapabilityOperation(LPRefObj pRef, ULONG ulID, ULONG ulOperations)
{
	SLONG nResult;
	LPNkMAIDCapInfo pCapInfo = GetCapInfo(pRef, ulID);

	if(pCapInfo != NULL){
		if(pCapInfo->ulOperations & ulOperations){
			nResult = kNkMAIDResult_NoError;
		}else{
			nResult = kNkMAIDResult_NotSupported;
		}
	}else{
		nResult = kNkMAIDResult_NotSupported;
	}

	return (nResult == kNkMAIDResult_NoError);
}
//------------------------------------------------------------------------------------------------------------------------------------
//
_BOOL AddChild( LPRefObj pRefParent, SLONG lIDChild )
{
	SLONG lResult;
	ULONG ulCount = pRefParent->ulChildCount;
	LPVOID pNewMemblock = realloc( pRefParent->pRefChildArray, (ulCount + 1) * sizeof(LPRefObj) );
	LPRefObj pRefChild = (LPRefObj)malloc( sizeof(RefObj) );

	if(pNewMemblock == NULL || pRefChild == NULL) {
		puts( "There is not enough memory" );
		return false;
	}
	pRefParent->pRefChildArray = pNewMemblock;
	((LPRefObj*)pRefParent->pRefChildArray)[ulCount] = pRefChild;
	InitRefObj(pRefChild);
	pRefChild->lMyID = lIDChild;
	pRefChild->pRefParent = pRefParent;
	pRefChild->pObject = (LPNkMAIDObject)malloc(sizeof(NkMAIDObject));
	if(pRefChild->pObject == NULL){
		puts( "There is not enough memory" );
		pRefParent->pRefChildArray = realloc( pRefParent->pRefChildArray, ulCount * sizeof(LPRefObj) );
		return false;
	}

	pRefChild->pObject->refClient = (NKREF)pRefChild;
	lResult = Command_Open( pRefParent->pObject, pRefChild->pObject, lIDChild );
	if(lResult == TRUE)
		pRefParent->ulChildCount ++;
	else {
		puts( "Failed in Opening an object." );
		pRefParent->pRefChildArray = realloc( pRefParent->pRefChildArray, ulCount * sizeof(LPRefObj) );
		free(pRefChild->pObject);
		free(pRefChild);
		return false;
	}

	lResult = EnumCapabilities( pRefChild->pObject, &(pRefChild->ulCapCount), &(pRefChild->pCapArray), NULL, NULL );
	
	// set callback functions to child object.
	SetProc( pRefChild );

	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------
//
_BOOL RemoveChild( LPRefObj pRefParent, SLONG lIDChild )
{
	LPRefObj pRefChild = NULL, *pOldRefChildArray, *pNewRefChildArray;
	ULONG i, n;
	pRefChild = GetRefChildPtr_ID( pRefParent, lIDChild );
	if ( pRefChild == NULL ) return false;

	while ( pRefChild->ulChildCount > 0 )
		RemoveChild( pRefChild, ((LPRefObj*)pRefChild->pRefChildArray)[0]->lMyID );

	if ( ResetProc( pRefChild ) == false ) return false;
	if ( Command_Close( pRefChild->pObject ) == false ) return false;
	pOldRefChildArray = (LPRefObj*)pRefParent->pRefChildArray;
	pNewRefChildArray = NULL;
	if( pRefParent->ulChildCount > 1 ){
		pNewRefChildArray = (LPRefObj*)malloc( (pRefParent->ulChildCount - 1) * sizeof(LPRefObj) );
		for( n = 0, i = 0; i < pRefParent->ulChildCount; i++ ){
			if( ((LPRefObj)pOldRefChildArray[i])->lMyID != lIDChild )
				memmove( &pNewRefChildArray[n++], &pOldRefChildArray[i], sizeof(LPRefObj) );
		}
	}
	pRefParent->pRefChildArray = pNewRefChildArray;
	pRefParent->ulChildCount--;
	if ( pRefChild->pObject != NULL )
		free( pRefChild->pObject );
	if ( pRefChild->pCapArray != NULL )
		free( pRefChild->pCapArray );
	if ( pRefChild->pRefChildArray != NULL )
		free( pRefChild->pRefChildArray );
	if ( pRefChild != NULL )
		free( pRefChild );
	if ( pOldRefChildArray != NULL )
		free( pOldRefChildArray );
	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------
//
_BOOL SetProc( LPRefObj pRefObj )
{
	_BOOL bRet;
	NkMAIDCallback	stProc;
	stProc.refProc = (NKREF)pRefObj;

	if( CheckCapabilityOperation( pRefObj, kNkMAIDCapability_ProgressProc, kNkMAIDCapOperation_Set ) ){
		stProc.pProc = (LPNKFUNC)ProgressProc;
		bRet = Command_CapSet( pRefObj->pObject, kNkMAIDCapability_ProgressProc, kNkMAIDDataType_CallbackPtr, (NKPARAM)&stProc, NULL, NULL );
		if ( bRet == false ) return false;
	}

	switch ( pRefObj->pObject->ulType  ) {
		case kNkMAIDObjectType_Module:
			// If Module object supports Cap_EventProc, set ModEventProc. 
			if( CheckCapabilityOperation( pRefObj, kNkMAIDCapability_EventProc, kNkMAIDCapOperation_Set ) ) {
				stProc.pProc = (LPNKFUNC)ModEventProc;
				bRet = Command_CapSet( pRefObj->pObject, kNkMAIDCapability_EventProc, kNkMAIDDataType_CallbackPtr, (NKPARAM)&stProc, NULL, NULL );
				if ( bRet == false ) return false;
			}
			// UIRequestProc is supported by Module object only.
			if( CheckCapabilityOperation( pRefObj, kNkMAIDCapability_UIRequestProc, kNkMAIDCapOperation_Set ) ) {
				stProc.pProc = (LPNKFUNC)UIRequestProc;
				bRet = Command_CapSet( pRefObj->pObject, kNkMAIDCapability_UIRequestProc, kNkMAIDDataType_CallbackPtr, (NKPARAM)&stProc, NULL, NULL );
				if ( bRet == false ) return false;
			}
			break;
		case kNkMAIDObjectType_Source:
			// If Source object supports Cap_EventProc, set SrcEventProc. 
			if( CheckCapabilityOperation( pRefObj, kNkMAIDCapability_EventProc, kNkMAIDCapOperation_Set ) ) {
				stProc.pProc = (LPNKFUNC)SrcEventProc;
				bRet = Command_CapSet( pRefObj->pObject, kNkMAIDCapability_EventProc, kNkMAIDDataType_CallbackPtr, (NKPARAM)&stProc, NULL, NULL );
				if ( bRet == false ) return false;
			}
			break;
		case kNkMAIDObjectType_Item:
			// If Item object supports Cap_EventProc, set ItmEventProc. 
			if( CheckCapabilityOperation( pRefObj, kNkMAIDCapability_EventProc, kNkMAIDCapOperation_Set ) ) {
				stProc.pProc = (LPNKFUNC)ItmEventProc;
				bRet = Command_CapSet( pRefObj->pObject, kNkMAIDCapability_EventProc, kNkMAIDDataType_CallbackPtr, (NKPARAM)&stProc, NULL, NULL );
				if ( bRet == false ) return false;
			}
			break;
		case kNkMAIDObjectType_DataObj:
			// if Data object supports Cap_EventProc, set DatEventProc. 
			if( CheckCapabilityOperation( pRefObj, kNkMAIDCapability_EventProc, kNkMAIDCapOperation_Set ) ) {
				stProc.pProc = (LPNKFUNC)DatEventProc;
				bRet = Command_CapSet( pRefObj->pObject, kNkMAIDCapability_EventProc, kNkMAIDDataType_CallbackPtr, (NKPARAM)&stProc, NULL, NULL );
				if ( bRet == false ) return false;
			}
			break;
	}

	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------
//
_BOOL ResetProc( LPRefObj pRefObj )
{
	_BOOL bRet;

	if( CheckCapabilityOperation( pRefObj, kNkMAIDCapability_ProgressProc, kNkMAIDCapOperation_Set ) ) {
		bRet = Command_CapSet( pRefObj->pObject, kNkMAIDCapability_ProgressProc, kNkMAIDDataType_Null, (NKPARAM)NULL, NULL, NULL );
		if ( bRet == false ) return false;
	}
	if( CheckCapabilityOperation( pRefObj, kNkMAIDCapability_EventProc, kNkMAIDCapOperation_Set ) ) {
		bRet = Command_CapSet( pRefObj->pObject, kNkMAIDCapability_EventProc, kNkMAIDDataType_Null, (NKPARAM)NULL, NULL, NULL );
		if ( bRet == false ) return false;
	}

	if ( pRefObj->pObject->ulType == kNkMAIDObjectType_Module ) {
			// UIRequestProc is supported by Module object only.
			if( CheckCapabilityOperation( pRefObj, kNkMAIDCapability_UIRequestProc, kNkMAIDCapOperation_Set ) ) {
				bRet = Command_CapSet( pRefObj->pObject, kNkMAIDCapability_UIRequestProc, kNkMAIDDataType_Null, (NKPARAM)NULL, NULL, NULL );
				if ( bRet == false ) return false;
			}
	}

	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Get pointer to reference of child object by child's ID
LPRefObj GetRefChildPtr_ID( LPRefObj pRefParent, SLONG lIDChild )
{
	LPRefObj pRefChild;
	ULONG ulCount;

	if(pRefParent == NULL)
		return NULL;

	for( ulCount = 0; ulCount < pRefParent->ulChildCount; ulCount++ ){
		if ( (pRefChild = GetRefChildPtr_Index(pRefParent, ulCount)) != NULL ) {
			if (pRefChild->lMyID == lIDChild)
				return pRefChild;
		}
	}

	return NULL;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Get pointer to reference of child object by index
LPRefObj GetRefChildPtr_Index( LPRefObj pRefParent, ULONG ulIndex )
{
	if (pRefParent == NULL)
		return NULL;

	if( (pRefParent->pRefChildArray != NULL) && (ulIndex < pRefParent->ulChildCount) )
		return (LPRefObj)((LPRefObj*)pRefParent->pRefChildArray)[ulIndex];
	else
		return NULL;
}
//------------------------------------------------------------------------------------------------------------------------------------

