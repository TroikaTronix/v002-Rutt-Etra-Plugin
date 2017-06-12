// ===========================================================================
//	Isadora Demo Plugin			(c)2017 Mark F. Coniglio. All rights reserved.
// ===========================================================================
//
//	IMPORTANT: This source code ("the software") is supplied to you in
//	consideration of your agreement to the following terms. If you do not
//	agree to the terms, do not install, use, modify or redistribute the
//	software.
//
//	Mark Coniglio (dba TroikaTronix) grants you a personal, non exclusive
//	license to use, reproduce, modify this software with and to redistribute it,
//	with or without modifications, in source and/or binary form. Except as
//	expressly stated in this license, no other rights are granted, express
//	or implied, to you by TroikaTronix.
//
//	This software is provided on an "AS IS" basis. TroikaTronix makes no
//	warranties, express or implied, including without limitation the implied
//	warranties of non-infringement, merchantability, and fitness for a 
//	particular purpurse, regarding this software or its use and operation
//	alone or in combination with your products.
//
//	In no event shall TroikaTronix be liable for any special, indirect, incidental,
//	or consequential damages arising in any way out of the use, reproduction,
//	modification and/or distribution of this software.
//
// ===========================================================================
//
// CUSTOMIZING THIS SOURCE CODE
// To customize this file, search for the text ###. All of the places where
// you will need to customize the file are marked with this pattern of 
// characters.
//
// ABOUT IMAGE BUFFER MAPS:
//
// The ImageBufferMap structure, and its accompanying functions,
// exists as a convenience to those writing video processing plugins.
//
// Basically, an image buffer contains an arbitrary number of input and
// output buffers (in the form of ImageBuffers). The ImageBufferMap code
// will automatically create intermediary buffers if needed, so that the
// size and depth of the source image buffers sent to your callback are
// the same for all buffers.
// 
// Typically, the ImageBufferMap is created in your CreateActor function,
// and dispose in the DiposeActor function.

// ---------------------------------------------------------------------------------
// INCLUDES
// ---------------------------------------------------------------------------------

#include "IsadoraTypes.h"
#include "IsadoraCallbacks.h"
#include "ImageBufferUtil.h"
#include "PluginDrawUtil.h"
#include "MultiVideoInOutUtil.h"
#include "CGLShader.h"
#include "v002RuttEtraPlugin.h"
#include "COpenGLUtil.h"

// STANDARD INCLUDES
#include <string.h>
#include <stdio.h>

// ---------------------------------------------------------------------------------
// MacOS Specific
// ---------------------------------------------------------------------------------
#if TARGET_OS_MAC
#define EXPORT_
#endif

// ---------------------------------------------------------------------------------
// Win32  Specific
// ---------------------------------------------------------------------------------
#if TARGET_OS_WIN32

	#include <windows.h>
	
	#define EXPORT_ __declspec(dllexport)
	
	#if FALSE

	#ifdef __cplusplus
	extern "C" {
	#endif

	BOOL WINAPI DllMain ( HINSTANCE hInst, DWORD wDataSeg, LPVOID lpvReserved );

	#ifdef __cplusplus
	}
	#endif

	BOOL WINAPI DllMain (
		HINSTANCE	/* hInst */,
		DWORD		wDataSeg,
		LPVOID		/* lpvReserved */)
	{
	switch(wDataSeg) {
	
	case DLL_PROCESS_ATTACH:
		return 1;
		break;
	case DLL_PROCESS_DETACH:
		break;
		
	default:
		return 1;
		break;
	}
	return 0;
	}

	#endif

#endif

// ---------------------------------------------------------------------------------
//	Exported Function Definitions
// ---------------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

EXPORT_ void GetActorInfo(void* inParam, ActorInfo* outActorParams);

#ifdef __cplusplus
}
#endif

// ---------------------------------------------------------------------------------
//	LOCAL DEFINES
// ---------------------------------------------------------------------------------

#define COMPILE_CORE_VIDEO_THIS_PLUGIN	(FALSE && COMPILE_CORE_VIDEO)

// ---------------------------------------------------------------------------------
//	FORWARD DECLARTIONS
// ---------------------------------------------------------------------------------
static void
ReceiveMessage(
	IsadoraParameters*	ip,
	MessageMask			inMessageMask,
	PortIndex			inPortIndex1,
	const MsgData*		inData,
	UInt32				inLen,
	long				inRefCon);

// ---------------------------------------------------------------------------------
// GLOBAL VARIABLES
// ---------------------------------------------------------------------------------
// ### Declare global variables, common to all instantiations of this plugin here

// Example: static int gMyGlobalVariable = 5;

// ---------------------------------------------------------------------------------
// PluginInfo struct
// ---------------------------------------------------------------------------------
// ### This structure neeeds to contain all variables used by your plugin. Memory for
// this struct is allocated during the CreateActor function, and disposed during
// the DisposeActor function, and is private to each copy of the plugin.
//
// If your plugin needs global data, declare them as static variables within this
// file. Any static variable will be global to all instantiations of the plugin.

typedef struct {

	ActorInfo*				mActorInfoPtr;		// our ActorInfo Pointer - set during create actor function
	MessageReceiverRef		mMessageReceiver;	// pointer to our message receiver reference
	Boolean					mNeedsDraw;			// set to true when the video output needs to be drawn
	
	ImageBufferMap			mImageBufferMap;	// CPU
	CMultiVideoInOut*		mv;					// GPU
	
	v002RuttEtraPlugIn*		mRuttEtra;
	Boolean					mBypass;
	
} PluginInfo;

// A handy macro for casting the mActorDataPtr to PluginInfo*
#if __cplusplus
#define	GetPluginInfo_(actorDataPtr)		static_cast<PluginInfo*>((actorDataPtr)->mActorDataPtr);
#else
#define	GetPluginInfo_(actorDataPtr)		(PluginInfo*)((actorDataPtr)->mActorDataPtr);
#endif

// ---------------------------------------------------------------------------------
//	Constants
// ---------------------------------------------------------------------------------
//	Defines various constants used throughout the plugin.

// ### GROUP ID
// Define the group under which this plugin will be displayed in the Isadora interface.
// These are defined under "Actor Types" in IsadoraTypes.h

static const OSType	kActorClass 	= kGroupVideo;

// ### PLUGIN IN
// Define the plugin's unique four character identifier. Contact TroikaTronix to
// obtain a unique four character code if you want to ensure that someone else
// has not developed a plugin with the same code. Note that TroikaTronix reserves
// all plugin codes that begin with an underline, an at-sign, and a pound sign
// (e.g., '_', '@', and '#'.)

