/*******************************************************
*   Mahjongg©
*
*   A classic old game.
*
*   @author  TheAbstractCompany, YNOP(ynop@acm.org) 
*   @vertion beta
*   @date    Feb 17 2000
*******************************************************/
#include <AppKit.h>
#include <InterfaceKit.h>
#include <TranslationUtils.h>
#include <String.h>
#include <PlaySound.h>
#include <storage/Path.h>

#include <stdio.h>
#include <stdlib.h>
//#include <memory.h>

#include "Globals.h"
#include "MView.h"
#include "static.h"
#include "TPreferences.h"
extern TPreferences prefs;

/*******************************************************
*
*******************************************************/
MView::MView(BRect frame):BView(frame, "", B_FOLLOW_ALL_SIDES, B_WILL_DRAW){//|B_FRAME_EVENTS
	SetViewColor(B_TRANSPARENT_32_BIT);
	
	BPath AppPath;
	app_info ai;
	be_app->GetAppInfo(&ai);
	BEntry entry(&ai.ref);
	entry.GetPath(&AppPath);
	AppPath.GetParent(&AppPath);
	
	BPath tmp;
	BString str;
	
	//try to load prefs for background
	if(prefs.FindString("default_tiles",&str) != B_OK){
		tmp = AppPath;
		tmp.Append("TileSets/TileSet.png");
	}else{
		tmp.SetTo(str.String());
	}
	
	FullTiles = BTranslationUtils::GetBitmap(tmp.Path());
	if(FullTiles==NULL){
		(new BAlert(NULL,"someone's deleted your default Tileset!","Oops!"))->Go();
		//set tile bmp to blank bmp.
		FullTiles = new BBitmap(BRect(0.0, 0.0, 9.0, 5.0), B_RGB32);
	}	
	
	//try to load prefs for background
	if(prefs.FindString("default_bg", &str) != B_OK){
		tmp = AppPath;
		tmp.Append("BackGrounds/bgnd.bmp");
	}else{
		tmp.SetTo(str.String());
	}
	
	bg = BTranslationUtils::GetBitmap(tmp.Path());
	if(bg==NULL){
		(new BAlert(NULL,"someone's deleting your default Background and haven't bothered to replace it!","Oops!"))->Go();
		//set tile bmp to blank bmp.
		bg = new BBitmap(BRect(0, 0, 50, 50), B_RGB32);
   	}	
	
	BRect b = Bounds();
	//Set up off screen canvis
	OffScreen = new BBitmap(b,B_RGB32,true);
	OffScreen->Lock();
	Drawer = new BView(b,"drawer",B_FOLLOW_NONE,0);
	OffScreen->AddChild(Drawer);
	OffScreen->Unlock();

	Drawer->GetFont(&ScoreFont);

	//set bounds for tiles and offsets
	tbounds.Set(0,0,36,52);
	Xoff = (int)(b.Width() - (WIDTH * tbounds.Width())) /2;
	Yoff = (int)(b.Height() - (HEIGHT * tbounds.Height())) /2;

	//set up face grab / stretch size for each tile
	BRect setsize = FullTiles->Bounds();
	tileRect.Set(0,0,setsize.IntegerWidth() / 9,setsize.IntegerHeight() / 5);
	 
	//set default game type
	HideLockedTiles=false;
	ptr = ClassicMask;
	ptr_Set = Classic_Set;
	ptr_Values = Classic_Values;
	seed = system_time();
	LoadBoard();
	
	soundON = true;
	transON = true;

	game_score=0;
	elapsedTime=0;
	killtimer=false;
	time_draw=false;
	start_game=false;
	//set up timer thread to loop continuously until kill sent
	timer_thread = spawn_thread(TimerLoop, "score_timer", B_LOW_PRIORITY, this);

	full = true; // do a full draw the first time

	tmp = AppPath;
	tmp.Append("click-single.wav");
	BEntry sndentry(tmp.Path());
	sndentry.GetRef(&singleref);
	tmp = AppPath;
	tmp.Append("click-double.wav");
	sndentry.SetTo(tmp.Path());
	sndentry.GetRef(&doubleref);
}

/*******************************************************
*
*******************************************************/
MView::~MView(){
	full=false;
	time_draw=false;
	killtimer=true;
	kill_thread(timer_thread);
	status_t *exitval=0;
	wait_for_thread(timer_thread,exitval);//wait for clock timer to die
	Loader.Lock();
	//this should fource use to wait for the game to load 
	// before deleteing it
	Loader.Unlock();
}


/*******************************************************
*
*******************************************************/
void MView::SetBG(BPath path){
	BBitmap *newBG = BTranslationUtils::GetBitmap(path.Path());
	if(newBG != NULL){
		// its a good image .. run with it
		if(bg != NULL){ delete bg; }
		bg = newBG;
		full = true;

		//try to save prefs for background
		prefs.SetString("default_bg", path.Path());
		      
		Invalidate();
	}
	else{
		(new BAlert(NULL,"Couldn't translate that file","Bummer"))->Go();
	}
}


