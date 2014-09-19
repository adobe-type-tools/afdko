
#if MACINTOSH
// Obtain Folder/Volume FSSpec from Standard Dialog Box
// Taken from Macintosh Developers' CDs

#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>


// Additional Toolbox Includes
#include <Types.h>
#include <Aliases.h>
#include <Dialogs.h>
#include <StandardFile.h>
#include <Files.h>
#include <Folders.h>
#include <Gestalt.h>
#include <Script.h>
#include <Traps.h>
#include <Errors.h>
#include <Strings.h>
#include <TextUtils.h>
#include <Resources.h>
#include <OSUtils.h>

// Generic Boolean Constants

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

// Generic Zero Constants

#ifndef NULL
#define NULL	0L
#endif

#ifndef nil
#define nil		0L
#endif

// Generic Data Constants

#define BLOCK_SIZE		512
#define MAX_STRING		256


typedef struct {
	StandardFileReply *replyPtr;
	FSSpec oldSelection;
} SFData, *SFDataPtr;


#define GETFOLDER		8006

#define	kDeskStrRsrc			(GETFOLDER+10)
#define	kSFDialog				(GETFOLDER+0)
#define	kSelectStrRsrc			(GETFOLDER+0)

#define	kCanSelectDesktop		TRUE

#define	kDefaultDeskString		"\pDefault"
#define kDefaultSelectString	"\pDefault Select"

#define	kSelectItem				10
#define	kSelectKey				's'


// Local Prototypes

static void InitStuff (void);
static pascal short MyDlgHook (short item, DialogPtr theDlg, Ptr userData);
static pascal Boolean MyModalFilter (DialogPtr theDlg, EventRecord *ev, short *itemHit, Ptr myData);
static void HitButton (DialogPtr theDlg, short item);
static pascal Boolean FilterAllFiles (CInfoPBPtr pb, Ptr myDataPtr);
static pascal Boolean AllowFoldersFiles (CInfoPBPtr pb, Ptr myDataPtr);
static void SetSelectButtonName (StringPtr selName, Boolean hilited, DialogPtr theDlg);
static Boolean SameFile (FSSpec *file1, FSSpec *file2);
static OSErr GetDeskFolderSpec (FSSpec *fileSpec, short vRefNum);
static Boolean ShouldHiliteSelect (FSSpec *fileSpec);
static OSErr MakeCanonFSSpec (FSSpec *fileSpec);
// Local Variables

Boolean lHasFindFolder;
FSSpec lDeskFolderSpec;
Str255 lSelectString;
Str255 lDesktopFName;

// Global Procedures
Boolean MacGetFileFolder (Boolean onlyFolders, FSSpec *fileSpec);

Boolean MacGetFileFolder (Boolean onlyFolders, FSSpec *fileSpec)
// returns TRUE if good, FALSE otherwise
{
	Point where = {-1,-1};
	StandardFileReply sfReply;
	SFData sfUserData;
	OSErr err;
	Boolean targetIsFolder, wasAliased;
	Boolean retval = false;
	
	InitStuff ();
	
	/* initialize user data area */
	sfUserData.replyPtr = &sfReply;
	sfUserData.oldSelection.vRefNum = -9999;	/* init to ridiculous value */
	
	CustomGetFile ((onlyFolders) ? (NewFileFilterYDProc(FilterAllFiles))
								 : (NewFileFilterYDProc(AllowFoldersFiles)),
				   -1, nil, &sfReply, kSFDialog, where,
	               NewDlgHookYDProc(MyDlgHook), 
	               NewModalFilterYDProc(MyModalFilter), 
	               nil, nil,
	               (Ptr)(&sfUserData));


	if (sfReply.sfGood) {
		err = ResolveAliasFile (&sfReply.sfFile, true, &targetIsFolder, &wasAliased);
		if (err!=noErr)
		{
			retval = false;
			goto done;
		}
	}
	
	err = FSMakeFSSpec (sfReply.sfFile.vRefNum, sfReply.sfFile.parID, sfReply.sfFile.name, fileSpec);
	if (err!=noErr)
		retval = false;
	else
		retval = sfReply.sfGood;
done:
	return retval;	
}

