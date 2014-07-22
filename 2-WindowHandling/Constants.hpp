////////////////////////////////////////////////////////////
/// @file		Constants.hpp
/// @details	Header file containing Constants for the Input Tutorial.
/// @author		Greg Nott
/// @version	1.0 - Initial Version
/// @date		1/07/14
////////////////////////////////////////////////////////////

#ifndef _CONSTANTS_HPP_
#define _CONSTANTS_HPP_

#define GLM_SWIZZLE		// Enable GLM Swizzling, must be before glm is included!

enum ExitCodes
{
	EC_NO_ERROR = 0,
	EC_GLFW_INIT_FAIL = 1,
	EC_GLFW_FIRST_WINDOW_CREATION_FAIL = 2,
};


const int c_iDefaultScreenWidth = 1280;
const int c_iDefaultScreenHeight = 720;

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

#endif // _CONSTANTS_HPP_