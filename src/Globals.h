
#ifndef GLOBALS_H
#define GLOBALS_H


#define LAYERS 5
#define HEIGHT 8
#define WIDTH 16
#define SPACE 5
#define MAX_DECK_SIZE 640


#define NEW_GAME    'newg'
#define HELP_ME     'hlpm'
#define SHOW_MATCH  'matc'
#define DEMO        'demo'
#define BACK_GROUND 'back'
#define TILE_SET    'tile'
#define HELP        'help'
#define TOGGLE_SOUND 'tgsd'
#define TOGGLE_TRANS 'tgtr'
#define DO_BG       'dobg'
#define DO_TS       'dots'
#define UNDO        'undo'
#define RESHUFFLE	'shuf'
#define HIDELOCKED	'hdlk'
#define HINT		'hint'

#define TOWER       'towr'
#define PYRAMIDE    'pyrd'
#define TRIANGLE    'trgl'
#define CLASSIC     'clsc'
#define CASTLE		'cstl'
#define DRAGON_BRIDGE 'drbr'
#define RICEBOWL	'rice'

//tile type definition
//from this data a tile can be used in undo, find etc.
struct tile_type {
	unsigned char face;
	unsigned char value;
	int	x;
	int y;
	int z;
	bool selected;
	bool locked;
	BRect hitZone;
};


#endif