static const OSType	kActorID		= FOUR_CHAR_CODE('_reP');

// ### ACTOR NAME
// The name of the actor. This is the name that will be shown in the User Interface.

static const char* kActorName		= "v002 Rutt Etra 3.0 by vade";

// ### PROPERTY DEFINITION STRING
// The property string. This string determines the inputs and outputs for your plugin.
// See the IsadoraCallbacks.h under the heading "PROPERTY DEFINITION STRING" for the
// meaning ofthese codes. (The IsadoraCallbacks.h header can be seen by opening up
// the IzzySDK Framework while in the Files view.)
//
// IMPORTANT: You cannot use spaces in the property name. Instead, use underscores (_)
// where you want to have a space.
//
// Note that each line ends with a carriage return (\r), and that only the last line of
// the bunch ends with a semicolon. This means that what you see below is one long
// null-terminated c-string, with the individual lines separated by carriage returns.

static const char* sDisplaceMode[2] = {
	"luminosity",
	"raw rgb to xyz"
};

static const char* sDrawModes[5] = {
	"scanlines",
	"points",
	"square mesh",
	"triangle mesh",
	"image plane"
};

static const char* sTexCoordMode[4] = {
	"model",
	"object linear",
	"eye linear",
	"sphere map"
};

static const char* sDepthModes[3] = {
	"none",
	"read/write",
	"read only"
};

static const char* sBlendModes[3] = {
	"additive",
	"transparent",
	"opaque"
};

static const char* sPropertyDefinitionString =

// INPUT PROPERTY DEFINITIONS

//	TYPE 	PROPERTY NAME	ID		DATATYPE	DISPLAY FMT			MIN		MAX		INIT VALUE

	"INPROP video					vin		data		texture				*		*		0\r"
	"INPROP displace				disp	data		texture				*		*		0\r"
	"INPROP point_sprite			ptsp	data		texture				*		*		0\r"

	"INPROP horz_resolution			sizh	int			number				10		8192	120\r"
	"INPROP vert_resolution			sizv	int			number				10		8192	68\r"

	"INPROP	draw_mode				dwmd	int			propdef,number		0		4		0\r"
	"INPROP	displace_mode			dsmd	int			propdef,number		0		1		0\r"
	"INPROP	blend_mode				blmd	int			propdef,number		0		2		0\r"
	"INPROP	depth_mode				dpmd	int			propdef,number		0		2		0\r"

	"INPROP	modulation_color		colr	int			acolor,hex			*		*		4294967295\r"

	// requires color spaces... we may not be able to do this
	"INPROP	color_correct			clcr	int			onoff				0		1		0\r"

	"INPROP	texture_coord			txcd	int			propdef,number		0		3		0\r"

	"INPROP	z_extrude				zext	float		number				*		*		10\r"
	"INPROP	wire_frame_size			wfsz	float		number				0		128		1\r"
	"INPROP	antialiasing			aali	int			onoff				0		1		0\r"

	"INPROP	point_attenuation		apen	int			onoff				0		1		0\r"
	"INPROP	point_constant_atten	apcs	float		number				0		100		0\r"
	"INPROP	point_linear_atten		apln	float		number				0		100		0\r"
	"INPROP	point_quadractic_atten	apqd	float		number				0		100		0\r"

	"INPROP	calc_normals			cnen	int			onoff				0		1		0\r"
	// 0.5, 1.0, 5.0... but we'll try a float
	"INPROP	normal_smooth			cncs	float		number				0		5.0		0\r"
	"INPROP	hq_normals				cnhq	int			onoff				0		1		0\r"

	"INPROP rotation_x				rotx	float		number				*		*		0\r"
	"INPROP rotation_y				roty	float		number				*		*		0\r"
	"INPROP rotation_z				rotz	float		number				*		*		0\r"

	"INPROP translate_x				trnx	float		number				*		*		0\r"
	"INPROP translate_y				trny	float		number				*		*		0\r"
	"INPROP translate_z				trnz	float		number				*		*		0\r"

	"INPROP scale_x					sclx	float		number				*		*		1\r"
	"INPROP scale_y					scly	float		number				*		*		1\r"
	"INPROP scale_z					sclz	float		number				*		*		1\r"

	"INPROP	enable_clipping_plane	cpen	int			onoff				0		1		0\r"
	"INPROP min_clipping_plane		cpmn	float		number				0		100		0\r"
	"INPROP max_clipping_plane		cpmx	float		number				0		100		100\r"

	"INPROP	bypass					byps	int			onoff				0		1		0\r"

// OUTPUT PROPERTY DEFINITIONS
//	TYPE 	 PROPERTY NAME	ID		DATATYPE	DISPLAY FMT			MIN		MAX		INIT VALUE
	"OUTPROP video_out				vout	data		texture				*		*		0\r";

// ### Property Index Constants
// Properties are referenced by a one-based index. The first input property
// index must have a value of 1, the second a value of 2, etc. Similarly,
// the first output property must have a value of 1, the second a value of 2,
// etc.
//
// IMPORTANT: Note the "= 1" after kOutputVideo which restarts the index
// count at 1!
//
// You need one constant for each input and output property defined in the
// property definition string above.

enum
{
	// ** IMPORTANT ** first input property index must start at 1!
	
	kInputVideoIn = 1,	// first input property index starts at 1
	kInputVideoLuma,
	kInputVideoPointSprite,
	
	kInputSizeH,
	kInputSizeV,

	kInputDrawMode,
	kInputDisplaceMode,
	kInputBlendMode,
	kInputDepthMode,
	
	kInputColor,
	kInputColorCorrect,
	
	kInputTexCoorMode,
	
	kInputZExtrude,
	kInputWireFrameSize,
	kInputAntialiasing,
	
	kInputPointAttenuation,
	kInputConstantPointAttenuation,
	kInputLinearPointAttenuation,
	kInputQuadracticPointAttenuation,
	
	kInputCalcNormals,
	kInputNormalSmoothness,
	kInputHQNormals,
	
	kInputRotationX,
	kInputRotationY,
	kInputRotationZ,
	
	kInputTranslateX,
	kInputTranslateY,
	kInputTranslateZ,
	
	kInputScaleX,
	kInputScaleY,
	kInputScaleZ,
	
	kInputEnableClip,
	kInputClipMin,
	kInputClipMax,
	
	kInputBypass,
	
	// ** IMPORTANT ** first output property index must start at 1!
	
	kOutputVideo = 1	// first output propety count; note "= 1" to restart count
};