/*******************************************************
*
*******************************************************/
void MView::SetTileSet(BPath path){
	BBitmap *newTiles = BTranslationUtils::GetBitmap(path.Path());
	if(newTiles != NULL){
		// its a good image .. run with it
		if(FullTiles != NULL){ delete FullTiles; }
		FullTiles = newTiles;

		//set up face grab / stretch size for each tile
		BRect setsize = FullTiles->Bounds();
		tileRect.Set(0,0,setsize.IntegerWidth() / 9,setsize.IntegerHeight() / 5);

		full = true;

		//try to save prefs for tiles
		prefs.SetString("default_tiles",path.Path());

		Invalidate();
   }else{
      (new BAlert(NULL,"Could not load requested tileset!","Bummer"))->Go();
   }
}

/*******************************************************
*
*******************************************************/
void MView::LoadBoard( ){
	//load in pattern for board layout
	int32 c = 0;
	BlockCount = 0;
	for(int z = 0; z < LAYERS;z++){
		for(int y = 0;y < HEIGHT;y++){
			for(int x = 0; x < WIDTH;x++){
				Mask[x][y][z] = ptr[c++]; //load mask from static.h
				if(Mask[x][y][z] != '.'){
					BlockCount++;
				}
				Board[x][y][z].face = 255; //set all tiles to not show
			}
		}
	}
	

	/*	set random seed value according to user / system decision
		this allows a deck to match a scatter pattern if a user
		defined seed is chosen.
	*/
	seed=system_time();
	srand(seed);	

	//load in allowed tile faces to create deck
	if (ptr_Set[0]==255){  //if this is a random set then fill at random
		for(int x = 0; x < MAX_DECK_SIZE; x++){
			Deck[x]=rand() % 44; //select any one of 44 tiles - 
								//tile 45 is used for displaying locked status
			Deck[x+1]=Deck[x]; //add an identical tile to ensure matches
			x++;
		}
	}
	else { //this is a planned set - fill according to schedule
		int t=0;
		c=0;
		while(t < 45){
			for(int x = 0; x < ptr_Set[t]; x++){ //loop till tile count is reached
				if(c < MAX_DECK_SIZE) 
				/* 	the above is to prevent overflow in future versions where users
					are able to make their own patterns and decks
				*/
				{
					Deck[c]=t;
					c++;
				}
			}
			t++;
		}
	}
}


/*******************************************************
*
*******************************************************/
int32 MView::LoadGame(){
	//this is responsible for creating tile arrays
	//here tiles are shuffled, and dealt - depending on the tileset used

	/*	in this series of loops all tiles from the lowest layer to the top
		are laid out.
		1. if Mask[x][y][z]!="." then 2.
		2. a tile is randomly selected from the Deck[] array
		3. if chosen tile is not already in the used[] array then
		3. the tile information is entered into the Board[x][y][z]
		4. at this point additional attributes relating to the tile are
		   also set, things such as selected, hitZone, and x,y,z,value data	
	*/
	
	if(BlockCount > MAX_DECK_SIZE){
		/*	this should NEVER happen - unless Layers 6+ are enabled in a future version
			If this was not here and BlockCount was greater an infinite loop can
			can result inside the dealing routine.  If Layers 6+ are enabled then an
			additional check should be placed inside LoadBoard() to warn of overrun
			between pattern tiles and actual deck tiles.
		*/
		return B_ERROR;
	}

	Loader.Lock();
	
	bool used[BlockCount];
	for(int c=0;c<BlockCount;c++)used[c]=false; //clear usage states

	seed=system_time();
	srand(seed);//initialise random sequence with user/system seed
	int dt; //this will hold the tile randomly drawn from the deck
	int x=0,y=0,z=0;

	for(z = 0; z < LAYERS;z++){
		for(y = 0; y < HEIGHT;y++){
			for(x = 0; x < WIDTH;x++){

				if(Mask[x][y][z] != '.'){
				
					dt=rand() % BlockCount;
					while(used[dt]==true)dt=rand() % BlockCount;
					
					//set all tile attributes
					Board[x][y][z].face = Deck[dt];
					Board[x][y][z].value = ptr_Values[Board[x][y][z].face];
					Board[x][y][z].x = x;
					Board[x][y][z].y = y;
					Board[x][y][z].z = z;
					Board[x][y][z].selected=false;
					used[dt]=true;
				}
				else {
					//set tile attributes - this is more to clear them than any use involved
					Board[x][y][z].face = 255;
					Board[x][y][z].value = 255;
					Board[x][y][z].x = x;
					Board[x][y][z].y = y;
					Board[x][y][z].z = z;
					Board[x][y][z].selected=false;
				}
							
				//decide where exactly the tiles' rect hitZone is.
				BRect dest;
				BRect tb = tbounds;
				switch (Mask[x][y][z])
				{
					case '.': //even set non-tile zones. probably not necessary
					case 'N':
						dest.left = x*tb.right+(z*SPACE) +Xoff;
						dest.top = y*tb.bottom-(z*SPACE) + Yoff;
						break;
						
					case 'X':
						dest.left = (x+0.5)*tb.right+(z*SPACE) +Xoff;
						dest.top = y*tb.bottom-(z*SPACE) + Yoff;
						break;
		
					case 'Y':
						dest.left = x*tb.right+(z*SPACE) +Xoff;
						dest.top = (y-0.5)*tb.bottom-(z*SPACE) + Yoff;
						break;
		
					case 'Z':
						dest.left = (x+0.5)*tb.right+(z*SPACE) +Xoff;
						dest.top = (y-0.5)*tb.bottom-(z*SPACE) + Yoff;
						break;

					case 'V':
						dest.left = x*tb.right+(z*SPACE) +Xoff;
						dest.top = (y+0.5)*tb.bottom-(z*SPACE) + Yoff;
						break;

					case 'U':
						dest.left = (x+0.5)*tb.right+(z*SPACE) +Xoff;
						dest.top = (y+0.5)*tb.bottom-(z*SPACE) + Yoff;
						break;
				}//end of switch BRect hitZone setup

				dest.right = dest.left + tb.right -1; 
				dest.bottom = dest.top + tb.bottom -1;
				Board[x][y][z].hitZone=dest;

			}//end of x loop
		}//end of y loop
	}//end of z loop

	//set misc variables
	BlocksLeft = BlockCount;
	FirstSelect = true;
	game_score=0;
	elapsedTime=0;

	//force board redraw
	time_draw=false;
	Window()->Lock();
	full = true;
	Invalidate();
	Window()->Unlock();
	Flush();

	isLoading = false;
	Loader.Unlock();
	start_game=true;
	resume_thread(timer_thread);
	return B_OK;
}


