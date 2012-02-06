#import "D300Controller.h"

LPMAIDEntryPointProc	g_pMAIDEntryPoint = NULL;
UCHAR	g_bFileRemoved = false;
ULONG	g_ulCameraType = 0;	// CameraType

CFragConnectionID	g_ConnID = 0;
short	g_nModRefNum = -1;

@implementation D300Controller

- (_BOOL)doInitMaid3 // from main2()
{
	_BOOL	bRet;
	
	// Search for a Module-file like "Type0001.md3".
#ifdef _WINDOWS
	bRet = Search_Module( ModulePath );
#else
	bRet = Search_Module( &ModulePath );
#endif
	if ( bRet == false ) {
		puts( "\"Type0001 Module\" is not found.\n" );
		return -1;
	}
	
	// Load the Module-file.
#ifdef _WINDOWS
	bRet = Load_Module( ModulePath );
#else
	//const char *modulePath = [[[NSBundle mainBundle] pathForResource:@"Type0001 Module" ofType:@"bundle"] cString];	
	bRet = Load_Module( &ModulePath );
#endif
	if ( bRet == false ) {
		puts( "Failed in loading \"Type0001 Module\".\n" );
		return -1;
	}
	
	// Allocate memory for reference to Module object.
	pRefMod = (LPRefObj)malloc(sizeof(RefObj));
	if ( pRefMod == NULL ) {
		puts( "There is not enough memory." );
		return -1;
	}
	InitRefObj( pRefMod );
	
	// Allocate memory for Module object.
	pRefMod->pObject = (LPNkMAIDObject)malloc(sizeof(NkMAIDObject));
	if ( pRefMod->pObject == NULL ) {
		puts( "There is not enough memory." );
		if ( pRefMod != NULL )	free( pRefMod );
		return -1;
	}
	
	//	Open Module object
	pRefMod->pObject->refClient = (NKREF)pRefMod;
	bRet = Command_Open(	NULL,					// When Module_Object will be opend, "pParentObj" is "NULL".
						pRefMod->pObject,	// Pointer to Module_Object 
						ulModID );			// Module object ID set by Client
	if ( bRet == false ) {
		puts( "Module object can't be opened.\n" );
		if ( pRefMod->pObject != NULL )	free( pRefMod->pObject );
		if ( pRefMod != NULL )	free( pRefMod );
		return -1;
	}
	
	//	Enumerate Capabilities that the Module has.
	bRet = EnumCapabilities( pRefMod->pObject, &(pRefMod->ulCapCount), &(pRefMod->pCapArray), NULL, NULL );
	if ( bRet == false ) {
		puts( "Failed in enumeration of capabilities." );
		if ( pRefMod->pObject != NULL )	free( pRefMod->pObject );
		if ( pRefMod != NULL )	free( pRefMod );
		return -1;
	}
	
	//	Set the callback functions(ProgressProc, EventProc and UIRequestProc).
	bRet = SetProc( pRefMod );
	if ( bRet == false ) {
		puts( "Failed in setting a call back function." );
		if ( pRefMod->pObject != NULL )	free( pRefMod->pObject );
		if ( pRefMod != NULL )	free( pRefMod );
		return -1;
	}
	
	//	Set the kNkMAIDCapability_ModuleMode.
	if( CheckCapabilityOperation( pRefMod, kNkMAIDCapability_ModuleMode, kNkMAIDCapOperation_Set )  ){
		bRet = Command_CapSet( pRefMod->pObject, kNkMAIDCapability_ModuleMode, kNkMAIDDataType_Unsigned, 
							  (NKPARAM)kNkMAIDModuleMode_Controller, NULL, NULL);
		if ( bRet == false ) {
			puts( "Failed in setting kNkMAIDCapability_ModuleMode." );
			return -1;
		}
	}	
	return 0;
}

- (void) doUninitMaid3 // from main2()
{
	_BOOL	bRet;

	// Close Module_Object
	bRet = Close_Module( pRefMod );
	if ( bRet == false )
		puts( "Module object can not be closed.\n" );
	
	CloseConnection( &g_ConnID );
	g_ConnID = 0;
	g_nModRefNum = -1;
	
	// Free memory blocks allocated in this function.
	if ( pRefMod->pObject != NULL )	free( pRefMod->pObject );
	if ( pRefMod != NULL ) free( pRefMod );
	
	puts( "This sample program has terminated.\n" );
}

- (_BOOL) enumSources // from SelectSource()
{
	LPRefObj pRefObj = pRefMod;
	
	_BOOL	bRet;
	NkMAIDEnum	stEnum;

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
	
	for (int i = 0; i < stEnum.ulElements; i++ )
	{

		/*ULONG lSrcID = ((ULONG*)stEnum.pData)[i];
		pRefSrc = GetRefChildPtr_ID( pRefObj, lSrcID );
		if ( pRefSrc == NULL ) {
			// Create Source object and RefSrc structure.
			if ( AddChild( pRefMod, ulSrcID ) == true ) {
				printf("Source object is opened.\n");
			} else {
				printf("Source object can't be opened.\n");
				break;
			}
			pRefSrc = GetRefChildPtr_ID( pRefMod, ulSrcID );
		}
		
		// Get CameraType
		Command_CapGet( pRefSrc->pObject, kNkMAIDCapability_CameraType, kNkMAIDDataType_UnsignedPtr, (NKPARAM)&g_ulCameraType, NULL, NULL );
		
		bRet = SetUnsignedCapability( pRefSrc, kNkMAIDCapability_CameraType );
		bRet = RemoveChild( pRefMod, lSrcID );*/
		
		printf( "%d. ID = %d\n", i + 1, ((ULONG*)stEnum.pData)[i] );
		NSMenuItem *m = [devicesMenu addItemWithTitle:[NSString stringWithFormat:@"Device %d", ((ULONG*)stEnum.pData)[i]] action:@selector(deviceSelected:) keyEquivalent:@""];
		[m setTarget:self];
		[m setTag: ((ULONG*)stEnum.pData)[i]];
		if (stEnum.ulElements == 1) [self deviceSelected:m];
	}
	free( stEnum.pData );

	return true;
}

- (void)awakeFromNib
{
	[self doInitMaid3];
	[self enumSources];
}

- (void) dealloc
{
	[self doUninitMaid3];
	[super dealloc];
}

- (void)deviceSelected:(id) sender
{
	[[[CameraWindowController alloc] initWithModule:pRefMod deviceId:[sender tag]] showWindow:self];
}

@end
