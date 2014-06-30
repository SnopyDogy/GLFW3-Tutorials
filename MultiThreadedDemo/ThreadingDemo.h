////////////////////////////////////////////////////////////
/// @file		ThreadingDemo.h
/// @details	Header file containing Config info for the OGL Threading Demo!
/// @author		Greg Nott
/// @version	1.0 - Initial Version
/// @date		26/11/13
////////////////////////////////////////////////////////////

#ifndef _THREADINGDEMO_H_
#define _THREADINGDEMO_H_

#include "glm\glm.hpp"

////////////////////////// Constants //////////////////////////////////
const int c_iDefaultScreenWidth = 1280;
const int c_iDefaultScreenHeight = 720;

const char *c_szDefaultPrimaryWindowTitle = "Threading Demo - Primary Window";
const char *c_szDefaultSecondaryWindowTitle = "Threading Demo - Secondary Window";


///////////////////// Custom Data Types ///////////////////////////////
enum ExitCodes
{
	EC_NO_ERROR = 0,
	EC_GLFW_INIT_FAIL = 1,
	EC_GLFW_FIRST_WINDOW_CREATION_FAIL = 2,
};

struct Window
{
	GLFWwindow*		m_pWindow;
	GLEWContext*	m_pGLEWContext;
	unsigned int	m_uiWidth;
	unsigned int	m_uiHeight;
	glm::mat4		m_m4Projection;
	glm::mat4		m_m4ViewMatrix;

	unsigned int	m_uiID;
};
typedef Window* WindowHandle;

struct FPSData
{
	float			m_fFPS;
	float			m_fFrameCount;
	float			m_fTimeBetweenChecks;
	float			m_fTimeElapsed;
	// temp vars for calcing delta time:
	float			m_fCurrnetRunTime;
	float			m_fPreviousRunTime;
};

struct Vertex
{
	glm::vec4 m_v4Position;
	glm::vec2 m_v2UV;
	glm::vec4 m_v4Colour;
};

struct Quad
{
	static const unsigned int	c_uiNoOfIndicies = 6;
	static const unsigned int	c_uiNoOfVerticies = 4;
	Vertex						m_Verticies[c_uiNoOfVerticies];
	unsigned int				m_uiIndicies[c_uiNoOfIndicies];
};


/////////////////////////// Shaders ///////////////////////////////////
const char *c_szVertexShader = "#version 330\n"
	"in vec4 Position;\n"
	"in vec2 UV;\n"
	"in vec4 Colour;\n"
	"out vec2 vUV;\n"
	"out vec4 vColour;\n"
	"uniform mat4 Projection;\n"
	"uniform mat4 View;\n"
	"uniform mat4 Model;\n"
	"void main()\n"
	"{\n" 
		"vUV = UV;\n"
		"vColour = Colour;"
		"gl_Position = Projection * View * Model * Position;\n"
	"}\n"
	"\n";

const char *c_szPixelShader = "#version 330\n"
	"in vec2 vUV;\n"
	"in vec4 vColour;\n"
	"out vec4 outColour;\n"
	"uniform sampler2D diffuseTexture;\n"
	"void main()\n"
	"{\n"
		"outColour = texture2D(diffuseTexture, vUV) + vColour;\n"
	"}\n"
	"\n";

#endif // _THREADINGDEMO_H_