/*******************************************************
*
*******************************************************/
void MView::DrawTile(tile_type tile){
	BRect dest;	
	dest=tile.hitZone;
	   
	if(tile.face == 255)
	{
		if(tile.z == 0)
		{
			Drawer->SetHighColor(255,9,9);
		}
		return;
	}
   
	StrokeTile(tile.face,BRect(0,0,tileRect.right,tileRect.bottom),dest,tile);
	
	//create shaded sides as polygon
	/*		 _________
		   /| 	      |
		 /| |	      |
		|#| |		  |
		|#| |		  |
		|#| |		  |
		|#| |	 	  |
		|#| |_________|
		|#|/_________/
		|###########/
		 
	*/
	BPoint pt_list[6];
	pt_list[0].x=dest.left-1;
	pt_list[0].y=dest.top;
	pt_list[1].x=dest.left-1-SPACE;
	pt_list[1].y=dest.top+SPACE;
	pt_list[2].x=dest.left-1-SPACE;
	pt_list[2].y=dest.bottom+1+SPACE;
	pt_list[3].x=dest.right-SPACE;
	pt_list[3].y=dest.bottom+1+SPACE;
	pt_list[4].x=dest.right;
	pt_list[4].y=dest.bottom+1;
	pt_list[5].x=dest.left-1;
	pt_list[5].y=dest.bottom+1;
	
	BPolygon poly;
	poly.AddPoints(pt_list,6);
	
	if(!tile.locked && ShowMatches && transON && !FirstSelect && tile.value==SelectedTile.value){
		Drawer->SetDrawingMode(B_OP_ADD);
	}
	else {
		Drawer->SetDrawingMode(B_OP_COPY);
	}
	
	
	Drawer->SetHighColor(132,96,67);//136,128,108);
	Drawer->FillPolygon(&poly,B_SOLID_HIGH);
	
	//reset highlight edges so they cut into corners for rounded effect..
	pt_list[5].x++;
	pt_list[5].y--;
	pt_list[4].x++;
	pt_list[4].y--;
	pt_list[0].x++;
	pt_list[0].y--;
	Drawer->SetHighColor(179,145,111);//166,158,138);
	// !! these are 1 too high
	Drawer->StrokeLine(pt_list[0], pt_list[1],B_SOLID_HIGH);
	Drawer->StrokeLine(pt_list[2], pt_list[5],B_SOLID_HIGH);
	Drawer->StrokeLine(pt_list[3], pt_list[4],B_SOLID_HIGH);
	
	//Drop Shadow
	//now redefine points 0,4,5 to enlarge shade area
	pt_list[0].x=pt_list[1].x-SPACE;
	pt_list[0].y=pt_list[1].y+SPACE;
	pt_list[4].x=pt_list[3].x-SPACE;
	pt_list[4].y=pt_list[3].y+SPACE;
	pt_list[5].x=pt_list[0].x;
	pt_list[5].y=pt_list[4].y;
	
	// !! this color should match tile set
	Drawer->SetDrawingMode(B_OP_ALPHA);
	Drawer->SetHighColor(50,50,50,100);
    //Drawer->SetHighColor(255,0,0);

	BPolygon poly_shade;
	poly_shade.AddPoints(pt_list,6);
	Drawer->FillPolygon(&poly_shade,B_SOLID_HIGH);
	
	//check if tile is selected and NOT ShowMatches then- if so then shade tile!
	if(tile.selected==true && !ShowMatches){
		Drawer->SetDrawingMode(B_OP_ALPHA);
		Drawer->SetHighColor(64,64,64,100);//126,93,61,100
		Drawer->FillRect(tile.hitZone,B_SOLID_HIGH);		
	}
	
	Drawer->SetDrawingMode(B_OP_OVER);
   
	Drawer->Sync();
   
}

