#ifndef _MAHJONGG_VIEW_H
#define _MAHJONGG_VIEW_H

#include <Application.h>
#include <AppKit.h>
#include <InterfaceKit.h>
#include <Locker.h>

#include "Globals.h"
//#include "static.h"

class MView : public BView {
	public:
		MView(BRect);
		~MView();
		void SetBG(BPath);
		void SetTileSet(BPath);
		virtual void Draw(BRect);
		virtual void MouseDown(BPoint);
		virtual void MessageReceived(BMessage*);
		static int32 GameLoader(void* obj){
			return ((MView*)obj)->LoadGame();
		}
	
		static int32 TimerLoop(void* obj){
			return ((MView*)obj)->LoopTimer();
		}
		
		int elapsedTime; //this is a counter in seconds of time since game start
		bool killtimer; //kill timer
		bool time_draw;//draw time score
		int game_score;//holds current game score
		bool start_game;//allow game to start?
		
	private:
		void DrawTile(tile_type);
		void StrokeTile(unsigned char,BRect,BRect,tile_type);
		void LoadBoard();
		int32 LoadGame();
		int32 LoopTimer();
		thread_id timer_thread;

		void DeleteBlock(tile_type);
		void Undo();
		bool IsTileLocked(tile_type);
		bool HideLockedTiles; //hide locked tiles
		bool ShowMatches; //should Matching Tiles be shown now?
		int32 ReShuffle();
		int32 FindHint();
		
		BLocker Loader;
		
		bool full;
		bool isLoading;
			
		BBitmap *OffScreen;
		BView *Drawer;
		
		BBitmap *bg;
		BBitmap *FullTiles;
		//BBitmap *t;
		BRect tbounds;
		BRect tileRect;//this holds selection face size for tiles stretching etc..
		//tile_type contains full select/tile info etc..
		//tile_type is defined in Globals.h
		tile_type Board[WIDTH][HEIGHT][LAYERS];
		char Mask[WIDTH][HEIGHT][LAYERS];
		
		unsigned char Deck[MAX_DECK_SIZE]; //this holds the tiles available to deal
		tile_type UndoBoard[MAX_DECK_SIZE]; //this holds deleted tiles
		tile_type SelectedTile; //this holds the currently selected tile
		
		int Xoff;
		int Yoff;
		
		BFont ScoreFont; //this holds the font for drawing score etc.
				
		bool FirstSelect;
		int BlockCount;
		int BlocksLeft;
		
		char *ptr;
		unsigned char *ptr_Set; //this points to a deck to use - see static.h
		unsigned char *ptr_Values; //this points to a value array for tiles - see static.h
		
		int32 seed; //this holds a user defined (or system timer) value for shuffling
				
		bool soundON;
		bool transON;
		
		entry_ref singleref;
		entry_ref doubleref;
};
#endif

