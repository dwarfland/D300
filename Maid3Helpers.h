/*
 *  Maid3Helpers.h
 *  D300
 *
 *  Created by marc hoffman on 12/19/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#import <Cocoa/Cocoa.h>
#import "maid3.h"
#import "maid3d1.h"
#import "CtrlSample.h"

NSString *GetCapabilityString( LPRefObj pRefObj, ULONG ulCapID, ULONG ulCurrentValue);

_BOOL GetEnumCapability( LPRefObj pRefObj, ULONG ulCapID, NkMAIDEnum *stEnum, NSString **name );
_BOOL GetUnsignedCapability( LPRefObj pRefObj, ULONG ulCapID, ULONG *ulCurrentValue);
_BOOL GetFloatCapability( LPRefObj pRefObj, ULONG ulCapID, double* lfValue );
_BOOL SetUnsignedCapabilityToValue( LPRefObj pRefObj, ULONG ulCapID, ULONG ulNewValue);
_BOOL GetLiveViewImage( LPRefObj pRefSrc, NSImage **image );