/*******************************************************
*
*******************************************************/
void MView::StrokeTile(unsigned char face,BRect src,BRect dest,tile_type tile){
	
	Drawer->SetDrawingMode(B_OP_OVER);
	if(IsTileLocked(tile)){
		if(HideLockedTiles)face=44;
	}
	
	BRect ts = tileRect;
	BRect tmp = src;
	
	tmp.left = ((ts.IntegerWidth()+1) * (face%9)) + src.left;
	tmp.right = tmp.left + src.Width();
	
	tmp.top = ((ts.IntegerHeight()+1) * (face/9)) + src.top;
	tmp.bottom = tmp.top + src.Height();
		

	if(ShowMatches && !FirstSelect){
		if(tile.value==SelectedTile.value && !tile.locked){
			if(transON){
				Drawer->SetDrawingMode(B_OP_ADD);
				Drawer->SetHighColor(64,64,64,100);
				Drawer->FillRect(dest,B_SOLID_HIGH);
				return;
			}
			else {
				Drawer->DrawBitmap(FullTiles,tmp,dest);
				Drawer->SetDrawingMode(B_OP_ALPHA);
				Drawer->SetHighColor(64,64,64,100);
				Drawer->FillRect(tile.hitZone,B_SOLID_HIGH);
				return;
			}
		}
		else {
			Drawer->DrawBitmap(FullTiles,tmp,dest);
		}
	}
	else {
		Drawer->DrawBitmap(FullTiles,tmp,dest);
	}
}


/*******************************************************
*
*******************************************************/
void MView::Draw(BRect){
	suspend_thread(timer_thread);
	if(full){
		char stim[11];
		sprintf(stim,"%d",elapsedTime);
	
		char score[11];
		sprintf(score,"%d",game_score);

		OffScreen->Lock();		//draw tiled bitmap

		if(!time_draw){		
			for(int b = 0;b < Bounds().Height();b += bg->Bounds().IntegerHeight()){
				for(int a = 0;a < Bounds().Width();a += bg->Bounds().IntegerWidth()){
					Drawer->DrawBitmap(bg,BPoint(a,b));
				}
			}
	      				
			//draw individual tiles
			for(int z = 0; z < LAYERS;z++){
				for(int y = 0;y < HEIGHT;y++){
					for(int x = WIDTH-1; x >= 0;x--){ //inverse draw due to 3D shadow!
						DrawTile(Board[x][y][z]);
					} 
				}
			}
			ShowMatches = false;
		}

		//draw board edge / border
		BRect score_zone;
		score_zone.left=Bounds().left;
		score_zone.bottom=Bounds().bottom;
		score_zone.right=Bounds().right;
		score_zone.top=Bounds().bottom-Yoff+5;
		Drawer->SetDrawingMode(B_OP_COPY);
		Drawer->SetHighColor(32,32,32,100);
		Drawer->FillRect(score_zone,B_SOLID_HIGH);
		
		Drawer->SetDrawingMode(B_OP_OVER);
		Drawer->SetHighColor(255,255,255,0);
		ScoreFont.SetSize(12.0);
		Drawer->SetFont(&ScoreFont,B_FONT_SIZE | B_FONT_FLAGS);
	
		//draw score
		Drawer->MovePenTo(Xoff,Bounds().bottom-5);
		Drawer->DrawString("Score:");
		Drawer->MovePenTo(Xoff+40,Bounds().bottom-5);
		Drawer->DrawString(score);

		//draw timer string
		Drawer->MovePenTo(Bounds().right-140,Bounds().bottom-5);
		Drawer->DrawString("Time Elapsed:");
		Drawer->MovePenTo(Bounds().right-60,Bounds().bottom-5);
		Drawer->DrawString(stim);

		OffScreen->Unlock();
		Drawer->Flush();
		time_draw=false;	
		full = false;
	}	
	Drawer->Flush();
	DrawBitmap(OffScreen);
	Flush();
	resume_thread(timer_thread);
}


