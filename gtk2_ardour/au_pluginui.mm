#include <pbd/error.h>
#include <ardour/audio_unit.h>
#include <ardour/insert.h>

#include <gdk/gdkquartz.h>

#include "au_pluginui.h"
#include "gui_thread.h"

#include <appleutility/CAAudioUnit.h>
#include <appleutility/CAComponent.h>

#import <AudioUnit/AUCocoaUIView.h>
#import <CoreAudioKit/AUGenericView.h>

#include "i18n.h"

using namespace ARDOUR;
using namespace Gtk;
using namespace sigc;
using namespace std;
using namespace PBD;

static const float kOffsetForAUView_X = 220;
static const float kOffsetForAUView_Y = 90;

AUPluginUI::AUPluginUI (boost::shared_ptr<PluginInsert> insert)
	: PlugUIBase (insert)
{
	if ((au = boost::dynamic_pointer_cast<AUPlugin> (insert->plugin())) == 0) {
		error << _("unknown type of editor-supplying plugin (note: no AudioUnit support in this version of ardour)") << endmsg;
		throw failed_constructor ();
	}

	bool has_carbon;
	bool has_cocoa;

	carbon_parented = false;
	cocoa_parented = false;
	cocoa_parent = 0;
	cocoa_window = 0;

	test_view_support (has_carbon, has_cocoa);

	if (has_cocoa) {
		create_cocoa_view ();
	} else if (has_carbon) {
		create_carbon_view (has_carbon);
	} else {
		/* fallback to cocoa */
		create_cocoa_view ();
	}
}


AUPluginUI::~AUPluginUI ()
{
	if (carbon_parented) {
		NSWindow* win = get_nswindow();
		RemoveEventHandler(carbon_event_handler);
		[win removeChildWindow:cocoa_parent];
	}
}

void
AUPluginUI::test_view_support (bool& has_carbon, bool& has_cocoa)
{
	has_carbon = test_carbon_view_support();
	has_cocoa = test_cocoa_view_support();
}

bool
AUPluginUI::test_carbon_view_support ()
{
	bool ret = false;
	
	carbon_descriptor.componentType = kAudioUnitCarbonViewComponentType;
	carbon_descriptor.componentSubType = 'gnrc';
	carbon_descriptor.componentManufacturer = 'appl';
	carbon_descriptor.componentFlags = 0;
	carbon_descriptor.componentFlagsMask = 0;
	
	OSStatus err;

	// ask the AU for its first editor component
	UInt32 propertySize;
	err = AudioUnitGetPropertyInfo(*au->get_au(), kAudioUnitProperty_GetUIComponentList, kAudioUnitScope_Global, 0, &propertySize, NULL);
	if (!err) {
		int nEditors = propertySize / sizeof(ComponentDescription);
		ComponentDescription *editors = new ComponentDescription[nEditors];
		err = AudioUnitGetProperty(*au->get_au(), kAudioUnitProperty_GetUIComponentList, kAudioUnitScope_Global, 0, editors, &propertySize);
		if (!err) {
			// just pick the first one for now
			carbon_descriptor = editors[0];
			ret = true;
		}
		delete[] editors;
	}

	return ret;
}
	
bool
AUPluginUI::test_cocoa_view_support ()
{
	UInt32 dataSize   = 0;
	Boolean isWritable = 0;
	OSStatus err = AudioUnitGetPropertyInfo(*au->get_au(),
						kAudioUnitProperty_CocoaUI, kAudioUnitScope_Global,
						0, &dataSize, &isWritable);
	
	return dataSize > 0 && err == noErr;
}

bool
AUPluginUI::plugin_class_valid (Class pluginClass)
{
	if([pluginClass conformsToProtocol: @protocol(AUCocoaUIBase)]) {
		if([pluginClass instancesRespondToSelector: @selector(interfaceVersion)] &&
		   [pluginClass instancesRespondToSelector: @selector(uiViewForAudioUnit:withSize:)]) {
				return true;
		}
	}
	return false;
}

