// ===========================================================================
//
//	v002RuttEtraPlugIn
//
//	created by Vade
//	adapted for Isadora by Mark Coniglio
//
// ===========================================================================

//
//  v002_Rutt_Etra_2_0PlugIn.h
//  v002 Rutt Etra 2.0
//
//  Created by vade on 2/15/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#include <OpenGLIncludes.h>

#include "CGLShader.h"
#include "IzzyFrameBufferBase.h"
#include "IsadoraCallbacks.h"

// ---------------------------------------------------------------------------------
//	v002RuttEtraPlugIn [ENUMS]
// ---------------------------------------------------------------------------------

enum {
	kDrawType_Scanlines = 0,
	kDrawType_Points,
	kDrawType_SquareMesh,
	kDrawType_TriangleMesh,
	kDrawType_SolidPlane
};
	
enum {
	kDisplaceMode_Luminance = 0,
	kDisplaceMode_RawRGBtoYZ
};

enum {
	kDepthMode_None,
	kDepthMode_ReadWrite,
	kDepthMode_ReadOnly
};

// changed the blend modes used by Rutt Etra to match
// those commonly seen in Isadora's projector actor and
// others.
//
// the blend modes used now are those defined in IsadoraTypes.h:
//
// enum {
//    kBlendMode_Additive,
//    kBlendMode_Transparent,
//    kBlendMode_Opaque
// };

// values for these constants in the original vade version
// were as follows:
//
//    replace (izzy 'opaque') was 0
//    over (izzy 'transparent') was 1
//    add (izzy 'additive') was 2

// ---------------------------------------------------------------------------------
//	v002RuttEtraPlugIn [CLASS DEFINITION]
// ---------------------------------------------------------------------------------
class v002RuttEtraPlugIn
{
public:
			v002RuttEtraPlugIn(
				IsadoraParameters*	inIzzyParams);

			~v002RuttEtraPlugIn();
	
	bool	RequiresContextToDelete() const;

// OWNING ISADORA FBO
	void			SetOwningIzzyFBO(
						IzzyFrameBuffer	inIzzyFBO);
	
	void			ClearOwningIzzyFBO();

	IzzyFrameBuffer	GetOwningIzzyFBO() const
						{ return izzyFrameBuffer; }
	
	void			DeleteAllGLResourcesForIzzyFrameBuffer(
						IzzyFrameBuffer	inCurrentIzzyFrameBuffer);

// ACCESSOR: DRAW TYPE
	void	SetDrawType(
				GLuint		inDrawType)
			{
				inputDrawType = inDrawType;
				needBufferRecreate = true;
			}

	GLuint	GetDrawType() const
				{ return inputDrawType; }
				
	// ACCESSOR: RESOLUTION
	void	GetResolution(
				GLuint*		outWidth,
				GLuint*		outHeight) const
			{
				*outWidth = inputResolutionX;
				*outHeight = inputResolutionY;
			}
	
	void	SetResolution(
				GLuint		inWidth,
				GLuint		inHeight)
			{
				if (inWidth != inputResolutionX || inHeight != inputResolutionY) {
					inputResolutionX = inWidth;
					inputResolutionY = inHeight;
					needBufferRecreate = true;
				}
			}
	
	// ACCESSOR: NORMAL CALCULATION SETTINGS
	void	SetCalculateNormalsFlag(
				bool	inCalculateNormalsFlag)
			{
				inputCalculateNormals = inCalculateNormalsFlag;
				needBufferRecreate = true;
			}

	void	SetNormalsSmoothingCoefficient(
				double	inNormalSmoothingCoeff)
			{
				inputNormalSmoothingCoeff = inNormalSmoothingCoeff;
				needBufferRecreate = true;
			}

	void	SetHighQualityNormalsFlag(
				bool	inHQNormalsFlag)
			{
				inputHQNormals = inHQNormalsFlag;
			}


	void	BindIzzyTexture(
				GLuint			inTextureUnit,
				IzzyTexture		inTexture);

	void	UnbindIzzyTexture(
				GLuint			inTextureUnit,
				IzzyTexture		inTexture);

	bool	Execute(
				IzzyTexture			inMainImage,
				IzzyTexture			inDisplaceImage,
				IzzyTexture			inPointSpringImage);
	
private:

	void	CreatePersistantGLResourcesInContext();
	
	void	DestroyPersistantGLResourcesInContext();
	
	void	SetPerspectiveMatrix(
				GLuint				inHorzRes,
				GLuint				inVertRes);
	
	void	CreateGLResourcesInContext(
				GLuint				w,
				GLuint				h,
				GLuint				drawType,
				bool				normals);
	
	void	DestroyGLResourcesInContext();
	
	void	PushGLState();
	
	void	PopGLState();

	
private:

	IsadoraParameters*	ip;
	IzzyFrameBuffer		izzyFrameBuffer;

// PERSISTENT RESOURCES
	CGLShader*		mMRTShader_2D;
	CGLShader*		mMRTShaderNormals_2D;
	CGLShader*		mMRTShaderHQNormals_2D;

	CGLShader*		mMRTShader_2DRect;
	CGLShader*		mMRTShaderNormals_2DRect;
	CGLShader*		mMRTShaderHQNormals_2DRect;

	double*		mPlaneEQ1;
	double*		mPlaneEQ2;
	
	GLuint		vaoID;								// the combined state vector
													// for all buffers for drawing
	// FBO and texture objects
	GLuint		fboID;								// for drawing our MRT targets
	GLuint		texureAttachmentVertices;			// vertices
	GLuint		texureAttachmentNormals;			// normals
	GLuint		texureAttachmentTextureCoords;		// texture coords

