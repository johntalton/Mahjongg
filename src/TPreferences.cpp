/*******************************************************
*   ImageShow©
*
*   This is hopefully going to be a realy nice image
*   viewer. Not like there are a 100 of them out there
*   or anything.
*
*   @author  YNOP (ynop@acm.org)
*   @version beta
*   @date    Sept 18 1999
*******************************************************/
/*******************************************************
*   Class for saving and loading preference information
*   via BMessages.
*
*   Origanly by Eric Shepherd
*******************************************************/
#include <Message.h>
#include <Messenger.h>
#include <File.h>
#include <FindDirectory.h>
#include <Alert.h>

#include "TPreferences.h"

/*******************************************************
*   Open the settings file and read the data in.
********************************************************/
TPreferences::TPreferences(char *filename) : BMessage('pref'){
	BFile file;
	
	status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK) {
		return;
	}
	path.Append(filename);
	status = file.SetTo(path.Path(), B_READ_ONLY);
	if (status == B_OK) {
		status = Unflatten(&file);	
	}
}


/*******************************************************
*   And finaly write them to disk
*******************************************************/
TPreferences::~TPreferences() {
   BFile file;
	
	if (file.SetTo(path.Path(), B_WRITE_ONLY | B_CREATE_FILE) == B_OK) {
		Flatten(&file);
	}
}


/*******************************************************
*   Synch preferences to disk
*******************************************************/
status_t TPreferences::Sync() {
	BFile file;
	
	if (file.SetTo(path.Path(), B_WRITE_ONLY | B_CREATE_FILE) == B_OK) {
		return Flatten(&file);
	}
}


/*******************************************************
*   
*******************************************************/
status_t TPreferences::SetBool(const char *name, bool b) {
	if (HasBool(name)) {
		return ReplaceBool(name, 0, b);
	}
	return AddBool(name, b);
}


/*******************************************************
*   
*******************************************************/
status_t TPreferences::SetInt8(const char *name, int8 i) {
	if (HasInt8(name)) {
		return ReplaceInt8(name, 0, i);
	}
	return AddInt8(name, i);
}

/*******************************************************
*   
*******************************************************/
status_t TPreferences::SetInt16(const char *name, int16 i) {
	if (HasInt16(name)) {
		return ReplaceInt16(name, 0, i);
	}
	return AddInt16(name, i);
}

/*******************************************************
*   
*******************************************************/
status_t TPreferences::SetInt32(const char *name, int32 i) {
	if (HasInt32(name)) {
		return ReplaceInt32(name, 0, i);
	}
	return AddInt32(name, i);
}

/*******************************************************
*   
*******************************************************/
status_t TPreferences::SetInt64(const char *name, int64 i) {
	if (HasInt64(name)) {
		return ReplaceInt64(name, 0, i);
	}
	return AddInt64(name, i);
}

/*******************************************************
*   
*******************************************************/
status_t TPreferences::SetFloat(const char *name, float f) {
	if (HasFloat(name)) {
		return ReplaceFloat(name, 0, f);
	}
	return AddFloat(name, f);
}

/*******************************************************
*   
*******************************************************/
status_t TPreferences::SetDouble(const char *name, double f) {
	if (HasDouble(name)) {
		return ReplaceDouble(name, 0, f);
	}
	return AddDouble(name, f);
}

/*******************************************************
*   
*******************************************************/
status_t TPreferences::SetString(const char *name, const char *s) {
	if (HasString(name)) {
		return ReplaceString(name, 0, s);
	}
	return AddString(name, s);
}

/*******************************************************
*   
*******************************************************/
status_t TPreferences::SetPoint(const char *name, BPoint p) {
	if (HasPoint(name)) {
		return ReplacePoint(name, 0, p);
	}
	return AddPoint(name, p);
}

/*******************************************************
*   
*******************************************************/
status_t TPreferences::SetRect(const char *name, BRect r) {
	if (HasRect(name)) {
		return ReplaceRect(name, 0, r);
	}
	return AddRect(name, r);
}

/*******************************************************
*   
*******************************************************/
status_t TPreferences::SetMessage(const char *name, const BMessage *message) {
	if (HasMessage(name)) {
		return ReplaceMessage(name, 0, message);
	}
	return AddMessage(name, message);
}

/*******************************************************
*   
*******************************************************/
status_t TPreferences::SetFlat(const char *name, const BFlattenable *obj) {
	if (HasFlat(name, obj)) {
		return ReplaceFlat(name, 0, (BFlattenable *) obj);
	}
	return AddFlat(name, (BFlattenable *) obj);
}
