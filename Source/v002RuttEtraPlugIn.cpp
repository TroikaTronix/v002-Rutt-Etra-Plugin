// ===========================================================================
//
//  v002_Rutt_Etra_2_0PlugIn.m
//  v002 Rutt Etra 2.0
//
//  created by vade on 2/15/11
//
//	This code is covered by the following Creative Commons license
//
//	Attribution-NonCommercial-ShareAlike 3.0 Unported (CC BY-NC-SA 3.0)
//
//	https://creativecommons.org/licenses/by-nc-sa/3.0/
//
//	adapted for Isadora by Mark Coniglio
//
// ===========================================================================

#include "v002RuttEtraPlugIn.h"
#include "COpenGLUtil.h"
#include "IsadoraTypes.h"

#ifndef MIN
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#endif
#ifndef MAX
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#endif

//#define	kQCPlugIn_Name				@"v002 Rutt Etra 3.0"
//#define	kQCPlugIn_Description		@"For Bill.\n\nEmulates the Rutt/Etra raster based analog computer, video synthesizer and effects system. This software is donation ware. All proceeds go towards helping Bill Etra and the further development of this plugin.\n\nIf you would like to donate, please visit http://v002.info. \n\nThank you."

#define kActorName "v002RuttEtraPlugIn"

// ---------------------------------------------------------------------------------
// QUTOE - for embedding shader code in source code
// ---------------------------------------------------------------------------------

#define QUOTE(...) #__VA_ARGS__

// ---------------------------------------------------------------------------------
// RuttEtraMRT_2DRect_frag - Fragment Shader
// ---------------------------------------------------------------------------------
// This version handles GL_TEXTURE_2D texture targets

static const GLcharARB* RuttEtraMRT_2D_frag = QUOTE(

varying vec2 texcoord0;

uniform sampler2D tex0;
uniform vec2 imageSize;
uniform float useRaw;
uniform float extrude;

const vec4 lumcoeff = vec4(0.299,0.587,0.114,0.);

void main (void)
{
	vec4 pixel = texture2D(tex0, texcoord0);
	float luma = dot(lumcoeff, pixel);

	float normX = floor(gl_FragCoord.x)  / (imageSize.x - 1.0);
	float normY = floor(gl_FragCoord.y)  / (imageSize.y - 1.0);
	vec4 lumaPixel = vec4(normX, normY, luma * extrude, 1.0);

	gl_FragData[0] = mix(lumaPixel, vec4(pixel.rgb, 1.0), useRaw);
	gl_FragData[1] = vec4(normX, normY, 0.0, 1.0);
}

);

// ---------------------------------------------------------------------------------
// RuttEtraMRT_2DRect_frag - Fragment Shader
// ---------------------------------------------------------------------------------
// This version handles GL_TEXTURE_RECTANGLE_ARB texture targets

static const GLcharARB* RuttEtraMRT_2DRect_frag = QUOTE(

varying vec2 texcoord0;

uniform sampler2DRect tex0;
uniform vec2 imageSize;
uniform float useRaw;
uniform float extrude;

const vec4 lumcoeff = vec4(0.299,0.587,0.114,0.);

void main (void)
{
	vec4 pixel = texture2DRect(tex0, texcoord0);
	float luma = dot(lumcoeff, pixel);

	float normX = floor(gl_FragCoord.x)  / (imageSize.x - 1.0);
	float normY = floor(gl_FragCoord.y)  / (imageSize.y - 1.0);
	vec4 lumaPixel = vec4(normX, normY, luma * extrude, 1.0);

	gl_FragData[0] = mix(lumaPixel, vec4(pixel.rgb, 1.0), useRaw);
	gl_FragData[1] = vec4(normX, normY, 0.0, 1.0);
}

);

// ---------------------------------------------------------------------------------
// RuttEtraMRT - Vertex Shader
// ---------------------------------------------------------------------------------

static const GLcharARB* RuttEtraMRT_vert = QUOTE(
varying vec2 texcoord0;

void main()
{
	// perform standard transform on vertex
	gl_Position = ftransform();

	// transform texcoords
	texcoord0 = vec2(gl_TextureMatrix[0] * gl_MultiTexCoord0);
}

);

// ---------------------------------------------------------------------------------
// RuttEtraMRTNormals_2DRect_frag - Fragment Shader
// ---------------------------------------------------------------------------------

static const GLcharARB* RuttEtraMRTNormals_2D_frag = QUOTE(

varying vec2 texcoord0;

varying vec2 s1Coord;
varying vec2 s2Coord;
varying vec2 s3Coord;

uniform sampler2D tex0;
uniform vec2 imageSize;
uniform float useRaw;
uniform float extrude;

uniform float coef;

const vec4 lumcoeff = vec4(0.299,0.587,0.114,0.);

void main (void)
{
	vec4 pixel = texture2D(tex0, texcoord0);
	float luma = dot(lumcoeff, pixel);

	float normX = floor(gl_FragCoord.x)  / (imageSize.x - 1.0);
	float normY = floor(gl_FragCoord.y)  / (imageSize.y - 1.0);

	vec4 lumaPixel = vec4(normX, normY, luma * extrude, 1.0);

	gl_FragData[0] = mix(lumaPixel, vec4(pixel.rgb, 1.0), useRaw);
	gl_FragData[1] = vec4(normX, normY, 0.0, 1.0);

	// Normal calculation
	float h0 = luma;
	float h1 = dot(lumcoeff, texture2D( tex0, s2Coord ));
	float h2 = dot(lumcoeff, texture2D( tex0, s3Coord ));
			
	vec3 v01 = vec3( s2Coord.x, s2Coord.y, h1 - h0 );
	vec3 v02 = vec3( s3Coord.x, s3Coord.y, h2 - h0 );

	vec3 n = cross( v02, v01 );

	// Can be useful to scale the Z component to tweak the
	// amount bumps show up, less than 1.0 will make them
	// more apparent, greater than 1.0 will smooth them out
	n.z *= -coef;

	gl_FragData[2] =  vec4(normalize( n ), 1.0);
}

);

// ---------------------------------------------------------------------------------
// RuttEtraMRTNormals_2DRect_frag - Fragment Shader
// ---------------------------------------------------------------------------------

static const GLcharARB* RuttEtraMRTNormals_2DRect_frag = QUOTE(

varying vec2 texcoord0;

varying vec2 s1Coord;
varying vec2 s2Coord;
varying vec2 s3Coord;

uniform sampler2DRect tex0;
uniform vec2 imageSize;
uniform float useRaw;
uniform float extrude;

uniform float coef;

const vec4 lumcoeff = vec4(0.299,0.587,0.114,0.);

void main (void)
{
	vec4 pixel = texture2DRect(tex0, texcoord0);
	float luma = dot(lumcoeff, pixel);

	float normX = floor(gl_FragCoord.x)  / (imageSize.x - 1.0);
	float normY = floor(gl_FragCoord.y)  / (imageSize.y - 1.0);

	vec4 lumaPixel = vec4(normX, normY, luma * extrude, 1.0);

	gl_FragData[0] = mix(lumaPixel, vec4(pixel.rgb, 1.0), useRaw);
	gl_FragData[1] = vec4(normX, normY, 0.0, 1.0);

	// Normal calculation
	float h0 = luma;
	float h1 = dot(lumcoeff, texture2DRect( tex0, s2Coord ));
	float h2 = dot(lumcoeff, texture2DRect( tex0, s3Coord ));
			
	vec3 v01 = vec3( s2Coord.x, s2Coord.y, h1 - h0 );
	vec3 v02 = vec3( s3Coord.x, s3Coord.y, h2 - h0 );

	vec3 n = cross( v02, v01 );

	// Can be useful to scale the Z component to tweak the
	// amount bumps show up, less than 1.0 will make them
	// more apparent, greater than 1.0 will smooth them out
	n.z *= -coef;

	gl_FragData[2] =  vec4(normalize( n ), 1.0);
}

);

