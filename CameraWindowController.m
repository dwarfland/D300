#import "CameraWindowController.h"

@interface CameraWindowController (Private)
- (void)doInit;
@end


@implementation CameraWindowController

- (id)initWithModule:(LPRefObj) aModule deviceId:(SLONG)aDeviceId
{
	self = [super initWithWindowNibName:@"CameraWindowController"];
	if (self)
	{
		pRefMod = aModule;
		deviceId = aDeviceId;
		[[self window] setTitle:[NSString stringWithFormat:@"Device %d", aDeviceId]];
		[self doInit];
	}
	return self;
}

- (void)dealloc
{
	[liveViewTimer invalidate];
	[liveViewTimer release];
	[settingsTimer invalidate];
	[settingsTimer release];
	if (pRefSrc) RemoveChild( pRefMod, deviceId );
	[super dealloc];
}

- (void)doInit
{
	pRefSrc = GetRefChildPtr_ID( pRefMod, deviceId );
	if ( pRefSrc == NULL ) {
		// Create Source object and RefSrc structure.
		if ( AddChild( pRefMod, deviceId ) == true ) 
		{
			printf("Source object is opened.\n");
		} 
		else 
		{
			printf("Source object can't be opened.\n");
		}
		pRefSrc = GetRefChildPtr_ID( pRefMod, deviceId );
	}
	
	ULONG value;
	
	NSString *name = nil;
	GetUnsignedCapability( pRefSrc, kNkMAIDCapability_CameraType, &value );
	switch (value)
	{
		case 32: name = @"Nikon D3"; break;
		case 33: name = @"Nikon D300"; break;
		default: name = @"Unknown Camera Model"; break;
	}
	[[self window] setTitle:name];
	settingsTimer = [[NSTimer scheduledTimerWithTimeInterval:0.5f target:self selector:@selector(onUpdateSettings:) userInfo:nil repeats:YES] retain];

	[self onUpdateSettings:nil];

	GetUnsignedCapability( pRefSrc, kNkMAIDCapability_LiveViewStatus, &value);
	if (value == 1)
	{
		[self liveViewEnabled];
	}
	
}

- (IBAction) liveViewEnabled
{
	[self onUpdateLiveView:nil];
	[liveViewToolbarItem setLabel:@"Disable Live"];
	liveViewTimer = [[NSTimer scheduledTimerWithTimeInterval:0.5f target:self selector:@selector(onUpdateLiveView:) userInfo:nil repeats:YES] retain];
}

- (IBAction) onUpdateLiveView:(id)sender
{
	NSImage *i = NULL;
	GetLiveViewImage( pRefSrc, &i );
	if (i) [liveImage setImage:i];
}

- (IBAction) onUpdateSettings:(id)sender
{
	_BOOL bRet = true;
	
	NSString *mode, *speed, *aperture;
	NkMAIDEnum enumValue;
	//ULONG ulValue;
	double fValue;
	
	// ExposureMode
	bRet = GetEnumCapability( pRefSrc, kNkMAIDCapability_ExposureMode, &enumValue, &mode );
	
	// ShutterSpeed(Exposure Time)
	bRet = GetEnumCapability( pRefSrc, kNkMAIDCapability_ShutterSpeed, &enumValue, &speed );
	// Aperture(F Number)
	bRet = GetEnumCapability( pRefSrc, kNkMAIDCapability_Aperture, &enumValue, &aperture );
	// FocusMode
	//bRet = GetUnsignedCapability( pRefSrc, kNkMAIDCapability_FocusMode, &ulValue );
	// FocalLength
	bRet = GetFloatCapability( pRefSrc, kNkMAIDCapability_FocalLength, &fValue );
	
	[cameraSettings setStringValue:[NSString stringWithFormat:@"%@ mode   Shutter: %@   Aperture: f/%@   Focal Length: %0.fmm", mode, speed, aperture, fValue]];

}

- (IBAction) onToggleLiveView:(id)sender
{
	ULONG value;
	GetUnsignedCapability( pRefSrc, kNkMAIDCapability_LiveViewStatus, &value);
	if (value == 1)
	{
		[liveViewTimer invalidate];
		[liveViewTimer release];
		liveViewTimer = nil;
		SetUnsignedCapabilityToValue( pRefSrc, kNkMAIDCapability_LiveViewStatus, 0);
		[liveViewToolbarItem setLabel:@"Enable Live"];
		[liveImage setImage:nil];
	}
	else
	{
		SetUnsignedCapabilityToValue( pRefSrc, kNkMAIDCapability_LiveViewStatus, 1);
		[self liveViewEnabled];
	}
}

- (IBAction) onAutoFocus:(id)sender
{
	_BOOL	bRet = true;
	bRet = IssueProcess( pRefSrc, kNkMAIDCapability_AutoFocus );
}

- (IBAction) onCapture:(id)sender
{
	_BOOL	bRet = true;
	bRet = IssueProcess( pRefSrc, kNkMAIDCapability_Capture );
}

- (IBAction) onPreCapture:(id)sender
{
	_BOOL	bRet = true;
	bRet = IssueProcess( pRefSrc, kNkMAIDCapability_PreCapture );
}


@end