// ---------------------
//	Help String
// ---------------------
// ### Help Strings
//
// The first help string is for the actor in general. This followed by help strings
// for all of the inputs, and then by the help strings for all of the outputs. These
// should be given in the order that they are defined in the Property Definition
// String above.
//
// In all, the total number of help strings should be (num inputs + num outputs + 1)
//
// Note that each string is followed by a comma -- it is a common mistake to forget the
// comma which results in the two strings being concatenated into one.

const char* sHelpStrings[] =
{
	"An Isadora native implementation of the v002 Rutt Extra plugin created by Vade. Mark sends a massive "
		"Troika Salute to the amazing Vade who coded this plugin. Vade's original credits read as follows:\n\n"
		"For Bill.\n\nEmulates the Rutt/Etra raster based analog computer, video synthesizer and effects system. "
		"This software is donation ware. All proceeds go towards helping Bill Etra and the further development "
		"of this plugin.\n\nIf you would like to donate, please visit http://v002.info. \n\nThank you.",

	"The source video stream be displaced.",
	
	"The video whose luma wil displace the sorce video. If no video input is supplied, then the video stream "
		"from the 'video' input is used to displace itself.",
	
	"The video stream as the point sprite.",

	"Internal horizontal resolution of the Rutt-Etra effect. (High values for this input may lead "
		"to poor performance!)",
	
	"Internal vertical resolution of the Rutt-Etra effect.  (High values for this input may lead "
		"to poor performance!)",
	
	"Drawing mode of the Rutt-Etra effect.",
	
	"Displacement mode of the Rutt-Etra effect. Normally you will leave this set to 'luminosity'. The "
		"'raw rgb to xyz' topion produces more unusual (and unpredcitable) results.",
	
	"Blend mode used when rendering the lines or points of the Rutt-Etra effect. 'add' will simply add the color "
		"of new lines or points to those drawn previously; 'over' and 'replace' will obscure previously drawn "
		"lines or points. The blend mode is affected by the setting of the 'depth mode' input. Specifically, if you "
		"want to see 'add' mode work as expected, set the 'depth mode' to 'none' or 'read only.'",
	
	"The depth mode determines if a point or line with a lower z-value will be obscured by those with a higher "
		"z value. When set to 'none', no depth testing is performed. The 'read/write' and 'read only' modes provide "
		"slightly different results.",
	
	"Modulates the color of the incoming image. Leave this set to white to see the colors from the source image stream.",
	
	"Color correction. (Not Used)",
	
	"Texture Coordinate Mode.",
	
	"The amount of displacement applied to the main video input.",
	
	"Size of the scalines or points. If the 'point_attenuation' input is turned on, the point size will be modulated "
		"based on the constant, linear, and quadractic point attenuation factors.",
	
	"When the 'draw mode' input is set to 'points' and this input is turned on, the 'point constant atten', 'point linear atten' "
		"and 'point quadractic atten' inputs are used to reduce the size of the points based on their distance ",
	
	"Constant point size attenuation factor. See 'point attenuation' for more information.",
	"Linear point size attenuation factor. See 'point_attenuation' for more information.",
	"Quadratic point size attenuation factor. See 'point_attenuation' for more information.",
	
	"Calculate normals for the lines or points, which allows them to respond to lighting.",
	"Smoothness factor for the calculated normals.",
	"When enabled, generate high quality normals. (Slower!)",
	
	"Rotation of the affected image around the x-axis.",
	"Rotation of the affected image around the y-axis.",
	"Rotation of the affected image around the z-axis.",
	
	"Translation of the affected image along the x-axis.",
	"Translation of the affected image along the y-axis.",
	"Translation of the affected image along the z-axis.",
	
	"Scaling of the affected image along the x-axis. Higher values produce more space between the scalines or points.",
	"Scaling of the affected image along the y-axis. Higher values produce more space between the scalines or points.",
	"Scaling of the affected image along the z-axis. Higher values produce more space between the scalines or points.",

	"Turn this input on to enable the 'min clipping plane' and 'max clipping plane' inputs.",
	"Don't render points or whose extrusion amount is less than this value.",
	"Don't render points or whose extrusion amount is greater than this value.",
	
	"Bypasses the effect passing the first video input directly to the output"
	
// OUTPUTS
	"The 'Rutt-Etra-fied' video output."
};

// ---------------------------------------------------------------------------------
//		CreateActor
// ---------------------------------------------------------------------------------
static void
CreateActor(
	IsadoraParameters*	ip,
	ActorInfo*			ioActorInfo)
{
	PluginInfo* info = (PluginInfo*) IzzyMallocClear_(ip, sizeof(PluginInfo));
	PluginAssert_(ip, info != NULL);
	
	ioActorInfo->mActorDataPtr = info;
	info->mActorInfoPtr = ioActorInfo;

	info->mImageBufferMap.mInputBufferCount = 1;
	info->mImageBufferMap.mOutputBufferCount = 1;
	CreateImageBufferMap(ip, &info->mImageBufferMap);
}

// ---------------------------------------------------------------------------------
//		DisposeActor
// ---------------------------------------------------------------------------------
static void
DisposeActor(
	IsadoraParameters*	ip,
	ActorInfo*			ioActorInfo)
{
	PluginInfo* info = GetPluginInfo_(ioActorInfo);
	PluginAssert_(ip, info != NULL);
	
	DisposeImageBufferMap(ip, &info->mImageBufferMap);

	PluginAssert_(ip, ioActorInfo->mActorDataPtr != NULL);
	IzzyFree_(ip, ioActorInfo->mActorDataPtr);

}


// ---------------------------------------------------------------------------------
//		DisposeCoreImageFilters
// ---------------------------------------------------------------------------------

#if COMPILE_CORE_VIDEO_THIS_PLUGIN
static void
DisposeCoreImageFilters(
	IsadoraParameters*	ip,
	ActorInfo*			inActorInfo)
{
	PluginInfo* info = GetPluginInfo_(inActorInfo);

	if (info->mCIColorFilter != NULL) {
		ReleaseCIFilter(info->mCIColorFilter);
		info->mCIColorFilter = NULL;
	}
}
#endif

// ---------------------------------------------------------------------------------
//		CreateRuttEtra
// ---------------------------------------------------------------------------------

static void
CreateRuttEtra(
	IsadoraParameters*	ip,
	ActorInfo*			inActorInfo)
{
	PluginInfo* info = GetPluginInfo_(inActorInfo);
	PluginAssert_(ip, info->mRuttEtra == NULL);
	info->mRuttEtra = new v002RuttEtraPlugIn(ip);
}