// ---------------------------------------------------------------------------------
// RuttEtraMRTNormals - Vertex Shader
// ---------------------------------------------------------------------------------

static const GLcharARB* RuttEtraMRTNormals_vert = QUOTE(

varying vec2 texcoord0;

varying vec2 s2Coord;
varying vec2 s3Coord;

void main()
{
	// perform standard transform on vertex
	gl_Position = ftransform();

	// transform texcoords
	texcoord0 = vec2(gl_TextureMatrix[0] * gl_MultiTexCoord0);
		
	s2Coord = texcoord0 + vec2(1.0, 0.0);
	s3Coord = texcoord0 + vec2(0.0, 1.0);
}

);

// ---------------------------------------------------------------------------------
// RuttEtraMRTHQNormals_2DRect_frag - Fragment Shader
// ---------------------------------------------------------------------------------

static const GLcharARB* RuttEtraMRTHQNormals_2D_frag = QUOTE(

varying vec2 texcoord0;

varying vec2 s1Coord;
varying vec2 s2Coord;
varying vec2 s3Coord;
varying vec2 s4Coord;
varying vec2 s5Coord;
varying vec2 s6Coord;
varying vec2 s7Coord;
varying vec2 s8Coord;

uniform sampler2D tex0;
uniform vec2 imageSize;
uniform float useRaw;
uniform float extrude;

uniform float coef;

const vec4 lumcoeff = vec4(0.299,0.587,0.114,0.);

void main (void)
{
	vec4 pixel = texture2D(tex0, texcoord0);
	float luma = dot(lumcoeff, pixel);

	float normX = floor(gl_FragCoord.x)  / (imageSize.x - 1.0);
	float normY = floor(gl_FragCoord.y)  / (imageSize.y - 1.0);

	vec4 lumaPixel = vec4(normX, normY, luma * extrude, 1.0);

	gl_FragData[0] = mix(lumaPixel, vec4(pixel.rgb, 1.0), useRaw);
	gl_FragData[1] = vec4(normX, normY, 0.0, 1.0);

	// Normal calculation

	float h00 = dot(lumcoeff,texture2D( tex0, s1Coord ));
	float h10 = dot(lumcoeff,texture2D( tex0, s2Coord ));
	float h20 = dot(lumcoeff,texture2D( tex0, s3Coord ));

	float h01 = dot(lumcoeff,texture2D( tex0, s4Coord ));
	float h21 = dot(lumcoeff,texture2D( tex0, s5Coord ));

	float h02 = dot(lumcoeff,texture2D( tex0, s6Coord ));
	float h12 = dot(lumcoeff,texture2D( tex0, s7Coord ));
	float h22 = dot(lumcoeff,texture2D( tex0, s8Coord ));

	// The Sobel X kernel is:
	//
	// [ 1.0  0.0  -1.0 ]
	// [ 2.0  0.0  -2.0 ]
	// [ 1.0  0.0  -1.0 ]

	float Gx = h00 - h20 + 2.0 * h01 - 2.0 * h21 + h02 - h22;
				
	// The Sobel Y kernel is:
	//
	// [  1.0    2.0    1.0 ]
	// [  0.0    0.0    0.0 ]
	// [ -1.0   -2.0   -1.0 ]

	float Gy = h00 + 2.0 * h10 + h20 - h02 - 2.0 * h12 - h22;
	//float Gz = h00 + 2.0 * h10 + h20 - h02 - 2.0 * h12 - h22;

	// Generate the missing Z component - tangent
	// space normals are +Z which makes things easier
	// The 0.5f leading coefficient can be used to control
	// how pronounced the bumps are - less than 1.0 enhances
	// and greater than 1.0 smoothes.
	float Gz = coef * sqrt( 1.0 - Gx * Gx - Gy * Gy );
	//float Gy = 0.5 * sqrt( 1.0 - Gx * Gx - Gz * Gz );

	// Make sure the returned normal is of unit length
	gl_FragData[2] = vec4( normalize( vec3( 2.0 * Gx, 2.0 * Gy, Gz)) , 1.0);
	//gl_FragData[2] = vec4( normalize( vec3( 2.0 * Gx, Gy * coef, Gz * 2.0)) , 1.0);

}

);

// ---------------------------------------------------------------------------------
// RuttEtraMRTHQNormals_2DRect_frag - Fragment Shader
// ---------------------------------------------------------------------------------

static const GLcharARB* RuttEtraMRTHQNormals_2DRect_frag = QUOTE(

varying vec2 texcoord0;

varying vec2 s1Coord;
varying vec2 s2Coord;
varying vec2 s3Coord;
varying vec2 s4Coord;
varying vec2 s5Coord;
varying vec2 s6Coord;
varying vec2 s7Coord;
varying vec2 s8Coord;

uniform sampler2DRect tex0;
uniform vec2 imageSize;
uniform float useRaw;
uniform float extrude;

uniform float coef;

const vec4 lumcoeff = vec4(0.299,0.587,0.114,0.);

void main (void)
{
	vec4 pixel = texture2DRect(tex0, texcoord0);
	float luma = dot(lumcoeff, pixel);

	float normX = floor(gl_FragCoord.x)  / (imageSize.x - 1.0);
	float normY = floor(gl_FragCoord.y)  / (imageSize.y - 1.0);

	vec4 lumaPixel = vec4(normX, normY, luma * extrude, 1.0);

	gl_FragData[0] = mix(lumaPixel, vec4(pixel.rgb, 1.0), useRaw);
	gl_FragData[1] = vec4(normX, normY, 0.0, 1.0);

	// Normal calculation

	float h00 = dot(lumcoeff,texture2DRect( tex0, s1Coord ));
	float h10 = dot(lumcoeff,texture2DRect( tex0, s2Coord ));
	float h20 = dot(lumcoeff,texture2DRect( tex0, s3Coord ));

	float h01 = dot(lumcoeff,texture2DRect( tex0, s4Coord ));
	float h21 = dot(lumcoeff,texture2DRect( tex0, s5Coord ));

	float h02 = dot(lumcoeff,texture2DRect( tex0, s6Coord ));
	float h12 = dot(lumcoeff,texture2DRect( tex0, s7Coord ));
	float h22 = dot(lumcoeff,texture2DRect( tex0, s8Coord ));

	// The Sobel X kernel is:
	//
	// [ 1.0  0.0  -1.0 ]
	// [ 2.0  0.0  -2.0 ]
	// [ 1.0  0.0  -1.0 ]

	float Gx = h00 - h20 + 2.0 * h01 - 2.0 * h21 + h02 - h22;
				
	// The Sobel Y kernel is:
	//
	// [  1.0    2.0    1.0 ]
	// [  0.0    0.0    0.0 ]
	// [ -1.0   -2.0   -1.0 ]

	float Gy = h00 + 2.0 * h10 + h20 - h02 - 2.0 * h12 - h22;
	//float Gz = h00 + 2.0 * h10 + h20 - h02 - 2.0 * h12 - h22;

	// Generate the missing Z component - tangent
	// space normals are +Z which makes things easier
	// The 0.5f leading coefficient can be used to control
	// how pronounced the bumps are - less than 1.0 enhances
	// and greater than 1.0 smoothes.
	float Gz = coef * sqrt( 1.0 - Gx * Gx - Gy * Gy );
	//float Gy = 0.5 * sqrt( 1.0 - Gx * Gx - Gz * Gz );

	// Make sure the returned normal is of unit length
	gl_FragData[2] = vec4( normalize( vec3( 2.0 * Gx, 2.0 * Gy, Gz)) , 1.0);
	//gl_FragData[2] = vec4( normalize( vec3( 2.0 * Gx, Gy * coef, Gz * 2.0)) , 1.0);

}

);

