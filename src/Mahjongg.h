#ifndef _MAHJONGG_H
#define _MAHJONGG_H

#include <Application.h>
#include <AppKit.h>
#include <InterfaceKit.h>

#include "MWindow.h"

extern const char *APP_SIGNATURE;
class Mahjongg : public BApplication {
   public:
      Mahjongg();
      ~Mahjongg();
      virtual void AboutRequested();
      virtual void MessageReceived(BMessage*);
      virtual bool QuitRequested();
   private:
     MWindow *theWin;
};
#endif