/*******************************************************
*
*******************************************************/
bool MView::IsTileLocked(tile_type tile){
	//this checks to see if a specific tile is locked
	bool right_locked = false;
	bool left_locked = false;
	bool top_locked = false;
	int x=tile.x;
	int y=tile.y;
	int z=tile.z;
	   
	if(tile.face != 255){ //this is a valid tile!
		/*	do a locked test for this tile
			here we check left, right and above
			remembering that there may be X,Y,Z,V,U type locking
			as well.
		*/
		if(!x==0){
			if(Board[x-1][y][z].face!=255)left_locked=true;
			if(Mask[x-1][y+1][z]=='Y' &&
				Board[x-1][y+1][z].face!=255)left_locked=true;
			if(Mask[x-1][y-1][z]=='V' &&
				Board[x-1][y-1][z].face!=255)left_locked=true;
		}
		if(x!=15){
			if(Board[x+1][y][z].face!=255)right_locked=true;
			if(Mask[x+1][y-1][z]=='V' &&
				Board[x+1][y-1][z].face!=255)right_locked=true;
			if(Mask[x+1][y+1][z]=='Y' &&
				Board[x+1][y+1][z].face!=255)right_locked=true;
		}
		//now test for locking from the layer above
		
		if(z!=LAYERS-1){
			if(Board[x][y][z+1].face!=255)top_locked=true;
			
			switch (Mask[x][y][z]){
			case 'N':
				if(y<HEIGHT-1){ //this is to trap bad board designs
					if((Mask[x][y+1][z+1]=='Y' || Mask[x][y+1][z+1]=='Z') &&
						Board[x][y+1][z+1].face!=255)top_locked=true;
				}
				
				if(y>0){ //this is to trap bad board designs
					if((Mask[x][y-1][z+1]=='V' || Mask[x][y-1][z+1]=='U') &&
						Board[x][y-1][z+1].face!=255)top_locked=true;
				}
				
				if(y<HEIGHT-1 && x>0){ //this is to trap bad board designs
					if(Mask[x-1][y+1][z+1]=='Z' &&
						Board[x-1][y+1][z+1].face!=255)top_locked=true;
				}
				
				if(y>0 && x>0){ //this is to trap bad board designs
					if(Mask[x-1][y-1][z+1]=='U' &&
						Board[x-1][y-1][z+1].face!=255)top_locked=true;
				}
				
				if(x>0){ //this is to trap bad board designs
					if((Mask[x-1][y][z+1]=='X' || Mask[x-1][y][z+1]=='Z'
						|| Mask[x-1][y][z+1]=='U') &&
						Board[x-1][y][z+1].face!=255)top_locked=true;
				}
				break;
			
			case 'Z':
				if(x<WIDTH-1){ //this is to trap bad board designs
					if((Mask[x+1][y][z+1]=='N'
						|| Mask[x+1][y][z+1]=='Y') &&
						Board[x+1][y][z+1].face!=255)top_locked=true;
				}
	
				if(y>0 && x<WIDTH-1){ //this is to trap bad board designs
					if((Mask[x+1][y-1][z+1]=='N'
						|| Mask[x+1][y-1][z+1]=='V') &&
						Board[x+1][y-1][z+1].face!=255)top_locked=true;
				}

				if(y>0 && x<WIDTH-1){ //this is to trap bad board designs
					if((Mask[x][y-1][z+1]=='N' || Mask[x][y-1][z+1]=='V'
						|| Mask[x][y-1][z+1]=='U' || Mask[x][y-1][z+1]=='X') &&
						Board[x][y-1][z+1].face!=255)top_locked=true;
				}
				break;

			case 'X':
				if(x<WIDTH-1){ //this is to trap bad board designs
					if((Mask[x+1][y][z+1]=='N'
						|| Mask[x+1][y][z+1]=='Y' 
						|| Mask[x+1][y][z+1]=='V') &&
						Board[x+1][y][z+1].face!=255)top_locked=true;
				}
	
				if(y>0){ //this is to trap bad board designs
					if((Mask[x][y-1][z+1]=='U'
						|| Mask[x][y-1][z+1]=='V') &&
						Board[x][y-1][z+1].face!=255)top_locked=true;
				}

				if(y<HEIGHT-1){ //this is to trap bad board designs
					if((Mask[x][y+1][z+1]=='Z'
						|| Mask[x][y+1][z+1]=='Y') &&
						Board[x][y+1][z+1].face!=255)top_locked=true;
				}
				break;

				if(y<HEIGHT-1 && x<WIDTH-1){ //this is to trap bad board designs
					if(Mask[x+1][y+1][z+1]=='Y' &&
						Board[x+1][y+1][z+1].face!=255)top_locked=true;
				}
				break;

				if(y>0 && x<WIDTH-1){ //this is to trap bad board designs
					if(Mask[x+1][y-1][z+1]=='V' &&
						Board[x+1][y-1][z+1].face!=255)top_locked=true;
				}
				break;

			case 'Y':
				if(x>0 && y>0){ //this is to trap bad board designs
					if((Mask[x-1][y-1][z+1]=='U'
						|| Mask[x-1][y-1][z+1]=='X') &&
						Board[x-1][y-1][z+1].face!=255)top_locked=true;
				}
	
				if(x>0){ //this is to trap bad board designs
					if((Mask[x-1][y][z+1]=='Z'
						|| Mask[x-1][y][z+1]=='X') &&
						Board[x-1][y][z+1].face!=255)top_locked=true;
				}

				if(y>0){ //this is to trap bad board designs
					if((Mask[x][y-1][z+1]=='N'
						|| Mask[x][y-1][z+1]=='V'
						|| Mask[x][y-1][z+1]=='X'
						|| Mask[x][y-1][z+1]=='U') &&
						Board[x][y-1][z+1].face!=255)top_locked=true;
				}
				break;

			case 'V':
				if(x>0 && y<HEIGHT-1){ //this is to trap bad board designs
					if((Mask[x-1][y+1][z+1]=='Z'
						|| Mask[x-1][y+1][z+1]=='X') &&
						Board[x-1][y+1][z+1].face!=255)top_locked=true;
				}
	
				if(x>0){ //this is to trap bad board designs
					if((Mask[x-1][y][z+1]=='U'
						|| Mask[x-1][y][z+1]=='X') &&
						Board[x-1][y][z+1].face!=255)top_locked=true;
				}

				if(y<HEIGHT-1){ //this is to trap bad board designs
					if((Mask[x][y+1][z+1]=='N'
						|| Mask[x][y+1][z+1]=='Y'
						|| Mask[x][y+1][z+1]=='X'
						|| Mask[x][y+1][z+1]=='Z') &&
						Board[x][y+1][z+1].face!=255)top_locked=true;
				}
				break;
				
			case 'U':
				if(x<WIDTH-1){ //this is to trap bad board designs
					if((Mask[x+1][y][z+1]=='N'
						|| Mask[x+1][y][z+1]=='V') &&
						Board[x+1][y][z+1].face!=255)top_locked=true;
				}
	
				if(x<WIDTH-1 && y<HEIGHT-1){ //this is to trap bad board designs
					if((Mask[x+1][y+1][z+1]=='N'
						|| Mask[x+1][y+1][z+1]=='Y') &&
						Board[x+1][y+1][z+1].face!=255)top_locked=true;
				}

				if(y<HEIGHT-1){ //this is to trap bad board designs
					if((Mask[x][y+1][z+1]=='N'
						|| Mask[x][y+1][z+1]=='Y'
						|| Mask[x][y+1][z+1]=='X'
						|| Mask[x][y+1][z+1]=='Z') &&
						Board[x][y+1][z+1].face!=255)top_locked=true;
				}
				break;
												
			}//end of switch statement
				
		}//end of hit-tests
		
		
		if((left_locked && right_locked) || top_locked){
			//this is a locked tile - ignore hit
			tile.locked=true;
			Board[x][y][z]=tile;
			return true;
		}
		else {
			tile.locked=false;
			Board[x][y][z]=tile;
			return false;
		}
	}
}