// ---------------------------------------------------------------------------------
// RuttEtraMRTHQNormals - Vertex Shader
// ---------------------------------------------------------------------------------

static const GLcharARB* RuttEtraMRTHQNormals_vert = QUOTE(

varying vec2 texcoord0;

varying vec2 s1Coord;
varying vec2 s2Coord;
varying vec2 s3Coord;
varying vec2 s4Coord;
varying vec2 s5Coord;
varying vec2 s6Coord;
varying vec2 s7Coord;
varying vec2 s8Coord;

void main()
{
	// perform standard transform on vertex
	gl_Position = ftransform();

	// transform texcoords
	texcoord0 = vec2(gl_TextureMatrix[0] * gl_MultiTexCoord0);

	s1Coord = texcoord0 + vec2(-1.0, -1.0);
	s2Coord = texcoord0 + vec2(0.0, -1.0);
	s3Coord = texcoord0 + vec2(1.0, -1.0);
	s4Coord = texcoord0 + vec2(-1.0, 0.0);
	s5Coord = texcoord0 + vec2(1.0, 0.0);
	s6Coord = texcoord0 + vec2(-1.0, 1.0);
	s7Coord = texcoord0 + vec2(0.0, 1.0);
	s8Coord = texcoord0 + vec2(1.0, 1.0);
}

);


static void planeEquation(float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3, double* eq)
{
	eq[0] = (y1*(z2 - z3)) + (y2*(z3 - z1)) + (y3*(z1 - z2));
	eq[1] = (z1*(x2 - x3)) + (z2*(x3 - x1)) + (z3*(x1 - x2));
	eq[2] = (x1*(y2 - y3)) + (x2*(y3 - y1)) + (x3*(y1 - y2));
	eq[3] = -((x1*((y2*z3) - (y3*z2))) + (x2*((y3*z1) - (y1*z3))) + (x3*((y1*z2) - (y2*z1))));
}


// ---------------------------------------------------------------------------------
//	v002RuttEtraPlugIn [CONSTRUCTOR]
// ---------------------------------------------------------------------------------

v002RuttEtraPlugIn::v002RuttEtraPlugIn(
	IsadoraParameters*	inIzzyParams) :
		ip(inIzzyParams)
{
	needBufferRecreate = true;
	
	izzyFrameBuffer = NULL;

	mMRTShader_2D = NULL;
	mMRTShader_2DRect = NULL;
	
	mMRTShaderNormals_2D = NULL;
	mMRTShaderNormals_2DRect = NULL;
	
	mMRTShaderHQNormals_2D = NULL;
	mMRTShaderHQNormals_2DRect = NULL;

	mPlaneEQ1 = (double*)malloc(4 * sizeof(double));
	mPlaneEQ2 = (double*)malloc(4 * sizeof(double));

	vaoID = 0;

	// FBO and texture objects
	fboID = 0;
	texureAttachmentVertices = 0;
	texureAttachmentNormals = 0;
	texureAttachmentTextureCoords = 0;

	// Buffer Objects
	vertexBufferID = 0;
	textureBufferID = 0;
	normalBufferID = 0;
	indexBufferID = 0;

	// state saving
	// GL State
	previousFBO = 0;
	previousReadFBO = 0;
	previousDrawFBO = 0;
	previousReadBuffer = 0;
	previousPixelPackBuffer = 0;
	previousShader = 0;

	inputModulationColor = 0;

	inputDisplaceMode = kDisplaceMode_Luminance;
	inputColorCorrect = 0;

	inputResolutionX = 0;
	inputResolutionY = 0;
	inputDrawType = 0;

	inputZExtrude = 0;
	inputWireFrameWidth = 0;
	inputAntialias = 0;
	inputCalculateNormals = 0;
	inputNormalSmoothingCoeff = 0;
	inputHQNormals = 0;
	inputAttenuatePoints = 0;
	inputConstantAttenuation = 0;
	inputLinearAttenuation = 0;
	inputQuadraticAttenuation = 0;

	inputUVGenMode = 0;
	inputTranslationX = 0;
	inputTranslationY = 0;
	inputTranslationZ = 0;
	inputRotationX = 0;
	inputRotationY = 0;
	inputRotationZ = 0;
	inputScaleX = 0;
	inputScaleY = 0;
	inputScaleZ = 0;
	inputBlendMode = 0;
	inputDepthMode = 0;
	inputEnableClipping = 0;
	inputMinClip = 0;
	inputMaxClip = 0;

	#if TARGET_OS_WIN32
	__ctx__accessor = NULL;
	mGLLFunctionsOK = false;
	#endif
}

// ---------------------------------------------------------------------------------
//	v002RuttEtraPlugIn [DESTRUCTOR]
// ---------------------------------------------------------------------------------

v002RuttEtraPlugIn::~v002RuttEtraPlugIn()
{
	PluginAssert_(ip, RequiresContextToDelete() == false);
	ClearOwningIzzyFBO();
    free(mPlaneEQ1);
    free(mPlaneEQ2);
}

// ---------------------------------------------------------------------------------
//	RequiresContextToDelete
// ---------------------------------------------------------------------------------

bool
v002RuttEtraPlugIn::RequiresContextToDelete() const
{
	return (mMRTShader_2D != NULL
				|| mMRTShaderNormals_2D != NULL
				|| mMRTShaderHQNormals_2D != NULL
				|| mMRTShader_2DRect != NULL
				|| mMRTShaderNormals_2DRect != NULL
				|| mMRTShaderHQNormals_2DRect != NULL
				|| vaoID != 0
				|| fboID != 0
				|| texureAttachmentVertices != 0
				|| texureAttachmentNormals != 0
				|| texureAttachmentTextureCoords != 0
				|| vertexBufferID != 0
				|| textureBufferID != 0
				|| normalBufferID != 0
				|| indexBufferID != 0);
}

// ---------------------------------------------------------------------------------
//	DeleteAllGLResourcesForIzzyFrameBuffer
// ---------------------------------------------------------------------------------

void
v002RuttEtraPlugIn::DeleteAllGLResourcesForIzzyFrameBuffer(
	IzzyFrameBuffer	inIzzyFrameBuffer)
{
	if (RequiresContextToDelete()) {
		PluginAssert_(ip, inIzzyFrameBuffer != NULL && inIzzyFrameBuffer == izzyFrameBuffer);
	} else {
		PluginAssert_(ip, inIzzyFrameBuffer == NULL && izzyFrameBuffer == NULL);
	}
	
	if (inIzzyFrameBuffer != NULL && inIzzyFrameBuffer == izzyFrameBuffer) {
		DestroyGLResourcesInContext();
		DestroyPersistantGLResourcesInContext();
	
	} else if (inIzzyFrameBuffer != NULL && inIzzyFrameBuffer != izzyFrameBuffer) {
		bool v002RuttEtraPlugIn_Requires_Matching_FBO_to_Delete = true;
		PluginAssert_(ip, v002RuttEtraPlugIn_Requires_Matching_FBO_to_Delete == false);
	} else if (inIzzyFrameBuffer == NULL && izzyFrameBuffer != NULL) {
		bool v002RuttEtraPlugIn_Requires_Non_NULL_Owning_FBO_to_Delete = true;
		PluginAssert_(ip, v002RuttEtraPlugIn_Requires_Non_NULL_Owning_FBO_to_Delete == false);
	}
}

