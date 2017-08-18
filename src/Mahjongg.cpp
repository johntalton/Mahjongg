/*******************************************************
*   Mahjongg©
*
*   A classic old game.
*
*   @author  YNOP(ynop@nft-tv.com) 
*   @vertion beta
*   @date    Feb 17 2000
*******************************************************/
#include <AppKit.h>
#include <InterfaceKit.h>
#include <Alert.h>
#include <Application.h>

#include <stdio.h>

#include "Globals.h"
#include "Mahjongg.h"
#include "MWindow.h"
#include "TPreferences.h"

TPreferences prefs("Mahjongg_prefs");

/*******************************************************
*
*******************************************************/
Mahjongg::Mahjongg() : BApplication(APP_SIGNATURE){
   BRect wind_pos;

   BRect defaultSize(50,50,690,530);

   if (prefs.InitCheck() != B_OK) {
      // New User!
   }

   if(prefs.FindRect("window_pos", &wind_pos) != B_OK){
      wind_pos = defaultSize;
   }
   
   if(!wind_pos.Intersects(BScreen().Frame())){
      (new BAlert(NULL,"The window was somehow off the screen. We reset it position for you","Thanks"))->Go();
      theWin = new MWindow(defaultSize);
   }else{
      // this is the normal start up.
      theWin = new MWindow(wind_pos);//
   }
}

/*******************************************************
*
*******************************************************/
Mahjongg::~Mahjongg(){
}

/*******************************************************
*   Our lovely about box with hidden box
*******************************************************/
void Mahjongg::AboutRequested(){
   (new BAlert("About Mahjongg","Mahjongg ©2000-2004\n\n\nynop@nft-tv.com\n\nVersion: 1.7","Thats Nice"))->Go();
}

/*******************************************************
*   Ah .. do nothing .. just defalut pass off.
*******************************************************/
void Mahjongg::MessageReceived(BMessage *msg){
   switch(msg->what){
   default:
      BApplication::MessageReceived(msg);
      break; 
   }  
}

/*******************************************************
*   Make shure all preffs are saved and kill it all
*******************************************************/
bool Mahjongg::QuitRequested(){
   if(theWin->QuitRequested()){
      return true; // Ok .. fine .. leave then
   }
   return false;
}