/*******************************************************
*
*******************************************************/
void MView::MouseDown(BPoint where){

//test for right mouse click
	BMessage *msg (Window()->CurrentMessage());
	int32 buttons (0),
			modifiers (0),
			clicks (0);

	msg->FindInt32 ("buttons", &buttons);
	msg->FindInt32 ("clicks",  &clicks);
	msg->FindInt32 ("modifiers", &modifiers);

	if (buttons == B_SECONDARY_MOUSE_BUTTON
	&& (modifiers & B_SHIFT_KEY)   == 0
	&& (modifiers & B_OPTION_KEY)  == 0
	&& (modifiers & B_COMMAND_KEY) == 0
	&& (modifiers & B_CONTROL_KEY) == 0)
	{
		ShowMatches=true;
		full=true;
		Invalidate();   
		return;
	}

	int x = 0;
	int y = 0;
	int z;
	bool found = false;
	bool exit_y = false;
	
	tile_type tile;
   
	//check normal tiles
	for(z = LAYERS-1; z >= 0;z--){
		exit_y=false;
		for(y=0;y<HEIGHT;y++){
			for(x=0;x<WIDTH;x++){
				tile=Board[x][y][z];
				if(tile.hitZone.Contains(where)){
					if(tile.face != 255){ //this is a valid tile!
						found = true;
						/*	do a locked test for this tile
							here we check left, right and above
							remembering that there may be X,Y,Z type locking
							as well.
						*/
												
						if(tile.locked){
							//this is a locked tile - ignore hit
						}
						else{
							if(tile.selected==true){
								tile.selected=false;
							}
							else {
								tile.selected=true;
								Board[tile.x][tile.y][tile.z]=tile;
								/*
									This is our little code to play some sound .. 
									should it realy be here ... I gess .
								*/
								if(soundON){
									play_sound(&singleref,true,false,true);
								}
								if(!FirstSelect){
									if(SelectedTile.value!=tile.value){
										SelectedTile.selected=false;
										Board[SelectedTile.x][SelectedTile.y][SelectedTile.z]=SelectedTile;
										SelectedTile=tile;
									}
									else {
										//place some tile delete sound here maybe?
										//remove tiles from board
										Board[x][y][z]=tile;
										DeleteBlock(tile);
										DeleteBlock(SelectedTile);
										FirstSelect=true;
									}
								} 
								else {
									FirstSelect=false;
									SelectedTile=Board[x][y][z];					
								}
							}
						}
					}
					if(found==true){
						exit_y=true;
						break; //only 1 tile per layer can occupy a hit zone
					}
				}
			}
			if(found==true || exit_y==true) break;
		}
		if(found==true) break;
	}


	if(!found){
		return;
	}
	
	full = true;
	Invalidate();
	
	if(BlocksLeft <= 0){
		start_game=false;
    	(new BAlert(NULL,"You Win, Wow!","Whatever"))->Go();
	}
}