// ---------------------------------------------------------------------------------
//	SetOwningIzzyFBO
// ---------------------------------------------------------------------------------
// if the destination FBO changes, we must re-allocate all of our resources
// because it might be in an entirely different context

void
v002RuttEtraPlugIn::SetOwningIzzyFBO(
	IzzyFrameBuffer	inIzzyFBO)
{
	if (inIzzyFBO != izzyFrameBuffer || inIzzyFBO == NULL) {
		DestroyPersistantGLResourcesInContext();
		izzyFrameBuffer = NULL;
	}
	
	if (inIzzyFBO != NULL) {

		bool setNewFrameBuffer = (inIzzyFBO != NULL && inIzzyFBO != izzyFrameBuffer);

		izzyFrameBuffer = inIzzyFBO;

		#if TARGET_OS_WIN32
		__ctx__accessor = inIzzyFBO->mFBOGLFunctions;
		#endif

		if (setNewFrameBuffer) {

			#if TARGET_OS_WIN32
			// VERTEX ARRAY OBJECT
			glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC) wglGetProcAddress("glGenVertexArrays");
			glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC) wglGetProcAddress("glDeleteVertexArrays");
			glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC) wglGetProcAddress("glBindVertexArray");

			// BUFFER DATA
			glGenBuffers = (PFNGLGENBUFFERSPROC) wglGetProcAddress("glGenBuffers");
			glDeleteBuffers = (PFNGLDELETEBUFFERSPROC) wglGetProcAddress("glDeleteBuffers");
			glBindBuffer = (PFNGLBINDBUFFERPROC) wglGetProcAddress("glBindBuffer");
			glDrawBuffers = (PFNGLDRAWBUFFERSPROC) wglGetProcAddress("glDrawBuffers");
			glBufferData = (PFNGLBUFFERDATAPROC) wglGetProcAddress("glBufferData");
			glMapBuffer = (PFNGLMAPBUFFERPROC) wglGetProcAddress("glMapBuffer");
			glUnmapBuffer = (PFNGLUNMAPBUFFERPROC) wglGetProcAddress("glUnmapBuffer");

			// FRAMEBUFFER OBJECTS
			glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC) wglGetProcAddress("glGenFramebuffers");
			glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSEXTPROC) wglGetProcAddress("glDeleteFramebuffers");
			glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC) wglGetProcAddress("glBindFramebuffer");
			glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC) wglGetProcAddress("glFramebufferTexture2D");

			// UNIFORM PARAMETERS
			glUniform1iARB = (PFNGLUNIFORM1IARBPROC) wglGetProcAddress("glUniform1iARB");
			glUniform1fARB = (PFNGLUNIFORM1FARBPROC) wglGetProcAddress("glUniform1fARB");
			glUniform2fARB = (PFNGLUNIFORM2FARBPROC) wglGetProcAddress("glUniform2fARB");

			// POINT PARAM
			glPointParameterfv = (PFNGLPOINTPARAMETERFVARBPROC) wglGetProcAddress("glPointParameterfv");

			mGLLFunctionsOK = (
				glGenVertexArrays != NULL
				&& glDeleteVertexArrays != NULL
				&& glBindVertexArray != NULL

				&& glGenBuffers != NULL
				&& glDeleteBuffers != NULL
				&& glBindBuffer != NULL
				&& glDrawBuffers != NULL
				&& glBufferData != NULL
				&& glMapBuffer != NULL
				&& glUnmapBuffer != NULL

				&& glGenFramebuffers != NULL
				&& glDeleteFramebuffers != NULL
				&& glBindFramebuffer != NULL
				&& glFramebufferTexture2D != NULL

				&& glUniform1iARB != NULL
				&& glUniform1fARB != NULL
				&& glUniform2fARB != NULL

				&& glPointParameterfv != NULL);
			#endif

			CreatePersistantGLResourcesInContext();
		}


	}
	
}

// ---------------------------------------------------------------------------------
//	ClearOwningIzzyFBO
// ---------------------------------------------------------------------------------
// before the destination FBO is destroyed, the owner of this object
// must call this function to destroy the resources owned by this object

void
v002RuttEtraPlugIn::ClearOwningIzzyFBO()
{
	izzyFrameBuffer = NULL;
}

// ---------------------------------------------------------------------------------
//	BindIzzyTexture [STATIC, INLINE]
// ---------------------------------------------------------------------------------
// binds an Isadora texture to the specified texture unit, normalizing the texture
// coordinates by scaling the Texture Matrix Mode scaling if necessary.

void
v002RuttEtraPlugIn::BindIzzyTexture(
	GLuint			inTextureUnit,
	IzzyTexture		inTexture)
{
	#if TARGET_OS_WIN32
	if (!mGLLFunctionsOK)
		return;
	#endif
	
	if (inTexture) {
	
		glActiveTexture(inTextureUnit);
		glEnable(inTexture->GetTextureTarget());
		glBindTexture(inTexture->GetTextureTarget(), inTexture->GetTextureName());

		// normalize texture coordinates if it is GL_TEXTURE_RECTANGLE_ARB
		if (inTexture->GetTextureTarget() == GL_TEXTURE_RECTANGLE_ARB) {
			glMatrixMode(GL_TEXTURE);
			glScalef(GLfloat(1.0 / float(inTexture->GetTextureVisibleWidth())), GLfloat(1.0 / float(inTexture->GetTextureVisibleHeight())), 1.0);
			glMatrixMode(GL_MODELVIEW);
		}
	}
}

// ---------------------------------------------------------------------------------
//	UnbindIzzyTexture [STATIC, INLINE]
// ---------------------------------------------------------------------------------
// unbinds an Isadora texture ffrom the specified texture unit.

void
v002RuttEtraPlugIn::UnbindIzzyTexture(
	GLuint			inTextureUnit,
	IzzyTexture		inTexture)
{
	#if TARGET_OS_WIN32
	if (!mGLLFunctionsOK)
		return;
	#endif

	if (inTexture) {
		glActiveTexture(inTextureUnit);
		glDisable(inTexture->GetTextureTarget());
		glBindTexture(inTexture->GetTextureTarget(), 0);

	// normalize texture coordinates if it is GL_TEXTURE_RECTANGLE_ARB
		if (inTexture->GetTextureTarget() == GL_TEXTURE_RECTANGLE_ARB) {
			glMatrixMode(GL_TEXTURE);
			glLoadIdentity();
			glMatrixMode(GL_MODELVIEW);
		}
	}
}

// ---------------------------------------------------------------------------------
//	SetPerspectiveMatrix
// ---------------------------------------------------------------------------------
// Isadora FBOs aren't set up for perspective rendering, so we must use
// glFrustrum to set that up here.

// ensure that M_PI is defined... windows does not define this
// unless you set a special switch
#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi */
#endif

