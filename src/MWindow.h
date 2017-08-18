#ifndef _M_WINDOW_H
#define _M_WINDOW_H

#include <Application.h>
#include <AppKit.h>
#include <InterfaceKit.h>

#include "MView.h"


class MWindow : public BWindow {
   public:
      MWindow(BRect);
      virtual void MessageReceived(BMessage*);
      virtual bool QuitRequested();
      virtual void FrameResized(float,float);
      virtual void WindowActivated(bool active);
   private:
      BMenuBar *menubar;
      MView *View;
      BBox *StatusBar;
      BMenuItem *snditem;
      BMenuItem *transitem;
      BFilePanel *openfd;
      BFilePanel *opentileset;
};
#endif