	// Buffer Objects
	GLuint		vertexBufferID;
	GLuint		textureBufferID;
	GLuint		normalBufferID;
	GLuint		indexBufferID;

	// GL state saving
	GLint		previousFBO;
	GLint		previousReadFBO;
	GLint		previousDrawFBO;
	GLint		previousReadBuffer;
	GLint		previousPixelPackBuffer;
	GLint		previousShader;

private:

	// these following inputs are private so the caller
	// must change them through accessor functions, which
	// set the needBufferRecreate flag as needed to ensure
	// the required resources are re-generated
	
	GLuint		inputResolutionX;			// requires generation of new resources
	GLuint		inputResolutionY;			// requires generation of new resources
	GLuint		inputDrawType;				// requires generation of new resources
	bool		inputCalculateNormals;		// requires generation of new resources
	bool		inputHQNormals;
	double		inputNormalSmoothingCoeff;
	
	bool		needBufferRecreate;

public:

	GLuint		inputModulationColor;
	bool		inputColorCorrect;

	// DRAWING MODES
	GLuint		inputDisplaceMode;
	GLuint		inputBlendMode; 
	GLuint		inputDepthMode;
	
	// DRAW SETTINGS
	float		inputZExtrude;
	float		inputWireFrameWidth;
	bool		inputAntialias;

	// POINT ATTENTUATION
	bool		inputAttenuatePoints;
	float		inputConstantAttenuation;
	float		inputLinearAttenuation;
	float		inputQuadraticAttenuation;

	// UV GENERATION MODE
	GLuint		inputUVGenMode;

	// ROTATION, TRANSLATION, SCALING
	double		inputTranslationX;
	double		inputTranslationY;
	double		inputTranslationZ;
	double		inputRotationX;
	double		inputRotationY;
	double		inputRotationZ;
	double		inputScaleX;
	double		inputScaleY;
	double		inputScaleZ;
	
	// CLIPPING PLANE
	bool		inputEnableClipping;
	float		inputMinClip;
	float		inputMaxClip;

#if TARGET_OS_WIN32

	CIZGLFunctions*					__ctx__accessor;
	bool							mGLLFunctionsOK;

	// MISC
	#define GL_PIXEL_PACK_BUFFER_BINDING	  0x88ED

	// VERTEX ARRAY OBJECT
	typedef void (APIENTRYP PFNGLBINDVERTEXARRAYPROC) (GLuint array);
	typedef void (APIENTRYP PFNGLGENVERTEXARRAYSPROC) (GLsizei n, GLuint *arrays);
	typedef void (APIENTRYP PFNGLDELETEVERTEXARRAYSPROC) (GLsizei n, const GLuint *arrays);

	PFNGLGENVERTEXARRAYSPROC		glGenVertexArrays;
	PFNGLDELETEVERTEXARRAYSPROC		glDeleteVertexArrays;
	PFNGLBINDVERTEXARRAYPROC		glBindVertexArray;

	// BUFFER DATA
	typedef void (APIENTRYP PFNGLBUFFERDATAPROC) (GLenum target, GLsizeiptr size, const void *data, GLenum usage);
	typedef void *(APIENTRYP PFNGLMAPBUFFERPROC) (GLenum target, GLenum access);
	typedef GLboolean (APIENTRYP PFNGLUNMAPBUFFERPROC) (GLenum target);

	PFNGLGENBUFFERSPROC				glGenBuffers;
	PFNGLDELETEBUFFERSPROC			glDeleteBuffers;
	PFNGLBINDBUFFERPROC				glBindBuffer;
	PFNGLDRAWBUFFERSPROC			glDrawBuffers;
	PFNGLBUFFERDATAPROC				glBufferData;
	PFNGLMAPBUFFERPROC				glMapBuffer;
	PFNGLUNMAPBUFFERPROC			glUnmapBuffer;

	// FRAMEBUFFER OBJECTS
	#define GL_FRAMEBUFFER			0x8D40
	#define GL_READ_FRAMEBUFFER_EXT           0x8CA8
	#define GL_DRAW_FRAMEBUFFER_EXT           0x8CA9
	#define GL_DRAW_FRAMEBUFFER_BINDING_EXT   0x8CA6
	#define GL_READ_FRAMEBUFFER_BINDING_EXT   0x8CAA

	typedef void (APIENTRYP PFNGLGENFRAMEBUFFERSPROC) (GLsizei n, GLuint *framebuffers);
	typedef void (APIENTRYP PFNGLDELETEFRAMEBUFFERSEXTPROC) (GLsizei n, const GLuint *framebuffers);
	typedef void (APIENTRYP PFNGLFRAMEBUFFERTEXTURE2DPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
	typedef void (APIENTRYP PFNGLBINDFRAMEBUFFERPROC) (GLenum target, GLuint framebuffer);

	PFNGLGENFRAMEBUFFERSPROC		glGenFramebuffers;
	PFNGLDELETEFRAMEBUFFERSEXTPROC	glDeleteFramebuffers;
	PFNGLFRAMEBUFFERTEXTURE2DPROC	glFramebufferTexture2D;
	PFNGLBINDFRAMEBUFFERPROC		glBindFramebuffer;

	// UNIFORM PARAMETERS
	PFNGLUNIFORM1IARBPROC			glUniform1iARB;
	PFNGLUNIFORM1FARBPROC			glUniform1fARB;
	PFNGLUNIFORM2FARBPROC			glUniform2fARB;

	// POINT PARAM
	typedef void (APIENTRYP PFNGLPOINTPARAMETERFVARBPROC) (GLenum pname, const GLfloat *params);

	PFNGLPOINTPARAMETERFVARBPROC	glPointParameterfv;

#endif

};