// convenience function to convert degrees to radians
#ifndef DEG2RAD
#define	DEG2RAD(a)	( (a) * (M_PI  / 180.0) )
#endif

void
v002RuttEtraPlugIn::SetPerspectiveMatrix(
	GLuint	inHorzRes,
	GLuint	inVertRes)
{
	#if TARGET_OS_WIN32
	if (!mGLLFunctionsOK)
		return;
	#endif

	// set identify matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	float cameraViewPosZ = 0.0;
	float shapeSize = 5000.0;
	
	GLdouble zNear = -cameraViewPosZ - shapeSize * 0.5;
	GLdouble zFar = -cameraViewPosZ + shapeSize * 0.5;
	
	if (zNear < 0.01)
		zNear = 0.01;
	if (zFar < 1.0)
		zFar = 1.0;

	const GLdouble ratio = (float) inHorzRes / (float) inVertRes;
	const GLdouble radians = DEG2RAD(50.0) / 2; // half field of view degrees to radians
	const GLdouble wd2 = zNear * tan(radians);

	if (ratio >= 1.0) {
		glFrustum(-(ratio * wd2), +(ratio * wd2), -wd2, +wd2, zNear, zFar);
	} else {
		glFrustum(-wd2, +wd2, -(wd2 / ratio), +(wd2 / ratio), zNear, zFar);
	}

	glMatrixMode(GL_MODELVIEW);
}