int
AUPluginUI::create_cocoa_view ()
{
	BOOL wasAbleToLoadCustomView = NO;
	AudioUnitCocoaViewInfo* cocoaViewInfo = NULL;
	UInt32               numberOfClasses = 0;
	UInt32     dataSize;
	Boolean    isWritable;
	NSString*	    factoryClassName = 0;
	NSURL*	            CocoaViewBundlePath;

	OSStatus result = AudioUnitGetPropertyInfo (*au->get_au(),
						    kAudioUnitProperty_CocoaUI,
						    kAudioUnitScope_Global, 
						    0,
						    &dataSize,
						    &isWritable );

	numberOfClasses = (dataSize - sizeof(CFURLRef)) / sizeof(CFStringRef);
	
	// Does view have custom Cocoa UI?
	
	if ((result == noErr) && (numberOfClasses > 0) ) {
		cocoaViewInfo = (AudioUnitCocoaViewInfo *)malloc(dataSize);
		if(AudioUnitGetProperty(*au->get_au(),
					kAudioUnitProperty_CocoaUI,
					kAudioUnitScope_Global,
					0,
					cocoaViewInfo,
					&dataSize) == noErr) {

			CocoaViewBundlePath	= (NSURL *)cocoaViewInfo->mCocoaAUViewBundleLocation;
			
			// we only take the first view in this example.
			factoryClassName	= (NSString *)cocoaViewInfo->mCocoaAUViewClass[0];

		} else {

			if (cocoaViewInfo != NULL) {
				free (cocoaViewInfo);
				cocoaViewInfo = NULL;
			}
		}
	}

	NSRect crect = { { 0, 0 }, { 1, 1} };

	// [A] Show custom UI if view has it

	if (CocoaViewBundlePath && factoryClassName) {
		NSBundle *viewBundle  	= [NSBundle bundleWithPath:[CocoaViewBundlePath path]];
		if (viewBundle == nil) {
			error << _("AUPluginUI: error loading AU view's bundle") << endmsg;
			return -1;
		} else {
			Class factoryClass = [viewBundle classNamed:factoryClassName];
			if (!factoryClass) {
				error << _("AUPluginUI: error getting AU view's factory class from bundle") << endmsg;
				return -1;
			}
			
			// make sure 'factoryClass' implements the AUCocoaUIBase protocol
			if (!plugin_class_valid (factoryClass)) {
				error << _("AUPluginUI: U view's factory class does not properly implement the AUCocoaUIBase protocol") << endmsg;
				return -1;
			}
			// make a factory
			id factoryInstance = [[[factoryClass alloc] init] autorelease];
			if (factoryInstance == nil) {
				error << _("AUPluginUI: Could not create an instance of the AU view factory") << endmsg;
				return -1;
			}

			// make a view
			au_view = [factoryInstance uiViewForAudioUnit:*au->get_au() withSize:crect.size];
			
			// cleanup
			[CocoaViewBundlePath release];
			if (cocoaViewInfo) {
				UInt32 i;
				for (i = 0; i < numberOfClasses; i++)
					CFRelease(cocoaViewInfo->mCocoaAUViewClass[i]);
				
				free (cocoaViewInfo);
			}
			wasAbleToLoadCustomView = YES;
		}
	}

	if (!wasAbleToLoadCustomView) {
		// [B] Otherwise show generic Cocoa view
		au_view = [[AUGenericView alloc] initWithAudioUnit:*au->get_au()];
		[(AUGenericView *)au_view setShowsExpertParameters:YES];
	}

	/* make a child cocoa window */

	cocoa_window = [[NSWindow alloc] 
			initWithContentRect:crect
			styleMask:NSBorderlessWindowMask
			backing:NSBackingStoreBuffered
			defer:NO];

	return 0;
}

int
AUPluginUI::create_carbon_view (bool generic)
{
	OSStatus err;
	ControlRef root_control;

	Component editComponent = FindNextComponent(NULL, &carbon_descriptor);
	
	OpenAComponent(editComponent, &editView);
	if (!editView) {
		error << _("AU Carbon view: cannot open AU Component") << endmsg;
		return -1;
	}
	
	Rect r = { 100, 100, 100, 100 };
	WindowAttributes attr = WindowAttributes (kWindowStandardHandlerAttribute |
						  kWindowCompositingAttribute|
						  kWindowNoShadowAttribute|
						  kWindowNoTitleBarAttribute);

	if ((err = CreateNewWindow(kDocumentWindowClass, attr, &r, &carbon_window)) != noErr) {
		error << string_compose (_("AUPluginUI: cannot create carbon window (err: %1)"), err) << endmsg;
		return -1;
	}
	
	if ((err = GetRootControl(carbon_window, &root_control)) != noErr) {
		error << string_compose (_("AUPlugin: cannot get root control of carbon window (err: %1)"), err) << endmsg;
		return -1;
	}

	ControlRef viewPane;
	Float32Point location  = { 0.0, 0.0 };
	Float32Point size = { 0.0, 0.0 } ;

	if ((err = AudioUnitCarbonViewCreate (editView, *au->get_au(), carbon_window, root_control, &location, &size, &viewPane)) != noErr) {
		error << string_compose (_("AUPluginUI: cannot create carbon plugin view (err: %1)"), err) << endmsg;
		return -1;
	}

	// resize window

	Rect bounds;
	GetControlBounds(viewPane, &bounds);
	size.x = bounds.right-bounds.left;
	size.y = bounds.bottom-bounds.top;
	SizeWindow(carbon_window, (short) (size.x + 0.5), (short) (size.y + 0.5),  true);

	prefwidth = (int) (size.x + 0.5);
	prefheight = (int) (size.y + 0.5);

#if 0	
	mViewPaneResizer->WantEventTypes (GetControlEventTarget(mAUViewPane), GetEventTypeCount(resizeEvent), resizeEvent);
#endif
	return 0;
}