// ---------------------------------------------------------------------------------
//		DisposeRuttEtraResources
// ---------------------------------------------------------------------------------

static void
DisposeRuttEtraResources(
	IsadoraParameters*	ip,
	ActorInfo*			inActorInfo)
{
	PluginInfo* info = GetPluginInfo_(inActorInfo);

	if (info->mRuttEtra != NULL) {
		
		if (info->mv->HasOutputFrameBuffer()) {
		
			IzzyFrameBuffer fboOut = info->mv->GetOutputFrameBuffer();
			PluginAssert_(ip, fboOut != NULL);
			
			if (BindIzzyFrameBuffer_(ip, inActorInfo, fboOut, TRUE)) {
				info->mRuttEtra->DeleteAllGLResourcesForIzzyFrameBuffer(fboOut);
				info->mRuttEtra->ClearOwningIzzyFBO();
				UnbindIzzyFrameBuffer_(ip, inActorInfo, fboOut);
			}
			
		} else if (info->mRuttEtra->GetOwningIzzyFBO() != NULL) {
			bool Rutt_Etra_Plugin_Has_Non_Matching_Owning_FBO = true;
			PluginAssert_(ip, Rutt_Etra_Plugin_Has_Non_Matching_Owning_FBO == false);
		}
	}
}


// ---------------------------------------------------------------------------------
//		ActivateActor
// ---------------------------------------------------------------------------------

static void
ActivateActor(
	IsadoraParameters*	ip,
	ActorInfo*			inActorInfo,
	Boolean				inActivate)
{
	PluginInfo* info = GetPluginInfo_(inActorInfo);
	
	if (inActivate) {
	
		info->mImageBufferMap.mDefaultPixelFormat = GetActorDefaultPixelFormat_(ip, inActorInfo);
		
		info->mMessageReceiver = CreateActorMessageReceiver_(
			ip,
			inActorInfo,
			ReceiveMessage,
			0,
			kWantVideoFrameTick | kWantStageAboutToDispose,		// GPU: add kWantStageAboutToDispose
			(long) inActorInfo);
		
		// setup Multi Video Input object
		info->mv = new CMultiVideoInOut(ip, inActorInfo, 3, kOutputVideo);
		IzzyDebugAssert_(info->mv != NULL);
		info->mv->SetVideoOutputTypeBasedOnProperty(kOutputProperty, kOutputVideo);

		CreateRuttEtra(ip, inActorInfo);

		#if COMPILE_CORE_VIDEO_THIS_PLUGIN
		info->mCIColorFilter = NULL;
		#endif

	} else {
	
		// make sure that the output is set to NULL
		Value v;
		v.type = kData;
		v.u.data = NULL;
		SetOutputPropertyValue_(ip, inActorInfo, kOutputVideo, &v);

		// dispose our message receiver
		if (info->mMessageReceiver != NULL) {
			DisposeMessageReceiver_(ip, info->mMessageReceiver);
			info->mMessageReceiver = NULL;
			info->mv->SetNeedsDraw();
		}

		DisposeRuttEtraResources(ip, inActorInfo);
		
		delete info->mRuttEtra;
		info->mRuttEtra = NULL;

		#if COMPILE_CORE_VIDEO_THIS_PLUGIN
		DisposeCoreImageFilters(ip, inActorInfo);
		#endif
		
		DisposeOwnedImageBuffers(ip, &info->mImageBufferMap);
		ClearSourceBuffers(ip, &info->mImageBufferMap);

		IzzyDebugAssert_(info->mv != NULL);
		delete info->mv;
		info->mv = NULL;
	}
}

// ---------------------------------------------------------------------------------
//		GetParameterString
// ---------------------------------------------------------------------------------

static const char*
GetParameterString(
	IsadoraParameters*	/* ip */,
	ActorInfo*			/* inActorInfo */)
{
	return sPropertyDefinitionString;
}

// ---------------------------------------------------------------------------------
//		GetHelpString
// ---------------------------------------------------------------------------------

static void
GetHelpString(
	IsadoraParameters*	ip,
	ActorInfo*			inActorInfo,
	PropertyType		inPropertyType,
	PropertyIndex		inPropertyIndex1,
	char*				outParamaterString,
	UInt32				inMaxCharacters)
{
	const char* helpstr = NULL;
	
	UInt32 index1 = PropertyTypeAndIndexToHelpIndex_(ip, inActorInfo, inPropertyType, inPropertyIndex1);
	
	helpstr = sHelpStrings[index1];
	
	strncpy(outParamaterString, helpstr, inMaxCharacters);
}
	
// ---------------------------------------------------------------------------------
//		HandlePropertyChangeValue	[INTERRUPT SAFE]
// ---------------------------------------------------------------------------------