/*******************************************************
*
*******************************************************/
void MView::DeleteBlock(tile_type tile){
	tile.selected=false;
	UndoBoard[BlocksLeft]=tile; //copy tile into undo array
	tile.face=255;
	tile.value=255;
	Board[tile.x][tile.y][tile.z]=tile;
	BlocksLeft--;
	game_score+=5;	
}

/*******************************************************
*
*******************************************************/
void MView::Undo(){
	if(BlocksLeft < BlockCount)
	{
		SelectedTile.selected=false;
		Board[SelectedTile.x][SelectedTile.y][SelectedTile.z]=SelectedTile;
		tile_type tile;
		BlocksLeft++;
		tile=UndoBoard[BlocksLeft];
		Board[tile.x][tile.y][tile.z]=tile;
		BlocksLeft++;
		tile=UndoBoard[BlocksLeft];
		FirstSelect=true;
		Board[tile.x][tile.y][tile.z]=tile;
		SelectedTile = tile;
		game_score-=15;
		start_game=true;
		full=true;
		Invalidate();
	}
}

/*******************************************************
*
*******************************************************/
int32 MView::ReShuffle(){
	/*
	this shuffles the faces on the remaining tiles to allow completion.
	all tiles are grabbed from the existing board tiles and are stored in the UndoBoard
	next they are re-applied according to the used array.
	*/
	game_score-=100;

	Loader.Lock();
	bool used[BlocksLeft];
	for(int c=0;c<BlocksLeft;c++)used[c]=false; //clear usage states
	
	//store existing tiles
	int x=0,y=0,z=0;
	int c=0;
	for(z = 0; z < LAYERS;z++){
		for(y = 0; y < HEIGHT;y++){
			for(x = 0; x < WIDTH;x++){
				if(Board[x][y][z].face!=255){
					UndoBoard[c]=Board[x][y][z];
					c++;
				}
			}
		}
	}
	seed=system_time();
	srand(seed);//initialise random sequence with user/system seed
	int dt; //this will hold the tile randomly drawn from the deck

	for(z = 0; z < LAYERS;z++){
		for(y = 0; y < HEIGHT;y++){
			for(x = 0; x < WIDTH;x++){

				if(Board[x][y][z].face != 255){
				
					dt=rand() % BlocksLeft;
					while(used[dt]==true)dt=rand() % BlocksLeft;
					
					//set all tile attributes
					Board[x][y][z].face = UndoBoard[dt].face;
					Board[x][y][z].value = UndoBoard[dt].value;
					Board[x][y][z].x = x;
					Board[x][y][z].y = y;
					Board[x][y][z].z = z;
					Board[x][y][z].selected=false;
					used[dt]=true;
				}

			}//end of x loop
		}//end of y loop
	}//end of z loop

	//set misc variables
	FirstSelect = true;

	//force board redraw
	Window()->Lock();
	full = true;
	Invalidate();
	Window()->Unlock();


	isLoading = false;
	Loader.Unlock();

	return B_OK;
}


/*******************************************************
*
*******************************************************/
int32 MView::LoopTimer(){
   //return B_OK;
	while(!killtimer){
	    if(!start_game){
	       //snooze(100000000);
	       bigtime_t duration = 1000000;
			bigtime_t max_slice = 50000;
			uint32 slices = duration / max_slice;
			for(uint32 i = 0; (i < slices) && !killtimer; i++){
			   snooze(max_slice);
			}
	       continue;
	    }
		//while(start_game && !killtimer){ //do this as long as game is active!
		    // this is a bad way becuase it eats CPU big time
		    //bigtime_t starttime;
			//starttime=system_time();
	        //while((starttime+1000000)>system_time() && !killtimer);
			
			// this is a bad way because it doesn't allow the thread to die
			// off fast enough
			//snooze(100000000);
			
			// This way is much better as it sleeps for the 
			// ~same time however it only uses up a small
			// amout of CPU and will repidly respond on exit
			// of thread
			// effect, sleep for 1000000 usecs (microseconds)
			
			bigtime_t duration = 1000000;
			bigtime_t max_slice = 50000;
			uint32 slices = duration / max_slice;
			for(uint32 i = 0; (i < slices) && !killtimer; i++){
			   snooze(max_slice);
			}
			
			
			
			elapsedTime++;
			if(!full && !killtimer){
				time_draw=true;
				if(Window()->LockLooper()){
   				   full=true;
	   			   Invalidate();
		   		   Window()->UnlockLooper();
		   		}
			}
		//}
	}
	
	
	/*while(!killtimer){
	   if(start_game){ // game is running
	      elapsedTime++;
	      LockLooper();
	      time_draw=true;
	      full=true;
	      Invalidate();
	      UnlockLooper();
	   }
	   snooze(100000000); // sleep for a sec
	}*/
	
	return B_OK;
}


