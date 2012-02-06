#import <Cocoa/Cocoa.h>
#import "maid3.h"
#import "maid3d1.h"
#import "CtrlSample.h"

#import "Maid3Helpers.h"
#import "CameraWindowController.h"

@interface D300Controller : NSObject 
{
	// Maid3
	FSSpec ModulePath;
	LPRefObj	pRefMod, pRefSrc, RefItm, pRefDat;
	ULONG	ulModID, ulSrcID;
	UWORD	wSel;
	
	IBOutlet NSMenu *devicesMenu;
}

@end