static void
HandlePropertyChangeValue(
	IsadoraParameters*	ip,
	ActorInfo*			inActorInfo,
	PropertyIndex		inPropertyIndex1,
	ValuePtr			/* inOldValue */,
	ValuePtr			inNewValue,
	Boolean				inInitializing)
{
	PluginInfo* info = GetPluginInfo_(inActorInfo);

	PluginAssert_(ip, info->mRuttEtra != NULL);
	
	bool updatePointInputNotEnable = false;
	
	switch (inPropertyIndex1) {
	case kInputVideoIn:
	case kInputVideoLuma:
	case kInputVideoPointSprite:
		{
			info->mv->SetVideoInputFromValue(inPropertyIndex1 - kInputVideoIn, inNewValue);
			SetImageBufferValue(ip, &info->mImageBufferMap, inPropertyIndex1 - kInputVideoIn, GetDataValueOfType(inNewValue, kImageBufferDataType, ImageBufferPtr));
			
			if (info->mBypass == false) {
				info->mv->SetNeedsDraw();
			} else if (inPropertyIndex1 == kInputVideoIn) {
				info->mv->SetVideoOutput(inNewValue);
			}
		}
		break;
		
	// SIZE
	case kInputSizeH:
	case kInputSizeV:
		{
			GLuint w = 0, h = 0;
			info->mRuttEtra->GetResolution(&w, &h);
			if (inPropertyIndex1 == kInputSizeH) {
				w = inNewValue->u.ivalue;
			} else {
				h = inNewValue->u.ivalue;
			}
			info->mRuttEtra->SetResolution(w, h);
			info->mv->SetNeedsDraw();
		}
		break;

	// DRAW MODE
	case kInputDrawMode:
		{
			info->mRuttEtra->SetDrawType(inNewValue->u.ivalue);
			updatePointInputNotEnable = true;
			info->mv->SetNeedsDraw();
		}
		break;

	// DISPLACE MODE
	case kInputDisplaceMode:
		info->mRuttEtra->inputDisplaceMode = inNewValue->u.ivalue;
		info->mv->SetNeedsDraw();
		break;

	// DEPTH MODE
	case kInputDepthMode:
		info->mRuttEtra->inputDepthMode = inNewValue->u.ivalue;
		info->mv->SetNeedsDraw();
		break;

	// BLEND
	case kInputBlendMode:
		info->mRuttEtra->inputBlendMode = inNewValue->u.ivalue;
		info->mv->SetNeedsDraw();
		break;

	// COLOR
	case kInputColor:
		info->mRuttEtra->inputModulationColor = inNewValue->u.ivalue;
		info->mv->SetNeedsDraw();
		break;

	case kInputColorCorrect:
		info->mRuttEtra->inputColorCorrect = false;
		info->mv->SetNeedsDraw();
		break;
	
	// TEXTURE COORDINATE MODE
	case kInputTexCoorMode:
		info->mRuttEtra->inputUVGenMode = inNewValue->u.ivalue;;
		info->mv->SetNeedsDraw();
		break;

	case kInputZExtrude:
		info->mRuttEtra->inputZExtrude = inNewValue->u.fvalue;
		info->mv->SetNeedsDraw();
		break;
		
	case kInputWireFrameSize:
		info->mRuttEtra->inputWireFrameWidth = inNewValue->u.fvalue;
		info->mv->SetNeedsDraw();
		break;

	case kInputAntialiasing:
		info->mRuttEtra->inputAntialias = (inNewValue->u.ivalue != 0);
		info->mv->SetNeedsDraw();
		break;

	// POINT ATTENTUATION
	case kInputPointAttenuation:
		info->mRuttEtra->inputAttenuatePoints = (inNewValue->u.ivalue != 0);
		updatePointInputNotEnable = true;
		info->mv->SetNeedsDraw();
		break;

	case kInputConstantPointAttenuation:
		info->mRuttEtra->inputConstantAttenuation  = inNewValue->u.fvalue;
		info->mv->SetNeedsDraw();
		break;

	case kInputLinearPointAttenuation:
		info->mRuttEtra->inputLinearAttenuation = inNewValue->u.fvalue;
		info->mv->SetNeedsDraw();
		break;

	case kInputQuadracticPointAttenuation:
		info->mRuttEtra->inputQuadraticAttenuation = inNewValue->u.fvalue;
		info->mv->SetNeedsDraw();
		break;
		
	// NORMALS
	case kInputCalcNormals:
		info->mRuttEtra->SetCalculateNormalsFlag(inNewValue->u.ivalue != 0);
		SetPropertyNotApplicableFlag_(ip, inActorInfo, kInputProperty, kInputNormalSmoothness, !inNewValue->u.ivalue);
		SetPropertyNotApplicableFlag_(ip, inActorInfo, kInputProperty, kInputHQNormals, !inNewValue->u.ivalue);
		info->mv->SetNeedsDraw();
		break;

	case kInputNormalSmoothness:
		info->mRuttEtra->SetNormalsSmoothingCoefficient(inNewValue->u.fvalue);
		info->mv->SetNeedsDraw();
		break;

	case kInputHQNormals:
		info->mRuttEtra->SetHighQualityNormalsFlag(inNewValue->u.ivalue != 0);
		info->mv->SetNeedsDraw();
		break;

	// ROTATION
	case kInputRotationX:
		info->mRuttEtra->inputRotationX = inNewValue->u.fvalue;
		info->mv->SetNeedsDraw();
		break;

	case kInputRotationY:
		info->mRuttEtra->inputRotationY = inNewValue->u.fvalue;
		info->mv->SetNeedsDraw();
		break;

	case kInputRotationZ:
		info->mRuttEtra->inputRotationZ = inNewValue->u.fvalue;
		info->mv->SetNeedsDraw();
		break;

	// TRANSLATION
	case kInputTranslateX:
		info->mRuttEtra->inputTranslationX = inNewValue->u.fvalue;
		info->mv->SetNeedsDraw();
		break;

	case kInputTranslateY:
		info->mRuttEtra->inputTranslationY = inNewValue->u.fvalue;
		info->mv->SetNeedsDraw();
		break;

	case kInputTranslateZ:
		info->mRuttEtra->inputTranslationZ = inNewValue->u.fvalue;
		info->mv->SetNeedsDraw();
		break;

	// SCALING
	case kInputScaleX:
		info->mRuttEtra->inputScaleX = inNewValue->u.fvalue;
		info->mv->SetNeedsDraw();
		break;

	case kInputScaleY:
		info->mRuttEtra->inputScaleY = inNewValue->u.fvalue;
		info->mv->SetNeedsDraw();
		break;

	case kInputScaleZ:
		info->mRuttEtra->inputScaleZ = inNewValue->u.fvalue;
		info->mv->SetNeedsDraw();
		break;

	// CLIPPING
	case kInputEnableClip:
		info->mRuttEtra->inputEnableClipping = (inNewValue->u.ivalue != 0);
		SetPropertyNotApplicableFlag_(ip, inActorInfo, kInputProperty, kInputClipMin, !info->mRuttEtra->inputEnableClipping);
		SetPropertyNotApplicableFlag_(ip, inActorInfo, kInputProperty, kInputClipMax, !info->mRuttEtra->inputEnableClipping);
		info->mv->SetNeedsDraw();
		break;

	case kInputClipMin:
		info->mRuttEtra->inputMinClip = inNewValue->u.fvalue / 100.0f;
		info->mv->SetNeedsDraw();
		break;

	case kInputClipMax:
		info->mRuttEtra->inputMaxClip = inNewValue->u.fvalue / 100.0f;
		info->mv->SetNeedsDraw();
		break;

	// BYPASS
	case kInputBypass:
		info->mBypass = (inNewValue->u.ivalue != 0);
		
		if (info->mBypass == false) {
			UpdateActorMessageReceiver_(
				ip,
				info->mMessageReceiver,
				inActorInfo,
				ReceiveMessage,
				0,
				kWantVideoFrameTick | kWantStageAboutToDispose,
				(long) inActorInfo);
			info->mv->SetNeedsDraw();
		
		} else {
		
			UpdateActorMessageReceiver_(
				ip,
				info->mMessageReceiver,
				inActorInfo,
				ReceiveMessage,
				0,
				kWantStageAboutToDispose,
				(long) inActorInfo);
			if (inInitializing == false) {
				ValuePtr v = GetInputPropertyValue_(ip, inActorInfo, kInputVideoIn);
				info->mv->SetVideoOutput(v);
			}
		}
		
		break;
	}
	
	if (updatePointInputNotEnable) {
		bool enablePointInputs = info->mRuttEtra->GetDrawType() == kDrawType_Points;
		SetPropertyNotApplicableFlag_(ip, inActorInfo, kInputProperty, kInputPointAttenuation, !enablePointInputs);
		SetPropertyNotApplicableFlag_(ip, inActorInfo, kInputProperty, kInputConstantPointAttenuation, !(enablePointInputs && info->mRuttEtra->inputAttenuatePoints));
		SetPropertyNotApplicableFlag_(ip, inActorInfo, kInputProperty, kInputLinearPointAttenuation, !(enablePointInputs && info->mRuttEtra->inputAttenuatePoints));
		SetPropertyNotApplicableFlag_(ip, inActorInfo, kInputProperty, kInputQuadracticPointAttenuation, !(enablePointInputs && info->mRuttEtra->inputAttenuatePoints));
	}
}