// ---------------------------------------------------------------------------------
//	Execute
// ---------------------------------------------------------------------------------
bool
v002RuttEtraPlugIn::Execute(
	IzzyTexture			inMainImage,
	IzzyTexture			inDisplaceImage,
	IzzyTexture			inPointSpriteImage)
{
	#if TARGET_OS_WIN32
	if (!mGLLFunctionsOK)
		return false;
	#endif

	GLuint width = MAX(inputResolutionX, 10);
	GLuint height = MAX(inputResolutionY, 10);
	bool normals = inputCalculateNormals;
	GLuint drawType = inputDrawType;

	if (needBufferRecreate) {
        CreateGLResourcesInContext(width, height, drawType, normals);
		needBufferRecreate = false;
    }
    
	// the following call are not necessary with
	// the isadora FBO because it will already be
	// bound by the time this method is called

    // save GL state
	PushGLState();
	
	// Bind the FBO as the rendering destination
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboID);

	glViewport(0, 0, width, height);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, width, 0, height, -1, 1);

	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	IzzyTexture whichImage = (inDisplaceImage != NULL) ? inDisplaceImage : inMainImage;
	
    if (whichImage != NULL) {
	
        glDisable(GL_LIGHTING);
		glDisable(GL_BLEND);
 		
        // glClampColorARB(GL_CLAMP_VERTEX_COLOR_ARB, GL_FALSE);
        // glClampColorARB(GL_CLAMP_FRAGMENT_COLOR_ARB, GL_FALSE);
        // glClampColorARB(GL_CLAMP_READ_COLOR_ARB, GL_FALSE);
        
        // MRT
        if (normals) {
		
            GLenum buffers[] = {GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT, GL_COLOR_ATTACHMENT2_EXT};
            glDrawBuffers(3, buffers);
			
		} else  {
		
            GLenum buffers[] = {GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT};
            glDrawBuffers(2, buffers);
			
        }
		
		// BIND DISPLACE TEXTURE
		BindIzzyTexture(GL_TEXTURE0, whichImage);

		glTexParameterf(whichImage->GetTextureTarget(), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(whichImage->GetTextureTarget(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		CGLShader* shader;
		
		if (!normals) {
	
			shader = whichImage->GetTextureTarget() == GL_TEXTURE_RECTANGLE_ARB ?
				mMRTShader_2DRect :
				mMRTShader_2D;
			
			glUseProgramObjectARB(shader->GetShaderProgramObj());
			glUniform1iARB(shader->GetUniformLocation("tex0"), 0); // load tex0 sampler to texture unit 0
			glUniform2fARB(shader->GetUniformLocation("imageSize"), (GLfloat) width, (GLfloat) height);
			glUniform1fARB(shader->GetUniformLocation("extrude"), (GLfloat) inputZExtrude);
			glUniform1fARB(shader->GetUniformLocation("useRaw"), (GLfloat) inputDisplaceMode);
			
		} else {

			if (!inputHQNormals) {

				shader = whichImage->GetTextureTarget() == GL_TEXTURE_RECTANGLE_ARB ?
					mMRTShaderNormals_2DRect :
					mMRTShaderNormals_2D;
			
				glUseProgramObjectARB(shader->GetShaderProgramObj());
				glUniform1iARB(shader->GetUniformLocation("tex0"), 0); // load tex0 sampler to texture unit 0
				glUniform2fARB(shader->GetUniformLocation("imageSize"), (GLfloat) width, (GLfloat) height);
				glUniform1fARB(shader->GetUniformLocation("extrude"), (GLfloat) inputZExtrude);
				glUniform1fARB(shader->GetUniformLocation("coef"), (GLfloat) inputNormalSmoothingCoeff);
				glUniform1fARB(shader->GetUniformLocation("useRaw"), (GLfloat) inputDisplaceMode);

			} else {

				shader = whichImage->GetTextureTarget() == GL_TEXTURE_RECTANGLE_ARB ?
					mMRTShaderHQNormals_2DRect :
					mMRTShaderHQNormals_2D;

				glUseProgramObjectARB(shader->GetShaderProgramObj());
				glUniform1iARB(shader->GetUniformLocation("tex0"), 0); // load tex0 sampler to texture unit 0
				glUniform2fARB(shader->GetUniformLocation("imageSize"), (GLfloat) width, (GLfloat) height);
				glUniform1fARB(shader->GetUniformLocation("extrude"), (GLfloat) inputZExtrude);
				glUniform1fARB(shader->GetUniformLocation("coef"), (GLfloat) inputNormalSmoothingCoeff);
				glUniform1fARB(shader->GetUniformLocation("useRaw"), (GLfloat) inputDisplaceMode);
			
			}
			
		}
			
		GLfloat tex_coords[] =
		{
			1.0,1.0,
			0.0,1.0,
			0.0,0.0,
			1.0,0.0
		};

		GLfloat verts[] = 
		{
			(GLfloat) width, (GLfloat) height,
			0.0, (GLfloat) height,
			0.0, 0.0,
			(GLfloat) width, 0.0
		};

		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, tex_coords);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, verts );
		glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );

		glUseProgramObjectARB(NULL);
		
		UnbindIzzyTexture(GL_TEXTURE0, whichImage);
		
	} else {
	
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT);
	
	}
	
	// grab the verts
	if (vertexBufferID) {
		glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
		glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, vertexBufferID);
		glReadPixels(0, 0, width, height, GL_RGBA, GL_FLOAT, NULL);
	}

	// grab the texture coords
	if (textureBufferID) {
		glReadBuffer(GL_COLOR_ATTACHMENT1_EXT);
		glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, textureBufferID);
		glReadPixels(0, 0, width, height, GL_RGBA, GL_FLOAT, NULL);
	}

	// grab the normals if enabled
	if (normals)  {
		glReadBuffer(GL_COLOR_ATTACHMENT2_EXT);
		glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, normalBufferID);
		glReadPixels(0, 0, width, height, GL_RGBA, GL_FLOAT, NULL);
	}

    glReadBuffer(GL_NONE);
    glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);
    
    glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
    
	glMatrixMode(GL_TEXTURE);
	glPopMatrix();
    
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	PopGLState();
        
    // now draw the VAO properly
	PushGLState();
    
 	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	
	SetPerspectiveMatrix(izzyFrameBuffer->FBO_Width(), izzyFrameBuffer->FBO_Height());
	
	// calculate aspect ratio
		
	GLfloat aspect = float(inMainImage->GetTextureVisibleWidth()) / float(inMainImage->GetTextureVisibleHeight());

	glTranslated(0, 0, inputTranslationZ );

	glRotated(inputRotationX, 1.0, 0.0, 0.0);
	glRotated(inputRotationY, 0.0, 1.0, 0.0);
	glRotated(inputRotationZ, 0.0, 0.0, 1.0);

	// if displacing using luminance
	if (inputDisplaceMode == kDisplaceMode_Luminance)  {

		glTranslated(-(0.5 * aspect * inputScaleX) + inputTranslationX, -(0.5 * inputScaleY) + inputTranslationY, 0 );
		glScaled(aspect * inputScaleX, inputScaleY, inputScaleZ);

	// if displacing using raw RGB
	}  else {

		glTranslated(-(0.5 * inputScaleX) + inputTranslationX, -(0.5 * inputScaleY) + inputTranslationY, 0 );
		glScaled(inputScaleX, inputScaleY, inputScaleZ);
	
	}

	// BLEND MODE
	if (inputBlendMode == kBlendMode_Opaque) {					// was == 0 in original vade version
		glDisable(GL_BLEND);
	} else if (inputBlendMode == kBlendMode_Transparent)  {		// was == 1 in original vade version
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	} else if (inputBlendMode == kBlendMode_Additive) {			// was == 2 in original vade version
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	}

	// DEPTH MODE
	if (inputDepthMode == kDepthMode_None) {
	
		glDisable(GL_DEPTH_TEST);
		
	} else if (inputDepthMode == kDepthMode_ReadWrite) {

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);

	} else if (inputDepthMode == kDepthMode_ReadOnly) {

		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);

	}
	
	BindIzzyTexture(GL_TEXTURE0, inMainImage);
	
	// these lines commented out in original
	// glTexParameterf([image textureTarget], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	// glTexParameterf([image textureTarget], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
    if (inputUVGenMode) {
	
        glEnable(GL_TEXTURE_GEN_S);
        glEnable(GL_TEXTURE_GEN_T);
        
        GLenum uvGenMode;
        switch (inputUVGenMode)  {
            case 1:
                uvGenMode = GL_OBJECT_LINEAR;
                break;
            case 2:
                uvGenMode = GL_EYE_LINEAR;
                break;
            case 3:
                uvGenMode = GL_SPHERE_MAP;
                break;
            default:
                uvGenMode = GL_OBJECT_LINEAR;
                break;
        }
        
        glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, uvGenMode);
        glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, uvGenMode);
		
    } else {
	
        glDisable(GL_TEXTURE_GEN_S);
        glDisable(GL_TEXTURE_GEN_T);
    }
    
    if (inputAntialias) {
        glEnable(GL_LINE_SMOOTH);
        glEnable(GL_POINT_SMOOTH);
    } else {
        glDisable(GL_LINE_SMOOTH);	
        glDisable(GL_POINT_SMOOTH);
    }
    
    glEnable(GL_NORMALIZE);
    
	#ifdef _MSC_VER
		glBindVertexArray(vaoID);
	#else
		glBindVertexArrayAPPLE(vaoID);
	#endif
	
    GLfloat color[4];
    
    color[0] = GLfloat(((inputModulationColor >> 16) & 0xFF) / 255.0);
    color[1] = GLfloat(((inputModulationColor >> 8) & 0xFF) / 255.0);
    color[2] = GLfloat(((inputModulationColor >> 0) & 0xFF) / 255.0);
    color[3] = GLfloat(((inputModulationColor >> 24) & 0xFF) / 255.0);
	
    glColor4f(color[0], color[1], color[2], color[3]);
	
    if (inputEnableClipping) {
	
		planeEquation(-1.0, 0.0, inputMaxClip, 0.0, 1.0, inputMaxClip, 0.0, 0.0, inputMaxClip, mPlaneEQ1);
		planeEquation(0.0, 1.0, inputMinClip, -1.0, 0.0, inputMinClip, 0.0, 0.0, inputMinClip, mPlaneEQ2);

        glClipPlane(GL_CLIP_PLANE0, mPlaneEQ1);
        glEnable(GL_CLIP_PLANE0);

        glClipPlane(GL_CLIP_PLANE1, mPlaneEQ2);
        glEnable(GL_CLIP_PLANE1);
    }
    
    switch(drawType) {
        case kDrawType_Scanlines: // scanlines
        {	
            glLineWidth(inputWireFrameWidth);
            glDrawElements(GL_LINES, (width - 1) * (height - 1) * 2, GL_UNSIGNED_INT, 0);
            break;
        }
        case kDrawType_Points: // points
        {
            glPointSize(inputWireFrameWidth);

            float coefficients[3];

            if (inputAttenuatePoints) {
                coefficients[0] = float(inputConstantAttenuation);
                coefficients[1] = float(inputLinearAttenuation);
                coefficients[2] = float(inputQuadraticAttenuation);
            } else {
                coefficients[0] = 1;
                coefficients[1] = 0;
                coefficients[2] = 0;            
            }
			
			glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION, coefficients);
			
			if (inPointSpriteImage) {
			
				glActiveTexture(GL_TEXTURE1);
				glEnable(GL_POINT_SPRITE);
	
				BindIzzyTexture(GL_TEXTURE1, inPointSpriteImage);
				
				if (inPointSpriteImage->GetTextureTarget() == GL_TEXTURE_2D) {
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				}
				
				glTexEnvf(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
				
				// glPointParameterfv(GL_POINT_SIZE_MIN, &min);
				// glPointParameterfv(GL_POINT_SIZE_MAX, &max);
				// glPointParameterfv(GL_POINT_FADE_THRESHOLD_SIZE, &thresh);
				
//				if (inputBlendMode == 1) {
//					glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
//				} else {
//					glBlendFunc(GL_ZERO, GL_ONE);
//				}
                
				// write to depth only
//				glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
//				glDepthFunc(GL_);
//				glDepthMask(GL_TRUE);
//				glDrawElements(GL_POINTS, (width - 1) * (height - 1) * 2, GL_UNSIGNED_INT, 0);
//
//				// disable writing to depth, use depth test if its enabled
//				glDepthMask(GL_FALSE);
//				glDepthFunc(GL_LEQUAL);
//				glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			}
            
			glDrawElements(GL_POINTS, (width - 1) * (height - 1) * 2, GL_UNSIGNED_INT, 0);

			if (inPointSpriteImage) {
				glDisable(GL_POINT_SPRITE);
				glTexEnvf(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_FALSE);
				UnbindIzzyTexture(GL_TEXTURE1, inPointSpriteImage);
				glActiveTexture(GL_TEXTURE0);
			}

			break;
		}
		
        case kDrawType_SquareMesh: // square mesh
        {
            glDrawElements(GL_LINES, (width -1) * (height -1) * 6, GL_UNSIGNED_INT, 0);
            break;
        }
			
		case kDrawType_TriangleMesh: // triangle mesh
        {
            glLineWidth(inputWireFrameWidth);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDrawElements(GL_TRIANGLES, (width -1) * (height -1) * 6, GL_UNSIGNED_INT, 0);
            break;
        }
			
		case kDrawType_SolidPlane: // solid plane
        {	
            glDrawElements(GL_TRIANGLES, (width -1) * (height -1) * 6, GL_UNSIGNED_INT, 0);
            break;
        }
			
		default:
            break;
    }
    
	#ifdef _MSC_VER
		glBindVertexArray(0);
	#else
		glBindVertexArrayAPPLE(0);
	#endif

	if (inputEnableClipping) {
		glDisable(GL_CLIP_PLANE0);
		glDisable(GL_CLIP_PLANE1);
	}

	UnbindIzzyTexture(GL_TEXTURE0, inMainImage);
	
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	PopGLState();

	return true;
}

