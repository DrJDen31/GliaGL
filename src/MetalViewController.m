// ==================================================================
// Apple Metal View Controller
// ==================================================================

@import AppKit;
#define PlatformViewController NSViewController
@import MetalKit;

#import "MetalRenderer.h"
#import "MetalMTKView.h"

@interface MetalViewController : PlatformViewController
@end


@implementation MetalViewController
{
  MetalMTKView *_view;
  MetalRenderer *_renderer;
}

- (void)viewDidLoad
{
  [super viewDidLoad];

    // Set the view to use the default device
    _view = (MetalMTKView *)self.view;
    _view.device = MTLCreateSystemDefaultDevice();
    
    if(!_view.device)
    {
        NSLog(@"Metal is not supported on this device");
        return;
    }

    _renderer = [[MetalRenderer alloc] initWithMetalKitView:_view];

    if(!_renderer)
    {
        NSLog(@"Renderer failed initialization");
        return;
    }

    // Initialize our renderer with the view size
    [_renderer mtkView:_view drawableSizeWillChange:_view.drawableSize];

    _view.delegate = _renderer;
}

@end