// Local Procedures

static void InitStuff ()
{
	OSErr err;
	long gResponse;
	Handle strHndl;
	
	lHasFindFolder = false;

	err = Gestalt (gestaltFindFolderAttr, &gResponse);
	if (err == noErr) lHasFindFolder = (gResponse && (1 << gestaltFindFolderPresent));

	// get string for select button
	strHndl = GetResource ('STR ', kSelectStrRsrc);
	if (ResError () != noErr || strHndl == nil || strHndl == nil) {
		// use default
		BlockMove (kDefaultSelectString, lSelectString, kDefaultSelectString[0]+1);
	} else {
		BlockMove (*strHndl, &lSelectString, (long) ((unsigned char *) (*strHndl)[0]+1));
		ReleaseResource (strHndl);
	}

	// get string for desktop name
	strHndl = GetResource ('STR ', kDeskStrRsrc);
	if (ResError () != noErr || strHndl == nil || strHndl == nil) {
		// use default
		BlockMove (kDefaultDeskString, lDesktopFName, kDefaultSelectString[0]+1);
	} else {
		BlockMove (*strHndl, &lDesktopFName, (long) ((unsigned char *) (*strHndl)[0]+1));
		ReleaseResource (strHndl);
	}
}

/*	this dialog hook checks the contents of the additional edit fields
	when the user selects a file.  The focus of the dialog is changed if one
	of the fields is out of range.
*/

static pascal short MyDlgHook (short item, DialogPtr theDlg, Ptr userData)
{
	SFDataPtr sfUserData;
	FSSpec curSpec;
	OSType refCon;
	
	refCon = GetWRefCon (theDlg);
	if (refCon!=sfMainDialogRefCon)
		return item;
		
	sfUserData = (SFDataPtr) userData;
	
	if (item==sfHookFirstCall || item==sfHookLastCall)
		return item;
	
	if (item==sfItemVolumeUser) {
		sfUserData->replyPtr->sfFile.name[0] = '\0';
		sfUserData->replyPtr->sfFile.parID = 2;
		sfUserData->replyPtr->sfIsFolder = false;
		sfUserData->replyPtr->sfIsVolume = false;
		sfUserData->replyPtr->sfFlags = 0;
		item = sfHookChangeSelection;
	}
		
	if (!SameFile (&sfUserData->replyPtr->sfFile, &sfUserData->oldSelection)) {
		BlockMove (&sfUserData->replyPtr->sfFile, &curSpec, sizeof (FSSpec));
		MakeCanonFSSpec (&curSpec);
		
		if (curSpec.vRefNum!=sfUserData->oldSelection.vRefNum)
			GetDeskFolderSpec (&lDeskFolderSpec, curSpec.vRefNum);	
		SetSelectButtonName (curSpec.name, ShouldHiliteSelect (&curSpec), theDlg);
		
		BlockMove (&sfUserData->replyPtr->sfFile, &sfUserData->oldSelection, sizeof (FSSpec));
	}
	
	if (item==kSelectItem)
		item = sfItemOpenButton;
		
	return item;
}

static pascal Boolean MyModalFilter (DialogPtr theDlg, EventRecord *ev, short *itemHit, Ptr myData)
{
	Boolean evHandled;
	char keyPressed;
	OSType refCon;
	
	refCon = GetWRefCon (theDlg);
	if (refCon!=sfMainDialogRefCon)
		return false;
		
	evHandled = false;
	
	switch (ev->what) {
		case keyDown:
		case autoKey:
			keyPressed = ev->message & charCodeMask;
			if ((ev->modifiers & cmdKey) != 0) {
				switch (keyPressed) {
					case kSelectKey:
						HitButton (theDlg, kSelectItem);
						*itemHit = kSelectItem;
						evHandled = true;
						break;
				}
			}
			break;
	}
	
	return evHandled;
}