// ---------------------------------------------------------------------------------
//		HandlePropertyChangeType
// ---------------------------------------------------------------------------------
typedef struct {
	PropertyType	type;
	PropertyIndex	index;
} PropItem;

static PropItem sPropItems[2] = {
	{ kInputProperty, kInputVideoIn },
	{ kOutputProperty, kOutputVideo }
};

static void
HandlePropertyChangeType(
	IsadoraParameters*					ip,
	ActorInfo*							inActorInfo,
	const PropertyDefinedValueSource*	inPropDefValueSrc,
	PropertyType						inPropertyType,
	PropertyIndex						inPropertyIndex1,
	PropertyDispFormat					inAvailableFormats,
	PropertyDispFormat					inCurFormat,
	ValuePtr							inMinimum,
	ValuePtr							inMaximum,
	ValuePtr							inScaleMin,
	ValuePtr							inScaleMax,
	ValuePtr							inValue)
{
	static bool sHandlePropertyChangeType = false;
	
	if (!sHandlePropertyChangeType) {
	
		const size_t kMaxVideoProperties = sizeof(sPropItems) / sizeof(PropItem);
		
		sHandlePropertyChangeType = true;
		
		for (size_t i=0; i<kMaxVideoProperties; i++) {
			if (sPropItems[i].type == inPropertyType && sPropItems[i].index == inPropertyIndex1) {
				for (size_t j=0; j<kMaxVideoProperties; j++) {
					if (j != i) {
						ChangePropertyType_(ip, inActorInfo, inPropDefValueSrc, sPropItems[j].type, sPropItems[j].index, inAvailableFormats, inCurFormat, inMinimum, inMaximum, inScaleMin, inScaleMax, inValue);
					}
				}
				break;
			}
		}

		PluginInfo* info = GetPluginInfo_(inActorInfo);
		if (info != NULL && info->mv != NULL) {
			info->mv->SetVideoOutputTypeBasedOnProperty(kOutputProperty, kOutputVideo);
		}
		
		sHandlePropertyChangeType = false;
	}
}

// ---------------------------------------------------------------------------------
//		GetActorDefinedArea
// ---------------------------------------------------------------------------------

static ActorPictInfo	gPictInfo = { false, nil, nil, 0, 0 };

static Boolean
GetActorDefinedArea(
	IsadoraParameters*			ip,			
	ActorInfo*					inActorInfo,
	SInt16*						outTopAreaWidth,
	SInt16*						outTopAreaMinHeight,
	SInt16*						outBotAreaHeight,
	SInt16*						outBotAreaMinWidth)
{
	if (gPictInfo.mInitialized == false) {
		PrepareActorDefinedAreaPict_(ip, inActorInfo, 0, &gPictInfo);
	}
	
	*outTopAreaWidth = gPictInfo.mWidth;
	*outTopAreaMinHeight = gPictInfo.mHeight;
	*outBotAreaHeight = 0;
	*outBotAreaMinWidth = 0;

	
	return true;
}

// ---------------------------------------------------------------------------------
//		DrawActorDefinedArea
// ---------------------------------------------------------------------------------

static void
DrawActorDefinedArea(
	IsadoraParameters*			ip,
	ActorInfo*					inActorInfo,
	void*						/* inDrawingContext */,
	ActorDefinedAreaPart		inActorDefinedAreaPart,
	ActorAreaDrawFlagsT			/* inAreaDrawFlags */,
	Rect*						inADAArea,
	Rect*						/* inUpdateArea */,
	Boolean						inSelected)
{
	if (inActorDefinedAreaPart == kActorDefinedAreaTop
	&& gPictInfo.mInitialized) {
		DrawActorDefinedAreaPict_(ip, inActorInfo, inSelected, inADAArea, &gPictInfo);
	}
}
	
// ---------------------------------------------------------------------------------
//		StringValueConversionSetup
// ---------------------------------------------------------------------------------

static const char**
StringValueConversionSetup(
	PropertyType		inPropertyType,
	PropertyIndex		inPropertyIndex1,
	SInt32&				outMin,
	SInt32&				outMax)
{
	outMin = outMax = 0;
	const char** outNameArray = NULL;
	
	if (inPropertyType == kInputProperty) {
		
		switch (inPropertyIndex1) {
		
		case kInputDisplaceMode:
			outNameArray = sDisplaceMode;
			outMax = 1;
			break;
		
		case kInputDrawMode:
			outNameArray = sDrawModes;
			outMax = 4;
			break;
			
		case kInputTexCoorMode:
			outNameArray = sTexCoordMode;
			outMax = 3;
			break;
			
		case kInputDepthMode:
			outNameArray = sDepthModes;
			outMax = 2;
			break;
		
		case kInputBlendMode:
			outNameArray = sBlendModes;
			outMax = 2;
			break;
		
		default:
			break;
		}
		
	}
	
	return outNameArray;
}

// ---------------------------------------------------------------------------------
//		PropertyValueToString
// ---------------------------------------------------------------------------------
//	Handles incoming video frame changed messages

static Boolean
PropertyValueToString(
	IsadoraParameters*	ip,
	ActorInfo*			/* inActorInfo */,
	PropertyType		inPropertyType,
	PropertyIndex		inPropertyIndex1,
	ValuePtr			inValue,
	char*				outString)
{
	Boolean result = false;
	
	SInt32 min = 0;
	SInt32 max = 0;
	const char** nameArray = StringValueConversionSetup(inPropertyType, inPropertyIndex1, min, max);

	if (nameArray) {
		if (inValue->u.ivalue >= min && inValue->u.ivalue <= max) {
			strcpy(outString, nameArray[inValue->u.ivalue - min]);
		} else {
			PluginAssert_(ip, false);
		}
		result = true;
	}
	
	return result;
}

