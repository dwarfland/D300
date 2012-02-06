/*
 *  Maid3Helpers.c
 *  D300
 *
 *  Created by marc hoffman on 12/19/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "Maid3Helpers.h"

_BOOL GetEnumUnsignedCapability( LPRefObj pRefObj, ULONG ulCapID, LPNkMAIDEnum pstEnum, NSString **name )
{
	_BOOL	bRet;
	char	psString[32];
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
	
	char *psStr = GetEnumString( ulCapID, ((ULONG*)pstEnum->pData)[pstEnum->ulValue], psString );
	printf("value name: %s\n", psStr);
	*name = [NSString stringWithCString:psStr];
	
	free( pstEnum->pData );
	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Show the current setting of a Enum(Packed String) type capability and set a value for it.
_BOOL GetEnumPackedStringCapability( LPRefObj pRefObj, ULONG ulCapID, LPNkMAIDEnum pstEnum, NSString **name )
{
	_BOOL	bRet;
	char	*psStr;
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
	for ( i = 0; i < pstEnum->ulElements; ) 
	{
		psStr = (char*)((ULONG)pstEnum->pData + i);
		if (ulCount == pstEnum->ulValue)
			*name = [NSString stringWithCString:psStr];
		printf( "%2d. %s\n", ++ulCount, psStr );
		i += strlen( psStr ) + 1;
	}
	printf( "Current Setting: %d\n", pstEnum->ulValue + 1 );
	
	//psStr = (char*)((ULONG)pstEnum->pData + (pstEnum->ulValue));
	//printf("value name: %s\n", psStr);
	
	
	free( pstEnum->pData );
	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Show the current setting of a Enum(String Integer) type capability and set a value for it.
_BOOL GetEnumStringCapability( LPRefObj pRefObj, ULONG ulCapID, LPNkMAIDEnum pstEnum, NSString **name )
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

_BOOL GetEnumCapability( LPRefObj pRefObj, ULONG ulCapID, NkMAIDEnum *stEnum, NSString **name )
{
	_BOOL	bRet;
	if (name) *name = nil;
	
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, ulCapID );
	if ( pCapInfo == NULL ) return false;
	
	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_Enum ) return false;
	// check if this capability suports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Get ) ) return false;
	
	bRet = Command_CapGet( pRefObj->pObject, ulCapID, kNkMAIDDataType_EnumPtr, (NKPARAM)stEnum, NULL, NULL );
	if( bRet == false ) return false;
	
	switch ( stEnum->ulType ) {
		case kNkMAIDArrayType_Unsigned:
			return GetEnumUnsignedCapability( pRefObj, ulCapID, stEnum, name );
			break;
		case kNkMAIDArrayType_PackedString:
			return GetEnumPackedStringCapability( pRefObj, ulCapID, stEnum, name );
			break;
		case kNkMAIDArrayType_String:
			return GetEnumStringCapability( pRefObj, ulCapID, stEnum, name );
			break;
		default:
			return false;
	}
}

_BOOL GetUnsignedCapability( LPRefObj pRefObj, ULONG ulCapID, ULONG *ulCurrentValue)
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
	*ulCurrentValue = ulValue;
	
	return true;
}

_BOOL GetFloatCapability( LPRefObj pRefObj, ULONG ulCapID, double* lfValue )
{
	_BOOL	bRet;
	char	buf[256];
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, ulCapID );
	if ( pCapInfo == NULL ) return false;
	
	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_Float ) return false;
	// check if this capability suports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Get ) ) return false;
	
	bRet = Command_CapGet( pRefObj->pObject, ulCapID, kNkMAIDDataType_FloatPtr, (NKPARAM)lfValue, NULL, NULL );
	if( bRet == false ) return false;
	// show current value of this capability
	printf( "[%s]\n", pCapInfo->szDescription );
	printf( "Current Value: %f\n", *lfValue );
	
	return true;
}

NSString *GetCapabilityString( LPRefObj pRefObj, ULONG ulCapID, ULONG ulCurrentValue)
{
	char	buf[256];
	return [NSString stringWithCString:GetUnsignedString( ulCapID, ulCurrentValue, buf )];
}

_BOOL SetUnsignedCapabilityToValue( LPRefObj pRefObj, ULONG ulCapID, ULONG ulNewValue)
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
		//printf( "Input new value\n>" );
		//scanf( "%s", buf );
		//ulValue = atol( buf );
		bRet = Command_CapSet( pRefObj->pObject, ulCapID, kNkMAIDDataType_Unsigned, (NKPARAM)ulNewValue, NULL, NULL );
		if( bRet == false ) return false;
	} else {
		// This capablity is read-only.
		printf( "This value cannot be changed. Enter '0' to exit.\n>" );
		//scanf( "%s", buf );
	}
	return true;
}

_BOOL GetLiveViewImage( LPRefObj pRefSrc, NSImage **image )
{
	ULONG	ulHeaderSize = 64;		//The header size of LiveView
	NkMAIDArray	stArray;
	unsigned char* pucData = NULL;	// LiveView data pointer
	_BOOL	bRet = true;
	
	
	memset( &stArray, 0, sizeof(NkMAIDArray) );		
	
	bRet = GetArrayCapability( pRefSrc, kNkMAIDCapability_GetLiveViewImage, &stArray );
	if ( bRet == false ) return false;
	
	pucData = (unsigned char*)stArray.pData;
	NSData *d = [NSData dataWithBytes:pucData+ulHeaderSize length:(stArray.ulElements-ulHeaderSize)];
	*image = [[[NSImage alloc] initWithData:d] autorelease];
	
	return true;
}