static void HitButton (DialogPtr theDlg, short item)
{
	short iType;
	ControlHandle iHndl;
	Rect iRect;
	unsigned long fTicks;
	
	GetDialogItem (theDlg, item, &iType, (Handle *) &iHndl, &iRect);
	HiliteControl (iHndl, kControlButtonPart); //inButton);
	Delay (8, &fTicks);
	HiliteControl (iHndl, 0);
}

static pascal Boolean FilterAllFiles (CInfoPBPtr pb, Ptr myDataPtr)
{
	if (!pb) return true;
	if (pb->hFileInfo.ioFlAttrib & (1 << 4))	/* file is a directory */
		return false; /* notfiltered */

	return true; /* isfiltered */
}
static pascal Boolean AllowFoldersFiles (CInfoPBPtr pb, Ptr myDataPtr)
{
  return false; /* none filtered */
}


static void SetSelectButtonName (StringPtr selName, Boolean hilited, DialogPtr theDlg)
{
	short iType;
	Handle iHndl;
	Rect iRect;
	Str255 storeName, tempLenStr, tempSelName;
	short btnWidth;
	
	BlockMove (selName, tempSelName, selName[0]+1);
	GetDialogItem (theDlg, kSelectItem, &iType, &iHndl, &iRect);
	
	// truncate select name to fit in button
	
	btnWidth = iRect.right - iRect.left;
	BlockMove (lSelectString, tempLenStr, lSelectString[0]+1);

	btnWidth -= StringWidth (tempLenStr);
	TruncString (btnWidth, tempSelName, smTruncMiddle);

	p2cstr (storeName);
	p2cstr (tempSelName);
	p2cstr (lSelectString);

	sprintf ((char *) storeName, (char *) lSelectString, (char *) tempSelName);

	c2pstr ((char *) storeName);
	c2pstr ((char *) tempSelName);
	c2pstr ((char *) lSelectString);
	
	SetControlTitle ((ControlHandle) iHndl, storeName);
	
	SetDialogItem (theDlg, kSelectItem, iType, iHndl, &iRect);

	if (hilited) HiliteControl ((ControlHandle) iHndl, 0);
	else HiliteControl ((ControlHandle) iHndl, 255);		
}

static Boolean SameFile (FSSpec *file1, FSSpec *file2)
{
	if (file1->vRefNum != file2->vRefNum)
		return false;
	if (file1->parID != file2->parID)
		return false;
	if (!EqualString (file1->name, file2->name, false, true))
		return false;
	
	return true;
}

static OSErr GetDeskFolderSpec (FSSpec *fileSpec, short vRefNum)
{
	OSErr err;
	
	if (!lHasFindFolder) {
		fileSpec->vRefNum = -9999;
		return -1;
	}
	
	fileSpec->name[0] = '\0';
	err = FindFolder (vRefNum, kDesktopFolderType, kDontCreateFolder,
	                  &fileSpec->vRefNum, &fileSpec->parID);
	if (err != noErr)
		return err;
	
	return MakeCanonFSSpec (fileSpec);
}

static Boolean ShouldHiliteSelect (FSSpec *fileSpec)
{
	if (SameFile (fileSpec, &lDeskFolderSpec)) {
		BlockMove (lDesktopFName, fileSpec->name, lDesktopFName[0]+1);
		return kCanSelectDesktop;
	}
	else return true;
}

static OSErr MakeCanonFSSpec (FSSpec *fileSpec)
{
	CInfoPBRec dirPtr;
	DirInfo *infoPB = (DirInfo *) &dirPtr;
	OSErr err;

	if (fileSpec->name[0] != '\0') return;
		
	infoPB->ioNamePtr = fileSpec->name;
	infoPB->ioVRefNum = fileSpec->vRefNum;
	infoPB->ioDrDirID = fileSpec->parID;
	infoPB->ioFDirIndex = -1;
	err = PBGetCatInfo (&dirPtr, false);
	fileSpec->parID = infoPB->ioDrParID;
	
	return err;
}

#endif /* MACINTOSH */