// ---------------------------------------------------------------------------------
//		PropertyStringToValue
// ---------------------------------------------------------------------------------

static Boolean
PropertyStringToValue(
	IsadoraParameters*	ip,
	ActorInfo*			/* inActorInfo */,
	PropertyType		inPropertyType,
	PropertyIndex		inPropertyIndex1,
	const char*			inString,
	ValuePtr			outValue)
{
	Boolean encodeSuccessful = false;
	
	outValue->type = kInteger;
	
	SInt32 min = 0;
	SInt32 max = 0;
	const char** nameArray = StringValueConversionSetup(inPropertyType, inPropertyIndex1, min, max);
	
	if (nameArray != NULL) {
		SInt32 matchIndex = LookupPartialStringInList_(ip, max - min + 1, nameArray, inString);
		if (matchIndex >= 0) {
			outValue->u.ivalue = matchIndex+min;
			encodeSuccessful = true;
		} else {
			int tempValue = 0;
			if (sscanf(inString, "%d", &tempValue) == 1) {
				if (tempValue >= min && tempValue < max) {
					outValue->u.ivalue = tempValue;
					encodeSuccessful = true;
				}
			}
		}
	}
	
	return encodeSuccessful;
}

// ---------------------------------------------------------------------------------
//		GetActorInfo
// ---------------------------------------------------------------------------------
//	This is function is called by to get the actor's class and ID, and to get
//	pointers to the all of the plugin functions declared locally.
//
//	All members of the ActorInfo struct pointed to by outActorParams have been
//	set to 0 on entry. You only need set functions defined by your plugin
//	
EXPORT_ void
GetActorInfo(
	void*				/* inParam */,
	ActorInfo*			outActorParams)
{
	// REQUIRED information
	outActorParams->mActorName							= kActorName;
	outActorParams->mClass								= kActorClass;
	outActorParams->mID									= kActorID;
	outActorParams->mCompatibleWithVersion				= kCurrentIsadoraCallbackVersion;
	#if TARGET_OS_MAC
	outActorParams->mActorFlags							|= kActorFlags_NeverUnloadPlugin;
	#endif
	
	// REQUIRED functions
	outActorParams->mGetActorParameterStringProc		= GetParameterString;
	outActorParams->mGetActorHelpStringProc				= GetHelpString;
	outActorParams->mCreateActorProc					= CreateActor;
	outActorParams->mDisposeActorProc					= DisposeActor;
	outActorParams->mActivateActorProc					= ActivateActor;
	outActorParams->mHandlePropertyChangeValueProc		= HandlePropertyChangeValue;
	
	// OPTIONAL FUNCTIONS
	outActorParams->mHandlePropertyChangeTypeProc		= HandlePropertyChangeType;
	outActorParams->mHandlePropertyConnectProc			= NULL;
	outActorParams->mPropertyValueToStringProc			= PropertyValueToString;
	outActorParams->mPropertyStringToValueProc			= PropertyStringToValue;
	outActorParams->mGetActorDefinedAreaProc			= GetActorDefinedArea;
	outActorParams->mDrawActorDefinedAreaProc			= DrawActorDefinedArea;
	outActorParams->mMouseTrackInActorDefinedAreaProc	= NULL;

}

// ---------------------------------------------------------------------------------
//		ProcessTextureImages
// ---------------------------------------------------------------------------------
static void
ProcessTextureImages(
	IsadoraParameters*		ip,
	ActorInfo*				inActorInfo,
	const CFrameFormatInfo&	inFFI)
{
	PluginInfo*	info = (PluginInfo*) inActorInfo->mActorDataPtr;

	TextureBufferPtr tb = info->mv->mValidInputArray[0]->GetTextureImageInput();
	
	if (tb != NULL && tb->mTexture != NULL && tb->mTargetStageIndex0 == inFFI.mTargetStageIndex0) {
		
		// IzzyTexture tx = tb->mTexture;
					
		info->mv->UpdateTextureOutput(inFFI.mTargetStageIndex0, inFFI.mFrameSize);

		IzzyFrameBuffer fboOut = info->mv->GetOutputFrameBuffer();
		
		// #define __ctx__accessor	(fboOut->mFBOGLFunctions)
		
		PluginAssert_(ip, info->mRuttEtra != NULL);

		GLuint reSizeH = 0, reSizeV = 0;
		info->mRuttEtra->GetResolution(&reSizeH, &reSizeV);

		if (fboOut != NULL && reSizeH > 0 && reSizeV > 0) {
			
			Value v;
			v.type = kData;
			v.u.data = NULL;
			
			if (UpdateIzzyFrameBuffer_(ip, inActorInfo, fboOut, inFFI.mTargetStageIndex0, inFFI.mFrameSize.width, inFFI.mFrameSize.height, TRUE, NULL)) {
				
				if (BindIzzyFrameBuffer_(ip, inActorInfo, fboOut, TRUE)) {
				
					if (info->mRuttEtra != NULL) {
					
						info->mRuttEtra->SetOwningIzzyFBO(fboOut);
			
						IzzyTexture mainImage = NULL;
						IzzyTexture displaceImage = NULL;
						IzzyTexture pointSpriteImage = NULL;
						
						if (info->mv->mVideoInputArray != NULL) {
						
							if (info->mv->mVideoInputArray[0].GetTextureImageInput() != NULL) {
								mainImage = info->mv->mVideoInputArray[0].GetTextureImageInput()->mTexture;
							}
							
							if (info->mv->mVideoInputArray[1].GetTextureImageInput() != NULL) {
								displaceImage = info->mv->mVideoInputArray[1].GetTextureImageInput()->mTexture;
							}
							
							if (info->mv->mVideoInputArray[2].GetTextureImageInput() != NULL) {
								pointSpriteImage = info->mv->mVideoInputArray[2].GetTextureImageInput()->mTexture;
							}
						}
						
						if (mainImage != NULL) {
							info->mRuttEtra->Execute(mainImage, displaceImage, pointSpriteImage);
						}
					}
				
					TextureBufferPtr outputBuffer = UnbindIzzyFrameBuffer_(ip, inActorInfo, fboOut);
					
					if (outputBuffer != NULL && outputBuffer->mTexture != NULL) {
						outputBuffer->mTexture->SetTextureIsPremultiplied(tb->mTexture->GetTextureIsPremultiplied());
						v.u.data = outputBuffer;
					}
				}
			}
			
			if (v.u.data != NULL) {
				info->mv->SetVideoOutput(&v);
			}
			
		} else {
			info->mv->InvalidateVideoOutput();
		}
		
	} else {
		info->mv->InvalidateVideoOutput();
	}
}

