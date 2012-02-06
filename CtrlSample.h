//================================================================================================
// Copyright Nikon Corporation - All rights reserved
//
// View this file in a non-proportional font, tabs = 3
//================================================================================================

#ifdef _WINDOWS
#else
//	#include	<CodeFragments.h>
	#include <CoreServices/CoreServices.h>
#endif
/////////////////////////////////////////////////////////////////////////////
// Structures

#pragma pack(push, 2)

	typedef struct tagRefObj
	{
		LPNkMAIDObject	pObject;
		SLONG lMyID;
		LPVOID pRefParent;
		ULONG ulChildCount;
		LPVOID pRefChildArray;
		ULONG ulCapCount;
		LPNkMAIDCapInfo pCapArray;
	} RefObj, *LPRefObj;

	typedef struct tagRefCompletionProc
	{
//		_BOOL bEnd;
		ULONG* pulCount;
		NKERROR nResult;
//		LPVOID pcProgressDlg;
		LPVOID pRef;
	} RefCompletionProc, *LPRefCompletionProc;

	typedef struct tagRefDataProc
	{
		LPVOID	pBuffer;
		ULONG	ulOffset;
		ULONG	ulTotalLines;
		SLONG	lID;
	} RefDataProc, *LPRefDataProc;

	typedef struct tagPSDFileHeader
	{
		char	type[5];
		char	space11[1];
		char	space01[6];
		short	Planecount; 	//0004 if RGB, this is 0003
		long	rowPixels;
		long	columnPixels;
		short	bits; 			//0008 means 8bit. 16bit also supported
		short	mode; 			//0004 means CMYK, Gray -- 1, RGB -- 3
		char	space02[14];
	} PSDFileHeader, *LPPSDFileHeader;

	typedef struct tagRefSpecialCap
	{
		ULONG ulCapID;
		ULONG ulCapValue;
//		ULONG ulCapType;
		ULONG ulUIID;
	} RefSpecialCap, *LPRefSpecialCap;

#pragma pack(pop)


/////////////////////////////////////////////////////////////////////////////
// Prototype

SLONG	CallMAIDEntryPoint( 
		LPNkMAIDObject	pObject,				// module, source, item, or data object
		ULONG				ulCommand,			// Command, one of eNkMAIDCommand
		ULONG				ulParam,				// parameter for the command
		ULONG				ulDataType,			// Data type, one of eNkMAIDDataType
		NKPARAM			data,					// Pointer or long integer
		LPNKFUNC			pfnComplete,		// Completion function, may be NULL
		NKREF				refComplete );		// Value passed to pfnComplete
_BOOL	Command_Async( LPNkMAIDObject pObject);
_BOOL	Command_CapSet( LPNkMAIDObject pObject, ULONG ulParam, ULONG ulDataType, NKPARAM pData, LPNKFUNC pfnComplete, NKREF refComplete );
_BOOL	Command_CapGet( LPNkMAIDObject pObject, ULONG ulParam, ULONG ulDataType, NKPARAM pData, LPNKFUNC pfnComplete, NKREF refComplete );
_BOOL	Command_CapStart( LPNkMAIDObject pObject, ULONG ulParam, LPNKFUNC pfnComplete, NKREF refComplete );
_BOOL	Command_CapGetArray( LPNkMAIDObject pObject, ULONG ulParam, ULONG ulDataType, NKPARAM pData, LPNKFUNC pfnComplete, NKREF refComplete );
_BOOL	Command_CapGetDefault( LPNkMAIDObject pObject, ULONG ulParam, ULONG ulDataType, NKPARAM pData, LPNKFUNC pfnComplete, NKREF refComplete );
_BOOL	Command_Abort(LPNkMAIDObject pobject, LPNKFUNC pfnComplete, NKREF refComplete);
_BOOL	Command_Open( LPNkMAIDObject pParentObj, NkMAIDObject* pChildObj, ULONG ulChildID );
_BOOL	Command_Close( LPNkMAIDObject pObject );

void	CALLPASCAL CALLBACK ModEventProc( NKREF refProc, ULONG ulEvent, NKPARAM data );
void	CALLPASCAL CALLBACK SrcEventProc( NKREF refProc, ULONG ulEvent, NKPARAM data );
void	CALLPASCAL CALLBACK ItmEventProc( NKREF refProc, ULONG ulEvent, NKPARAM data );
void	CALLPASCAL CALLBACK DatEventProc( NKREF refProc, ULONG ulEvent, NKPARAM data );
void	CALLPASCAL CALLBACK ProgressProc( ULONG ulCommand, ULONG ulParam, NKREF refProc, ULONG ulDone, ULONG ulTotal );
ULONG	CALLPASCAL CALLBACK UIRequestProc( NKREF ref, LPNkMAIDUIRequestInfo pUIRequest );
void	CALLPASCAL CALLBACK CompletionProc( LPNkMAIDObject pObject, ULONG ulCommand, ULONG ulParam, ULONG ulDataType, NKPARAM data, NKREF refComplete, NKERROR nResult );
NKERROR	CALLPASCAL CALLBACK DataProc( NKREF ref, LPVOID pDataInfo, LPVOID pData );