NSWindow*
AUPluginUI::get_nswindow ()
{
	Gtk::Container* toplevel = get_toplevel();

	if (!toplevel || !toplevel->is_toplevel()) {
		error << _("AUPluginUI: no top level window!") << endmsg;
		return 0;
	}

	NSWindow* true_parent = gdk_quartz_window_get_nswindow (toplevel->get_window()->gobj());

	if (!true_parent) {
		error << _("AUPluginUI: no top level window!") << endmsg;
		return 0;
	}

	return true_parent;
}

void
AUPluginUI::activate ()
{
	NSWindow* win = get_nswindow ();
	[win setLevel:NSFloatingWindowLevel];
	
	if (carbon_parented) {
		[cocoa_parent makeKeyAndOrderFront:nil];
		ActivateWindow (carbon_window, TRUE);
	} 
}

void
AUPluginUI::deactivate ()
{
	/* nothing to do here */
}


OSStatus 
_carbon_event (EventHandlerCallRef nextHandlerRef, EventRef event, void *userData) 
{
	return ((AUPluginUI*)userData)->carbon_event (nextHandlerRef, event);
}

OSStatus 
AUPluginUI::carbon_event (EventHandlerCallRef nextHandlerRef, EventRef event)
{
	UInt32 eventKind = GetEventKind(event);
	ClickActivationResult howToHandleClick;
	NSWindow* win = get_nswindow ();

	switch (eventKind) {
	case kEventWindowHandleActivate:
		[win makeMainWindow];
		return eventNotHandledErr;
		break;

	case kEventWindowHandleDeactivate:
		return eventNotHandledErr;
		break;
		
	case kEventWindowGetClickActivation:
		howToHandleClick = kActivateAndHandleClick;
		SetEventParameter(event, kEventParamClickActivation, typeClickActivationResult, 
				  sizeof(ClickActivationResult), &howToHandleClick);
		break;
	}

	return noErr;
}

int
AUPluginUI::parent_carbon_window ()
{
	NSWindow* win = get_nswindow ();
	int x, y;

	if (!win) {
		return -1;
	}

	Gtk::Container* toplevel = get_toplevel();

	if (!toplevel || !toplevel->is_toplevel()) {
		error << _("AUPluginUI: no top level window!") << endmsg;
		return -1;
	}
	
	toplevel->get_window()->get_root_origin (x, y);

	/* compute how tall the title bar is, because we have to offset the position of the carbon window
	   by that much.
	*/

	NSRect content_frame = [NSWindow contentRectForFrameRect:[win frame] styleMask:[win styleMask]];
	NSRect wm_frame = [NSWindow frameRectForContentRect:content_frame styleMask:[win styleMask]];

	int titlebar_height = wm_frame.size.height - content_frame.size.height;

	MoveWindow (carbon_window, x, y + titlebar_height, false);
	ShowWindow (carbon_window);

	// create the cocoa window for the carbon one and make it visible
	cocoa_parent = [[NSWindow alloc] initWithWindowRef: carbon_window];

	EventTypeSpec	windowEventTypes[] = {
		{kEventClassWindow, kEventWindowGetClickActivation },
		{kEventClassWindow, kEventWindowHandleDeactivate },
		{kEventClassWindow, kEventWindowHandleActivate }
	};
	
	EventHandlerUPP   ehUPP = NewEventHandlerUPP(_carbon_event);
	OSStatus result = InstallWindowEventHandler (carbon_window, ehUPP, 
						     sizeof(windowEventTypes) / sizeof(EventTypeSpec), 
						     windowEventTypes, this, &carbon_event_handler);
	if (result != noErr) {
		return -1;
	}

	[win addChildWindow:cocoa_parent ordered:NSWindowAbove];
	[win setLevel:NSFloatingWindowLevel];
	[win setHidesOnDeactivate:YES];

	carbon_parented = true;
		
	return 0;
}	