// ---------------------------------------------------------------------------------
//		ProcessCoreImageImages
// ---------------------------------------------------------------------------------
#if COMPILE_CORE_VIDEO_THIS_PLUGIN
static void
ProcessCoreImageImages(
	IsadoraParameters*		ip,
	ActorInfo*				inActorInfo,
	const CFrameFormatInfo&	inFFI)
{
	PluginInfo*	info = (PluginInfo*) inActorInfo->mActorDataPtr;

	CoreImageBufferPtr cb = info->mv->mValidInputArray[0]->GetCoreImageInput();
	
	if (cb != NULL && cb->mImage != NULL) {
	
		if (info->mCIColorFilter == NULL) {
			info->mCIColorFilter = CreateColorMatrixFilter();
		}
		
		if (info->mCIColorFilter != NULL) {
			
			CoreImageBuffer& ciBuffer = info->mv->GetOutputCoreImageBuffer();
			
			CI_Image oldImage = ciBuffer.mImage;
			ciBuffer.mImage = NULL;
			
			float argbArray[4];
			argbArray[0] = 1.0;
			argbArray[1] = info->mRedAmount + 1.0f;
			argbArray[2] = info->mGreenAmount + 1.0f;
			argbArray[3] = info->mBlueAmount + 1.0f;
			
			CI_Image scaledImage = ScaleImageTo(cb->mImage, inFFI.mFrameSize.width,  inFFI.mFrameSize.height);
			
			CI_Image finalImage = ColorMatrixFilter_ModulateImageARGB_Individual(info->mCIColorFilter, scaledImage, argbArray);
		
			ciBuffer.mImage = finalImage;
			ciBuffer.mTargetStageIndex0 = cb->mTargetStageIndex0;
			#if DEBUG
			PluginAssert_(ip, ciBuffer.mImage != NULL);
			#endif
			RetainCIImage(ciBuffer.mImage);
			
			Value v;
			v.type = kData;
			v.u.data = &ciBuffer;
			ciBuffer.mInfo.mDataChangeCount++;
			
			if (oldImage != NULL) {
				ReleaseCIImage(oldImage);
			}
			
			if (v.u.data != NULL) {
				info->mv->SetVideoOutput(&v);
			}

		} else {
			info->mv->InvalidateVideoOutput();
		}
	
	} else {
		info->mv->InvalidateVideoOutput();
	}
}
#endif

// ---------------------------------------------------------------------------------
//		ProcessBitmapImages
// ---------------------------------------------------------------------------------

static void
ProcessBitmapImages(
	IsadoraParameters*	ip,
	ActorInfo*			inActorInfo)
{
	PluginInfo* info = GetPluginInfo_(inActorInfo);

	ImageBufferPtr img1 = GetImageBufferPtr(ip, &info->mImageBufferMap, 0);
	ImageBufferPtr out = GetOutputImageBufferPtr(&info->mImageBufferMap, 0);

	if (img1 != NULL && out != NULL) {

		// then, draw if we need to draw
		if (info->mv->GetNeedsDraw()) {
			
			info->mv->ClearNeedsDraw();
		
			bool drawFrame = false;
			
			// process video bitmap here
			
			if (drawFrame) {
				out->mInfo.mDataChangeCount++;
				Value v;
				v.type = kData;
				v.u.data = out;
				info->mv->SetVideoOutput(&v);
			}
			
		}
		
	} else {
		info->mv->InvalidateVideoOutput();
	}
}

// ---------------------------------------------------------------------------------
//		ReceiveMessage
// ---------------------------------------------------------------------------------
//	Handles incoming midi messages

void
ReceiveMessage(
	IsadoraParameters*	ip,
	MessageMask			inMessageMask,
	PortIndex			/* inPortIndex1 */,
	const MsgData*		inData,
	UInt32				/* inLen */,
	long				inRefCon)
{
	ActorInfo*	inActorInfo = (ActorInfo*) inRefCon;
	PluginInfo*	info = (PluginInfo*) inActorInfo->mActorDataPtr;
	
	if ((inMessageMask & kWantStageAboutToDispose) != 0) {
	
		if (info->mv->WillDisposeBecauseOfStageMessage(inData)) {
			DisposeRuttEtraResources(ip, inActorInfo);
		}
		
		if (info->mv->HandleAboutToDisposeStageMessage(inData)) {
			#if COMPILE_CORE_VIDEO_THIS_PLUGIN
			DisposeCoreImageFilters(ip, inActorInfo);
			#endif
		}
		return;
	}

	Boolean bitmapWasRemapped = false;
	UpdateImageBufferMapNotifyRemap(ip, &info->mImageBufferMap, &bitmapWasRemapped);
	
	CFrameFormatInfo ffi;
	bool imageInputWasRemapped = false;
	info->mv->GetOutputResolutionForImageProcessingMode(ip, ffi, &imageInputWasRemapped);
	
	if (bitmapWasRemapped || imageInputWasRemapped) {
		info->mv->SetNeedsDraw();
	}

	if (info->mv->GetNeedsDraw()) {
	
		UInt64 vpStart = EnterVideoProcessing_(ip);
		
		if (info->mv->GetValidVideoInputCount() > 0 && ffi.mFrameSize.width > 0 && ffi.mFrameSize.height > 0) {
		
			switch (info->mv->GetOutputVideoBufferType()) {
			
			case kVideoBuffer_Bitmap:
				ProcessBitmapImages(ip, inActorInfo);
				break;
			
			#if COMPILE_CORE_VIDEO_THIS_PLUGIN
			case kVideoBuffer_CoreImage:
				if (ffi.mTargetStageIndex0 != kInvalidStageIndex) {
					ProcessCoreImageImages(ip, inActorInfo, ffi);
				} else {
					info->mv->InvalidateVideoOutput();
				}
				break;
			#endif
			
			case kVideoBuffer_Texture:
				if (ffi.mTargetStageIndex0 != kInvalidStageIndex) {
					ProcessTextureImages(ip, inActorInfo, ffi);
				} else {
					info->mv->InvalidateVideoOutput();
				}
				break;
			}

		} else {
			info->mv->InvalidateVideoOutput();
		}
	
		ExitVideoProcessing_(ip, vpStart);
	}
}
