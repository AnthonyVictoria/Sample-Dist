#include "MacDropTarget.h"

#if defined(__APPLE__)
 #import <Cocoa/Cocoa.h>

@interface ShaqDropCatcher : NSView
@property (nonatomic, copy) void (^callback)(NSString* path);
@end

@implementation ShaqDropCatcher
- (instancetype)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self != nil)
    {
        [self registerForDraggedTypes:@[
            NSPasteboardTypeFileURL,
            NSPasteboardTypeURL,
            NSPasteboardTypeString,
            @"NSFilenamesPboardType",
            @"public.file-url"
        ]];
    }
    return self;
}

- (NSView*)hitTest:(NSPoint)point
{
    (void) point;
    return nil;
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender
{
    (void) sender;
    return NSDragOperationCopy;
}

- (BOOL)prepareForDragOperation:(id<NSDraggingInfo>)sender
{
    (void) sender;
    return YES;
}

- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender
{
    NSPasteboard* pb = [sender draggingPasteboard];
    NSDictionary* opts = @{ NSPasteboardURLReadingFileURLsOnlyKey: @YES };
    NSArray* urls = [pb readObjectsForClasses:@[ NSURL.class ] options:opts];
    for (NSURL* url in urls)
    {
        if (url.isFileURL && self.callback != nil)
        {
            self.callback(url.path);
            return YES;
        }
    }

    for (NSString* type in @[ NSPasteboardTypeString, @"NSFilenamesPboardType" ])
    {
        NSString* text = [pb stringForType:type];
        if (text.length > 0 && self.callback != nil)
        {
            self.callback(text);
            return YES;
        }
    }
    return NO;
}
@end

void installMacDropTarget(void* nativeView, std::function<void(const std::string&)> callback)
{
    if (nativeView == nullptr)
        return;

    NSView* view = (NSView*) nativeView;
    [view registerForDraggedTypes:@[
        NSPasteboardTypeFileURL,
        NSPasteboardTypeURL,
        NSPasteboardTypeString,
        @"NSFilenamesPboardType",
        @"public.file-url"
    ]];

    ShaqDropCatcher* catcher = [[ShaqDropCatcher alloc] initWithFrame:view.bounds];
    catcher.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    catcher.callback = ^(NSString* path)
    {
        std::string p(path.UTF8String);
        callback(p);
    };
    [view addSubview:catcher positioned:NSWindowBelow relativeTo:nil];
}
#else
void installMacDropTarget(void*, std::function<void(const std::string&)>) {}
#endif