// ---------------------------------------------------------------------------------
//	CreatePersistantGLResourcesInContext
// ---------------------------------------------------------------------------------
// Creates the invariant resources needed to execute the Rutt Etra plugin

void
v002RuttEtraPlugIn::CreatePersistantGLResourcesInContext()
{
	#if TARGET_OS_WIN32
	if (!mGLLFunctionsOK)
		return;
	#endif

	// load and compile the necessary shaders
	if (mMRTShader_2D == NULL) {
		mMRTShader_2D = new CGLShader(izzyFrameBuffer->mFBOGLFunctions);
		if (!mMRTShader_2D->CreateShader(RuttEtraMRT_vert, RuttEtraMRT_2D_frag)) {
			printf("****** RuttEtraMRT_2D Shader Has Errors! *****\n");
			printf("%s\n", mMRTShader_2D->GetErrors().c_str());
		}
	}
	
	if (mMRTShader_2DRect == NULL) {
		mMRTShader_2DRect = new CGLShader(izzyFrameBuffer->mFBOGLFunctions);
		if (!mMRTShader_2DRect->CreateShader(RuttEtraMRT_vert, RuttEtraMRT_2DRect_frag)) {
			printf("****** RuttEtraMRT_2DRect Shader Has Errors! *****\n");
			printf("%s\n", mMRTShader_2D->GetErrors().c_str());
		}
	}
	
	if (mMRTShaderNormals_2D == NULL) {
		mMRTShaderNormals_2D = new CGLShader(izzyFrameBuffer->mFBOGLFunctions);
		if (!mMRTShaderNormals_2D->CreateShader(RuttEtraMRTNormals_vert, RuttEtraMRTNormals_2D_frag)) {
			printf("****** RuttEtraMRTNormals_2D Shader Has Errors! *****\n");
			printf("%s\n", mMRTShader_2D->GetErrors().c_str());
		}
	}
	
	if (mMRTShaderNormals_2DRect == NULL) {
		mMRTShaderNormals_2DRect = new CGLShader(izzyFrameBuffer->mFBOGLFunctions);
		if (!mMRTShaderNormals_2DRect->CreateShader(RuttEtraMRTNormals_vert, RuttEtraMRTNormals_2DRect_frag)) {
			printf("****** RuttEtraMRTNormals_2DRect Shader Has Errors! *****\n");
			printf("%s\n", mMRTShaderNormals_2DRect->GetErrors().c_str());
		}
	}
	
	if (mMRTShaderHQNormals_2D == NULL) {
		mMRTShaderHQNormals_2D = new CGLShader(izzyFrameBuffer->mFBOGLFunctions);;
		if (!mMRTShaderHQNormals_2D->CreateShader(RuttEtraMRTHQNormals_vert, RuttEtraMRTHQNormals_2D_frag)) {
			printf("****** RuttEtraMRTHQNormals_2D Shader Has Errors! *****\n");
			printf("%s\n", mMRTShaderHQNormals_2D->GetErrors().c_str());
		}
	}

	if (mMRTShaderHQNormals_2DRect == NULL) {
		mMRTShaderHQNormals_2DRect = new CGLShader(izzyFrameBuffer->mFBOGLFunctions);;
		if (!mMRTShaderHQNormals_2DRect->CreateShader(RuttEtraMRTHQNormals_vert, RuttEtraMRTHQNormals_2DRect_frag)) {
			printf("****** RuttEtraMRTHQNormals_2DRect Shader Has Errors! *****\n");
			printf("%s\n", mMRTShaderHQNormals_2DRect->GetErrors().c_str());
		}
	}
}

// ---------------------------------------------------------------------------------
//	DestroyPersistantGLResourcesInContext
// ---------------------------------------------------------------------------------
// Releases the invariant resources needed to execute the Rutt Etra plugin

void
v002RuttEtraPlugIn::DestroyPersistantGLResourcesInContext()
{
	delete mMRTShader_2D;
	mMRTShader_2D = NULL;

	delete mMRTShader_2DRect;
	mMRTShader_2DRect = NULL;

	delete mMRTShaderNormals_2D;
	mMRTShaderNormals_2D = NULL;

	delete mMRTShaderNormals_2DRect;
	mMRTShaderNormals_2DRect = NULL;

	delete mMRTShaderHQNormals_2D;
	mMRTShaderHQNormals_2D = NULL;

	delete mMRTShaderHQNormals_2DRect;
	mMRTShaderHQNormals_2DRect = NULL;
}

// ---------------------------------------------------------------------------------
//	CreateGLResourcesInContext
// ---------------------------------------------------------------------------------
// Creates the resources needed to execute the Rutt Etra plugin.
//
// These resources need to be recrated whenver the dimensions,
// the draw type, or the normal flag are changed.