int
AUPluginUI::parent_cocoa_window ()
{
	NSWindow* win = get_nswindow ();

	if (!win) {
		return -1;
	}

	Gtk::Container* toplevel = get_toplevel();

	if (!toplevel || !toplevel->is_toplevel()) {
		error << _("AUPluginUI: no top level window!") << endmsg;
		return -1;
	}
	
	// Get the size of the new AU View's frame 
	NSRect au_view_frame = [au_view frame];

	if (au_view_frame.size.width > 500 || au_view_frame.size.height > 500) {
		
		/* its too big - use a scrollview */

		NSRect frameRect = [[cocoa_window contentView] frame];
		scroll_view = [[[NSScrollView alloc] initWithFrame:frameRect] autorelease];
		[scroll_view setDrawsBackground:NO];
		[scroll_view setHasHorizontalScroller:YES];
		[scroll_view setHasVerticalScroller:YES];

		NSSize frameSize = [NSScrollView  frameSizeForContentSize:au_view_frame.size
				    hasHorizontalScroller:[scroll_view hasHorizontalScroller]
				    hasVerticalScroller:[scroll_view hasVerticalScroller]
				    borderType:[scroll_view borderType]];
		
		// Create a new frame with same origin as current
		// frame but size equal to the size of the new view
		NSRect newFrame;
		newFrame.origin = [scroll_view frame].origin;
		newFrame.size = frameSize;
		
		// Set the new frame and document views on the scroll view
		NSRect currentFrame = [scroll_view frame];
		[scroll_view setFrame:newFrame];
		[scroll_view setDocumentView:au_view];
		
		cerr << "scroll view size is " << newFrame.size.width << " x " << newFrame.size.height << endl;
		
		NSSize oldContentSize = [[cocoa_window contentView] frame].size;
		NSSize newContentSize = oldContentSize;
		
		cerr << "original size is " << newContentSize.width << " x " << newContentSize.height << endl;
		
		newContentSize.width += (newFrame.size.width - currentFrame.size.width);
		newContentSize.height += (newFrame.size.height - currentFrame.size.height);
		
		[cocoa_window setContentSize:newContentSize];
		[cocoa_window setContentView:scroll_view];
		
	} else {

		[cocoa_window setContentSize:au_view_frame.size];
		[cocoa_window setContentView:au_view];

	}

	/* compute how tall the title bar is, because we have to offset the position of the child window
	   by that much.
	*/

	NSRect content_frame = [NSWindow contentRectForFrameRect:[win frame] styleMask:[win styleMask]];
	NSRect wm_frame = [NSWindow frameRectForContentRect:content_frame styleMask:[win styleMask]];
	int titlebar_height = wm_frame.size.height - content_frame.size.height;

	// move cocoa window into position relative to the toplevel window

	NSRect view_frame = [[cocoa_window contentView] frame];
	view_frame.origin.x = content_frame.origin.x;
	view_frame.origin.y = content_frame.origin.y;

	[cocoa_window setFrame:view_frame display:NO];

	/* make top level window big enough to hold cocoa window and titlebar */
	
	content_frame.size.width = view_frame.size.width;
	content_frame.size.height = view_frame.size.height + titlebar_height;

	[win setFrame:content_frame display:NO];

	/* now make cocoa window a child of this top level */

	[win addChildWindow:cocoa_window ordered:NSWindowAbove];
	// [win setLevel:NSFloatingWindowLevel];
	[win setHidesOnDeactivate:YES];

	cocoa_parented = true;

	return 0;
}

void
AUPluginUI::on_realize ()
{
	VBox::on_realize ();

	if (cocoa_window) {
		
		if (parent_cocoa_window ()) {
		}

	} else if (carbon_window) {

		if (parent_carbon_window ()) {
			// ShowWindow (carbon_window);
		}
	}
}

void
AUPluginUI::on_show ()
{
	cerr << "AU plugin window shown\n";

	VBox::on_show ();

	if (cocoa_window) {
		[cocoa_window setIsVisible:YES];
	} else if (carbon_window) {
		[cocoa_parent setIsVisible:YES];
	}
}

bool
AUPluginUI::start_updating (GdkEventAny* any)
{
	return false;
}

bool
AUPluginUI::stop_updating (GdkEventAny* any)
{
	return false;
}

PlugUIBase*
create_au_gui (boost::shared_ptr<PluginInsert> plugin_insert, VBox** box)
{
	AUPluginUI* aup = new AUPluginUI (plugin_insert);
	(*box) = aup;
	return aup;
}