/*******************************************************
*
*******************************************************/
int32 MView::FindHint(){
	//this de-selects if selected - finds new tile and sets matches to true
	if(!FirstSelect){
		Board[SelectedTile.x][SelectedTile.y][SelectedTile.z].selected=false;
		FirstSelect=true;
	}
	//build hint matrix
	for(int z = 0; z < LAYERS;z++){
		for(int y = 0;y < HEIGHT;y++){
			for(int x = 0; x <WIDTH;x++){
				if(!Board[x][y][z].locked && Board[x][y][z].face!=255){
					for(int hz = 0; hz < LAYERS;hz++){
						for(int hy = 0;hy < HEIGHT;hy++){
							for(int hx = 0; hx <WIDTH;hx++){
								if(!Board[hx][hy][hz].locked &&
									Board[hx][hy][hz].face!=255 &&
									!(x==hx && y==hy && z==hz) &&
									Board[x][y][z].value==Board[hx][hy][hz].value) {
									Board[x][y][z].selected=true;
									SelectedTile=Board[x][y][z];
									FirstSelect=false;
									ShowMatches = true;
									return B_OK;
								}
							}
						}
					}
				}
			} 
		}
	}
	if(BlocksLeft==2){
		(new BAlert(NULL,"Hmm.. Stacked tiles!  Think its time to Undo and then re-shuffle!","Darn!"))->Go();	
	}else {
		(new BAlert(NULL,"Hmm.. No matches found!  Think its time to re-shuffle!","Darn!"))->Go();	
	}
	return B_ERROR; 
}

/*******************************************************
*
*******************************************************/
void MView::MessageReceived(BMessage *msg){
   switch(msg->what){
   case TOGGLE_SOUND:
      soundON = !soundON;
      break;
   case TOGGLE_TRANS:
      transON = !transON;
      break;      
   case TOWER:
      ptr = TowerMask;
      ptr_Set = Tower_Set;
      ptr_Values = Basic_Values;
      LoadBoard();
      resume_thread(spawn_thread(GameLoader, "Loading Game", B_NORMAL_PRIORITY, this));
      break;
   case CASTLE:
      ptr = CastleMask;
      ptr_Set = Castle_Set;
      ptr_Values = Classic_Values;
      LoadBoard();
      resume_thread(spawn_thread(GameLoader, "Loading Game", B_NORMAL_PRIORITY, this));
      break;
   case PYRAMIDE:
      ptr = PyramideMask;
      ptr_Set = Pyramide_Set;
      ptr_Values = Basic_Values;
      LoadBoard();
      resume_thread(spawn_thread(GameLoader, "Loading Game", B_NORMAL_PRIORITY, this));
      break;
   case TRIANGLE:
      ptr = TriangleMask;
      ptr_Set = Triangle_Set;
      ptr_Values = Basic_Values;
      LoadBoard();
      resume_thread(spawn_thread(GameLoader, "Loading Game", B_NORMAL_PRIORITY, this));
      break;
   case CLASSIC:
      ptr = ClassicMask;
      ptr_Set = Classic_Set;
      ptr_Values = Classic_Values;
      LoadBoard();
      resume_thread(spawn_thread(GameLoader, "Loading Game", B_NORMAL_PRIORITY, this));
      break;
   case DRAGON_BRIDGE:
      ptr = DragonBridgeMask;
      ptr_Set = Dragon_Set;
      ptr_Values = Classic_Values;
      LoadBoard();
      resume_thread(spawn_thread(GameLoader, "Loading Game", B_NORMAL_PRIORITY, this));
      break;
   case RICEBOWL:
      ptr = RiceBowlMask;
      ptr_Set = RiceBowl_Set;
      ptr_Values = Classic_Values;
      LoadBoard();
      resume_thread(spawn_thread(GameLoader, "Loading Game", B_NORMAL_PRIORITY, this));
      break;
   case UNDO:
	  Undo();
      break;
   case HIDELOCKED:
      HideLockedTiles=!HideLockedTiles;
	  full=true;
	  Invalidate();      
      break;
   case SHOW_MATCH:
   	  ShowMatches=true;
	  full=true;
	  Invalidate();      
   	  break;
   case HINT:
   	  if(FindHint()==B_OK){
		  full=true;
		  Invalidate();      
	  }
   	  break;
   case RESHUFFLE:
   	  ReShuffle();
   	  break;
   case NEW_GAME:
      //if(!isLoading){
      //   isLoading = true;
         if(!Loader.IsLocked()){
            resume_thread(spawn_thread(GameLoader, "Loading Game", B_NORMAL_PRIORITY, this));
         }
      //}
      break;
   default:
      BView::MessageReceived(msg);
   }
}












