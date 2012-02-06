#import <Cocoa/Cocoa.h>
#import "maid3.h"
#import "maid3d1.h"
#import "CtrlSample.h"
#import "Maid3Helpers.h"

@interface CameraWindowController : NSWindowController 
{
	SLONG deviceId;
	LPRefObj pRefMod, pRefSrc;
	
	NSTimer *liveViewTimer;
	NSTimer *settingsTimer;
	
	IBOutlet NSToolbarItem *liveViewToolbarItem;
	IBOutlet NSImageView *liveImage;
	IBOutlet NSTextField *cameraSettings;
}

- initWithModule:(LPRefObj) aModule deviceId:(SLONG)aDeviceId;

- (IBAction) onUpdateLiveView:(id)sender;
- (IBAction) onToggleLiveView:(id)sender;

- (IBAction) onAutoFocus:(id)sender;
- (IBAction) onCapture:(id)sender;
- (IBAction) onPreCapture:(id)sender;

@end