void
v002RuttEtraPlugIn::CreateGLResourcesInContext(
	GLuint			w,
	GLuint			h,
	GLuint			drawType,
	bool			normals)
{
	#if TARGET_OS_WIN32
	if (!mGLLFunctionsOK)
		return;
	#endif

	DestroyGLResourcesInContext();

	PushGLState();

	// create our FBO, and texture attachments
	PluginAssert_(ip, fboID == 0);
	glGenFramebuffers(1, & fboID);
	PluginAssert_(ip, fboID != 0);

	PluginAssert_(ip, texureAttachmentVertices == 0);
	glGenTextures(1, &texureAttachmentVertices);
	PluginAssert_(ip, texureAttachmentVertices != 0);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texureAttachmentVertices);
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F_ARB, w, h, 0, GL_RGBA, GL_FLOAT, NULL);

	PluginAssert_(ip, texureAttachmentTextureCoords == 0);
	glGenTextures(1, &texureAttachmentTextureCoords);
	PluginAssert_(ip, texureAttachmentTextureCoords != 0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texureAttachmentTextureCoords);
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F_ARB, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

	if (normals) {
		PluginAssert_(ip, texureAttachmentNormals == 0);
		glGenTextures(1, &texureAttachmentNormals);
		PluginAssert_(ip, texureAttachmentNormals != 0);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texureAttachmentNormals);
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F_ARB, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	}
	
	// associate our textures to the renderable attachments of the FBO
	glBindFramebuffer(GL_FRAMEBUFFER, fboID);
	glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, texureAttachmentVertices, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT, GL_TEXTURE_RECTANGLE_ARB, texureAttachmentTextureCoords, 0);
	
	if (normals) {
		glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT2_EXT, GL_TEXTURE_RECTANGLE_ARB, texureAttachmentNormals, 0);
	}
	// create our buffers
	PluginAssert_(ip, vertexBufferID == 0);
	glGenBuffers(1, &vertexBufferID);
	PluginAssert_(ip, vertexBufferID != 0);
	
	if (vertexBufferID) {
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glBufferData(GL_ARRAY_BUFFER, w * h * 4 * sizeof(GLfloat), NULL, GL_STREAM_COPY);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	PluginAssert_(ip, textureBufferID == 0);
	glGenBuffers(1, &textureBufferID);
	PluginAssert_(ip, textureBufferID != 0);
	
	if (textureBufferID) {
		glBindBuffer(GL_ARRAY_BUFFER, textureBufferID);
		glBufferData(GL_ARRAY_BUFFER, w * h * 4 * sizeof(GLfloat), NULL, GL_STREAM_COPY);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	if (normals) {
		PluginAssert_(ip, normalBufferID == 0);
		glGenBuffers(1, &normalBufferID);
		PluginAssert_(ip, normalBufferID != 0);
		glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
		glBufferData(GL_ARRAY_BUFFER, w * h * 4 * sizeof(GLfloat), NULL, GL_STREAM_COPY);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	PluginAssert_(ip, indexBufferID == 0);
	glGenBuffers(1, &indexBufferID);
	PluginAssert_(ip, indexBufferID != 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
	
	// scanlines or points
	if ((drawType == 0) || (drawType == 1)) {
	
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (w - 1) * (h -1) * 2 * sizeof(GLuint), NULL, GL_STATIC_DRAW);
		GLuint* indices = (GLuint*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY_ARB);

		GLuint i = 0, x, y;
		for (y = 0; y < h - 1 ; y++) {
			for (x = 0; x < w - 1 && w > 2; x++) {
				// this little aparatus makes sure we do not draw a line segment between different rows of scanline.
				if (i % (w - 2) <= (w - 1)) {
					indices[i + 0] = x + y * w;
					indices[i + 1] = x + y * w + 1;
				} 
				i+= 2;
			} 	  
		}

		glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	/*
	
	// these two blocks were commented out by vade
	
	} else if (drawType == 1) {
		
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (w - 1) * (h -1) * 2 * sizeof(GLuint), NULL, GL_STATIC_DRAW);
		GLuint* indices = (GLuint*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY_ARB);        

		GLuint i = 0, x, y;
		for ( y = 0; y < h - 1; y++) {
			for (x = 0; x < w -1 ; x++) {
				indices[i + 0] = x + y * w;
				indices[i + 1] = x + y * w + w + 1;
				i += 2;
			}
		}
		glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	 
	} else if (drawType == 2) {
			
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (w - 1) * (h -1) * 2 * sizeof(GLuint), NULL, GL_STATIC_DRAW);
		GLuint* indices = (GLuint*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY_ARB);        

		GLuint i = 0, x, y;

		for ( y = 0; y < h - 1; y++) {
			for (x = 0; x < w -1 ; x++) {
				indices[i + 0] = x + y * w + 1;
				indices[i + 1] = x + y * w + w;
				i += 2;
			}
		}
 
		glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	*/
	
	} else {
		
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (w - 1) * (h - 1) * 6 * sizeof(GLuint), NULL, GL_STATIC_DRAW);
		GLuint* indices = (GLuint*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY_ARB);

		GLuint i = 0, x, y;
		
		for (y = 0; y < h - 1; y++) {
			for (x = 0; x < w - 1 ; x++) {
				indices[i + 0] = x + y * w;
				indices[i + 1] = x + y * w + w;
				indices[i + 2] = x + y * w + 1;
				indices[i + 3] = x + y * w + 1;
				indices[i + 4] = x + y * w + w;
				indices[i + 5] = x + y * w + w + 1;
				i += 6;
			}
		}		

		glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	// create our VAO which encapsulates all client state
	// and buffer binding in one object. this saves us
	// state validation, and calls to GL.
	
	#ifdef _MSC_VER
		PluginAssert_(ip, vaoID == 0);
		glGenVertexArrays(1, &vaoID);
		PluginAssert_(ip, vaoID != 0);
		glBindVertexArray(vaoID);
	#else
		glGenVertexArraysAPPLE(1, &vaoID);
		glBindVertexArrayAPPLE(vaoID);
	#endif

	if (normals) {
		glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, sizeof(GLfloat) *4, NULL);
	}

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER, textureBufferID);
	glTexCoordPointer(4, GL_FLOAT, sizeof(GLfloat) * 4, 0);

	glEnableClientState(GL_VERTEX_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
	glVertexPointer( 4, GL_FLOAT, 4 * sizeof(float), 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);

	#ifdef _MSC_VER
		glBindVertexArray(0);
	#else
		glBindVertexArrayAPPLE(0);
	#endif

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	PopGLState();
}

// ---------------------------------------------------------------------------------
//	DestroyGLResourcesInContext
// ---------------------------------------------------------------------------------
// Releases the resources needed to execute the Rutt Etra plugin

void
v002RuttEtraPlugIn::DestroyGLResourcesInContext()
{
	#if TARGET_OS_WIN32
	if (!mGLLFunctionsOK)
		return;
	#endif

	if (fboID) {
		glDeleteFramebuffers(1, &fboID);
		fboID = 0;
	}

	if (texureAttachmentVertices) {
		glDeleteTextures(1, &texureAttachmentVertices);
		texureAttachmentVertices = 0;
	}

	if (texureAttachmentNormals) {
		glDeleteTextures(1, &texureAttachmentNormals);
		texureAttachmentNormals = 0;
	}

	if (texureAttachmentTextureCoords) {
		glDeleteTextures(1, &texureAttachmentTextureCoords);
		texureAttachmentTextureCoords = 0;
	}

	if (vertexBufferID) {
		glDeleteBuffers(1, &vertexBufferID);
		vertexBufferID = 0;
	}

	if (normalBufferID) {
		glDeleteBuffers(1, &normalBufferID);
		normalBufferID = 0;
	}

	if (textureBufferID) {
		glDeleteBuffers(1, &textureBufferID);
		textureBufferID = 0;
	}

	if (indexBufferID) {
		glDeleteBuffers(1, &indexBufferID);
		indexBufferID = 0;
	}

	if (vaoID)  {
		#ifdef _MSC_VER
			glDeleteVertexArrays(1, &vaoID);
		#else
			glDeleteVertexArraysAPPLE(1, &vaoID);
		#endif
		vaoID = 0;
	}

	needBufferRecreate = true;
}

void
v002RuttEtraPlugIn::PushGLState()
{
	#if TARGET_OS_WIN32
	if (!mGLLFunctionsOK)
		return;
	#endif

	glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
    
	glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &previousFBO);
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING_EXT, &previousReadFBO);
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING_EXT, &previousDrawFBO);
	glGetIntegerv(GL_CURRENT_PROGRAM, &previousShader);
    glGetIntegerv(GL_READ_BUFFER, &previousReadBuffer);
    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &previousPixelPackBuffer);
}

void
v002RuttEtraPlugIn::PopGLState()
{
	#if TARGET_OS_WIN32
	if (!mGLLFunctionsOK)
		return;
	#endif

	glUseProgramObjectARB((GLhandleARB) previousShader);
	glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, previousDrawFBO);
	glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, previousReadFBO);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, previousFBO);
    
    glReadBuffer(previousReadBuffer);
    glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, previousPixelPackBuffer);

    glPopClientAttrib();
	glPopAttrib();
	
	previousShader = 0;
	previousDrawFBO = 0;
	previousReadFBO = 0;
	previousFBO = 0;
	previousReadBuffer = 0;
	previousPixelPackBuffer = 0;
	
}