void	InitRefObj( LPRefObj pRef );
_BOOL	Search_Module( void* Path );
_BOOL	Load_Module( const void* Path );
_BOOL	Close_Module( LPRefObj pRefMod );
_BOOL	EnumCapabilities( LPNkMAIDObject pobject, ULONG* pulCapCount, LPNkMAIDCapInfo* ppCapArray, LPNKFUNC pfnComplete, NKREF refComplete );
_BOOL	EnumChildrten( LPNkMAIDObject pobject );
_BOOL	AddChild( LPRefObj pRefParent, SLONG lIDChild );
_BOOL	RemoveChild( LPRefObj pRefParent, SLONG lIDChild );
_BOOL	SetProc( LPRefObj pRefObj );
_BOOL	ResetProc( LPRefObj pRefObj );
_BOOL	IdleLoop( LPNkMAIDObject pObject, ULONG* pulCount, ULONG ulEndCount );
void WaitEvent(void);

_BOOL	SourceCommandLoop( LPRefObj pRefMod, ULONG ulSrcID );
_BOOL	ItemCommandLoop( LPRefObj pRefSrc, ULONG ulItemID );
_BOOL	ImageCommandLoop( LPRefObj pRefItm, ULONG ulDatID );
_BOOL	ThumbnailCommandLoop( LPRefObj pRefItm, ULONG ulDatID );
_BOOL	SelectSource( LPRefObj pRefMod, ULONG *pulSrcID );
_BOOL	SelectItem( LPRefObj pRefSrc, ULONG *pulItemID );
_BOOL	SelectData( LPRefObj pRefItm, ULONG *pulDataType );
_BOOL	SetUpCamera1( LPRefObj pRefSrc );
_BOOL	SetUpCamera2( LPRefObj pRefSrc );
_BOOL	SetShootingMenu( LPRefObj pRefSrc );
_BOOL	SetCustomSettings( LPRefObj pRefSrc );
_BOOL	SetEnumCapability( LPRefObj pRefObj, ULONG ulCapID );
_BOOL	SetEnumUnsignedCapability( LPRefObj pRefObj, ULONG ulCapID, LPNkMAIDEnum pstEnum );
_BOOL	SetEnumPackedStringCapability( LPRefObj pRefObj, ULONG ulCapID, LPNkMAIDEnum pstEnum );
_BOOL	SetEnumStringCapability( LPRefObj pRefObj, ULONG ulCapID, LPNkMAIDEnum pstEnum );
_BOOL	SetFloatCapability( LPRefObj pRefObj, ULONG ulCapID );
_BOOL	SetBoolCapability( LPRefObj pRefObj, ULONG ulCapID );
_BOOL	SetIntegerCapability( LPRefObj pRefObj, ULONG ulCapID );
_BOOL	SetUnsignedCapability( LPRefObj pRefObj, ULONG ulCapID );
_BOOL	SetStringCapability( LPRefObj pRefObj, ULONG ulCapID );
_BOOL	SetSizeCapability( LPRefObj pRefObj, ULONG ulCapID );
_BOOL	SetDateTimeCapability( LPRefObj pRefObj, ULONG ulCapID );
_BOOL	SetRangeCapability( LPRefObj pRefObj, ULONG ulCapID );
_BOOL	SetWBPresetDataCapability( LPRefObj pRefSrc );
_BOOL	GetPreviewImageCapability( LPRefObj pRefSrc, ULONG ulItmID );
_BOOL	DeleteDramCapability( LPRefObj pRefItem, ULONG ulItmID );
_BOOL	GetLiveViewImageCapability( LPRefObj pRefSrc );
_BOOL	PictureControlDataCapability( LPRefObj pRefSrc );
_BOOL	SetPictureControlDataCapability( LPRefObj pRefObj, NkMAIDPicCtrlData* pPicCtrlData, char* filename );
_BOOL	GetPictureControlDataCapability( LPRefObj pRefObj, NkMAIDPicCtrlData* pPicCtrlData );
_BOOL	GetPictureControlInfoCapability( LPRefObj pRefSrc );
_BOOL	DeleteCustomPictureControlCapability( LPRefObj pRefSrc );
_BOOL	ShowArrayCapability( LPRefObj pRefObj, ULONG ulCapID );
_BOOL	GetArrayCapability( LPRefObj pRefObj, ULONG ulCapID, LPNkMAIDArray pstArray );
_BOOL	LoadArrayCapability( LPRefObj pRefObj, ULONG ulCapID, char* filename );
_BOOL	SetNewLut( LPRefObj pRefSrc );
char*	GetEnumString( ULONG ulCapID, ULONG ulValue, char *psString );
char*	GetUnsignedString( ULONG ulCapID, ULONG ulValue, char *psString );
_BOOL	IssueProcess( LPRefObj pRefSrc, ULONG ulCapID );
_BOOL	IssueProcessSync( LPRefObj pRefSrc, ULONG ulCapID );
_BOOL	IssueAcquire( LPRefObj pRefDat );
_BOOL	IssueThumbnail( LPRefObj pRefSrc );

LPNkMAIDCapInfo	GetCapInfo( LPRefObj pRef, ULONG ulID );
_BOOL	CheckCapabilityOperation( LPRefObj pRef, ULONG ulID, ULONG ulOperations );
LPRefObj	GetRefChildPtr_Index( LPRefObj pRefParent, ULONG ulIndex );
LPRefObj	GetRefChildPtr_ID( LPRefObj pRefParent, SLONG lIDChild );


/////////////////////////////////////////////////////////////////////////////
// Static variables

extern LPMAIDEntryPointProc g_pMAIDEntryPoint;
extern UCHAR	g_bFileRemoved;
extern _BOOL		g_bFirstCall;	// used in ProgressProc, and DoDeleteDramImage
#ifdef _WINDOWS
	extern HINSTANCE	g_hInstModule;
#else
	extern CFragConnectionID	g_ConnID;
	extern short	g_nModRefNum;
#endif
