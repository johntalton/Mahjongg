/*******************************************************
*   Mahjongg©
*
*   A classic old game.
*
*   @author  TheAbstractCompany, YNOP(ynop@acm.org) 
*   @vertion beta
*   @date    Feb 17 2000
*******************************************************/
#include <Application.h>
#include <AppKit.h>
#include <InterfaceKit.h>
#include <storage/Path.h>
#include <storage/FilePanel.h>
#include <String.h>

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "Globals.h"
#include "MWindow.h"
#include "MView.h"
#include "TPreferences.h"
extern TPreferences prefs;
/*******************************************************
*   Our wonderful BWindow, ya its kewl like that.
*   we dont do much here but set up the menubar and 
*   let the view take over.  We also nead some message
*   redirection and handling
*******************************************************/
MWindow::MWindow(BRect frame) : BWindow(frame,"Mahjongg",B_TITLED_WINDOW,B_ASYNCHRONOUS_CONTROLS|B_NOT_RESIZABLE|B_NOT_ZOOMABLE){
   BRect r;
   BMenu *menu;
   BMenuItem *item;
   
   r = Bounds();
   // Creat a standard menubar
   menubar = new BMenuBar(r, "MenuBar");
   // Standard File menu
   menu = new BMenu("Game");
   // menu->AddItem(new BSeparatorItem());
   //menu->AddItem(item=new BMenuItem("New Game", new BMessage(NEW_GAME), 'N'));
   BMenu *subm = new BMenu("New Game");
   subm->AddItem(new BMenuItem("Tower Mask",new BMessage(TOWER)));
   subm->AddItem(new BMenuItem("Pyramid Mask",new BMessage(PYRAMIDE)));
   subm->AddItem(new BMenuItem("Triangle Mask",new BMessage(TRIANGLE)));
   subm->AddItem(new BMenuItem("Classic Mask",new BMessage(CLASSIC)));
   subm->AddItem(new BMenuItem("Castle Mask",new BMessage(CASTLE)));
   subm->AddItem(new BMenuItem("Dragon Bridge Mask",new BMessage(DRAGON_BRIDGE)));
   subm->AddItem(new BMenuItem("Rice Bowl Mask",new BMessage(RICEBOWL)));
   menu->AddItem(item = new BMenuItem(subm,new BMessage(NEW_GAME)));
   item->SetShortcut('N',0);
   //menu->AddItem(item=new BMenuItem("Help Me", new BMessage(HELP_ME), 'H'));
   //menu->AddItem(item=new BMenuItem("Demo Mode", new BMessage(DEMO), 'D'));
  
   menu->AddItem(new BSeparatorItem()); 
   menu->AddItem(item=new BMenuItem("About...", new BMessage(B_ABOUT_REQUESTED), 'A'));
   item->SetTarget(be_app);
   menu->AddItem(new BSeparatorItem());
   menu->AddItem(item=new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q'));
   // add File menu now
   menubar->AddItem(menu);
    
   menu = new BMenu("Options");
   menu->AddItem(item=new BMenuItem("Load Background Image...", new BMessage(BACK_GROUND), 'L'));
   menu->AddItem(item=new BMenuItem("Load Tile Set...", new BMessage(TILE_SET), 'T'));
   //item->SetEnabled(false);
   menu->AddItem(snditem = new BMenuItem("Sound is ON",new BMessage(TOGGLE_SOUND)));
   menu->AddItem(transitem = new BMenuItem("Transparency is ON",new BMessage(TOGGLE_TRANS)));
   menu->AddItem(new BSeparatorItem());
   menu->AddItem(item=new BMenuItem("Show Matching Tiles", new BMessage(SHOW_MATCH), 'M'));
   menu->AddItem(item=new BMenuItem("Undo Last Remove", new BMessage(UNDO), 'U'));
   menu->AddItem(item=new BMenuItem("Shuffle Existing Tiles", new BMessage(RESHUFFLE), 'S'));
   menu->AddItem(item=new BMenuItem("Hide Locked Tiles", new BMessage(HIDELOCKED), 'H'));
   menu->AddItem(item=new BMenuItem("Ask For Hint!", new BMessage(HINT), 'I'));
   menubar->AddItem(menu);
   
   menu = new BMenu("Help");
   menu->AddItem(item=new BMenuItem("Web page", new BMessage(HELP)));
   menubar->AddItem(menu);
   
   
   
   
   
   // Attach the menu bar to he window
   AddChild(menubar);
   
  // BRect b = Bounds();
  // StatusBar = new BBox(BRect(b.left-3,b.bottom-14,b.right+3,b.bottom+3),"StatusBar",B_FOLLOW_LEFT_RIGHT|B_FOLLOW_BOTTOM); 
  // AddChild(StatusBar);
   
   
   // Do a little claculating to take menubar int account
   r = Bounds();
   r.bottom = r.bottom - menubar->Bounds().bottom ;

   View = new MView(r);
   View->MoveBy(0, menubar->Bounds().Height() + 1);

   AddChild(View);
   //View->MakeFocus(true);
   Show();
   
   entry_ref AppRef;
   app_info ai;
   be_app->GetAppInfo(&ai);
   BEntry entry(&ai.ref);
   entry.GetParent(&entry);
   entry.GetRef(&AppRef);
   openfd = new BFilePanel(B_OPEN_PANEL,new BMessenger(this),&AppRef,B_FILE_NODE,false,new BMessage(DO_BG),NULL,false,true);

   opentileset = new BFilePanel(B_OPEN_PANEL,new BMessenger(this),&AppRef,B_FILE_NODE,false,new BMessage(DO_TS),NULL,false,true);
   
}

/*******************************************************
*  
*******************************************************/
void MWindow::FrameResized(float,float){
}

/*******************************************************
*  
*******************************************************/
void MWindow::WindowActivated(bool active){
}

/*******************************************************
*   More nothingness. pass menu msg down to View.
*   like new game and pause and stuff like that.
*******************************************************/
void MWindow::MessageReceived(BMessage* msg){
   BString tmpS;
   char *ad;
   
//   const char *name;
   entry_ref dir;
   BPath path;
   
   switch(msg->what){
   case TILE_SET:
      opentileset->Show();
      break;
   case DO_TS:
      if (msg->FindRef("refs", &dir) != B_OK) return;
      path.SetTo(&dir);
      if(path.InitCheck() != B_OK){
         (new BAlert(NULL,"Failed to set dir","Bummer"))->Go();
         return;
      }
      View->SetTileSet(path);
      break;
   case BACK_GROUND:
      openfd->Show();
      break;
   case DO_BG:
      if (msg->FindRef("refs", &dir) != B_OK) return;
      path.SetTo(&dir);
      if(path.InitCheck() != B_OK){
         (new BAlert(NULL,"Failed to set dir","Bummer"))->Go();
         return;
      }
      View->SetBG(path);
      break;

   case TOGGLE_TRANS:
      if(!strcmp(transitem->Label(),"Transparency is ON")){
         transitem->SetLabel("Transparency is OFF");
      }else{
         transitem->SetLabel("Transparency is ON");
      }
      View->MessageReceived(msg);
      break;

   case TOGGLE_SOUND:
      if(!strcmp(snditem->Label(),"Sound is ON")){
         snditem->SetLabel("Sound is OFF");
      }else{
         snditem->SetLabel("Sound is ON");
      }
      View->MessageReceived(msg);
      break;
   case TOWER:
   case PYRAMIDE:
   case TRIANGLE:
   case CLASSIC:
   case CASTLE:
   case DRAGON_BRIDGE:
   case RICEBOWL:
   case NEW_GAME:
   case RESHUFFLE:
   case UNDO:
   case HIDELOCKED:
   case SHOW_MATCH:
   case HINT:
      View->MessageReceived(msg);
      break;
   case HELP:
      tmpS.SetTo("http://ynop.yi.org/BeOS/Mahjongg/index.html");
      ad = (char*)tmpS.String(); 
      be_roster->Launch("text/html",1,&ad);
      break;
   default:
      BWindow::MessageReceived(msg);
   }
}

/*******************************************************
*   Someone asked us nicely to quit. I gess we should
*   so clean up. save our setings (position of win)
*   and tell the main be_app to shut it all down .. bye
*******************************************************/
bool MWindow::QuitRequested(){
   if (prefs.InitCheck() != B_OK) {
   }
   prefs.SetRect("window_pos", Frame());
   be_app->Lock();
   be_app->Quit();
   be_app->Unlock();
   return true;
}
