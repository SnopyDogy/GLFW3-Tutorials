// Note that the following includes must be defined in order:
#include "Constants.hpp"
#include "Window.h"
#include "GL\glew.h"
#include "GLFW\glfw3.h"

// Note the the following Includes do not need to be defined in order:
#include <cstdio>
#include <map>
#include <list>
#include "glm\glm.hpp"  // Core GLM stuff, same as GLSL math.
#include "glm\ext.hpp"	// GLM extensions.

//////////////////////// global Vars //////////////////////////////

std::map<int, unsigned int>	g_mVAOs;
unsigned int g_VBO = 0;
unsigned int g_IBO = 0;
unsigned int g_Texture = 0;
unsigned int g_Shader = 0;
glm::mat4	 g_ModelMatrix;

struct Vertex
{
	glm::vec4 m_v4Position;
	glm::vec2 m_v2UV;
	glm::vec4 m_v4Colour;
};

//////////////////////// Function Declerations //////////////////////////////

int Init();
int MainLoop();
int ShutDown();

void GLFWErrorCallback(int a_iError, const char* a_szDiscription);
bool ShouldClose();

//////////////////////// Function Definitions //////////////////////////////
int main()
{
	int iReturnCode = EC_NO_ERROR;

	iReturnCode = Init();
	if (iReturnCode != EC_NO_ERROR)
		return iReturnCode;

	iReturnCode = MainLoop();
	if (iReturnCode != EC_NO_ERROR)
		return iReturnCode;

	iReturnCode = ShutDown();
	if (iReturnCode != EC_NO_ERROR)
		return iReturnCode;

	return iReturnCode;
}


int Init()
{
	// Setup Our GLFW error callback, we do this before Init so we know what goes wrong with init if it fails:
	glfwSetErrorCallback(GLFWErrorCallback);

	// Init GLFW:
	if (!glfwInit())
		return EC_GLFW_INIT_FAIL;

	// create our first window:
	WindowHandle hPrimaryWindow = new Window(c_iDefaultScreenWidth, c_iDefaultScreenHeight, "First");
	
	if (hPrimaryWindow == nullptr)
	{
		glfwTerminate();
		return EC_GLFW_FIRST_WINDOW_CREATION_FAIL;
	}

	// Print out GLFW, OpenGL version and GLEW Version:
	int iOpenGLMajor = glfwGetWindowAttrib(hPrimaryWindow->GLFWWindowHandle(), GLFW_CONTEXT_VERSION_MAJOR);
	int iOpenGLMinor = glfwGetWindowAttrib(hPrimaryWindow->GLFWWindowHandle(), GLFW_CONTEXT_VERSION_MINOR);
	int iOpenGLRevision = glfwGetWindowAttrib(hPrimaryWindow->GLFWWindowHandle(), GLFW_CONTEXT_REVISION);
	printf("Status: Using GLFW Version %s\n", glfwGetVersionString());
	printf("Status: Using OpenGL Version: %i.%i, Revision: %i\n", iOpenGLMajor, iOpenGLMinor, iOpenGLRevision);
	printf("Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

	// create our second window:
	new Window(c_iDefaultScreenWidth, c_iDefaultScreenHeight, "Second", nullptr, hPrimaryWindow);
	hPrimaryWindow->MakeCurrent();

	// create shader:
	GLint iSuccess = 0;
	GLchar acLog[256];
	GLuint vsHandle = glCreateShader(GL_VERTEX_SHADER);
	GLuint fsHandle = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(vsHandle, 1, (const char**)&c_szVertexShader, 0);
	glCompileShader(vsHandle);
	glGetShaderiv(vsHandle, GL_COMPILE_STATUS, &iSuccess);
	glGetShaderInfoLog(vsHandle, sizeof(acLog), 0, acLog);
	if (iSuccess == GL_FALSE)
	{
		printf("Error: Failed to compile vertex shader!\n");
		printf(acLog);
		printf("\n");
	}

	glShaderSource(fsHandle, 1, (const char**)&c_szPixelShader, 0);
	glCompileShader(fsHandle);
	glGetShaderiv(fsHandle, GL_COMPILE_STATUS, &iSuccess);
	glGetShaderInfoLog(fsHandle, sizeof(acLog), 0, acLog);
	if (iSuccess == GL_FALSE)
	{
		printf("Error: Failed to compile fragment shader!\n");
		printf(acLog);
		printf("\n");
	}

	g_Shader = glCreateProgram();
	glAttachShader(g_Shader, vsHandle);
	glAttachShader(g_Shader, fsHandle);
	glDeleteShader(vsHandle);
	glDeleteShader(fsHandle);

	// specify Vertex Attribs:
	glBindAttribLocation(g_Shader, 0, "Position");
	glBindAttribLocation(g_Shader, 1, "UV");
	glBindAttribLocation(g_Shader, 2, "Colour");
	glBindFragDataLocation(g_Shader, 0, "outColour");

	glLinkProgram(g_Shader);
	glGetProgramiv(g_Shader, GL_LINK_STATUS, &iSuccess);
	glGetProgramInfoLog(g_Shader, sizeof(acLog), 0, acLog);
	if (iSuccess == GL_FALSE)
	{
		printf("Error: failed to link Shader Program!\n");
		printf(acLog);
		printf("\n");
	}

	glUseProgram(g_Shader);

	// create and load a texture:
	glm::vec4 *texData = new glm::vec4[256 * 256];
	for (int i = 0; i < 256 * 256; i += 256)
	{
		for (int j = 0; j < 256; ++j)
		{
			if (j % 2 == 0)
			{
				texData[i + j] = glm::vec4(0, 0, 0, 1);
			}
			else
			{
				texData[i + j] = glm::vec4(1, 1, 1, 1);
			}
		}
	}

	// Create texture without mipmaps:
	if (GLEW_ARB_texture_storage)
	{
		glGenTextures(1, &g_Texture);
		glBindTexture(GL_TEXTURE_2D, g_Texture);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 256, 256);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 256, GL_RGBA, GL_FLOAT, texData);
	}
	else
	{
		glGenTextures(1, &g_Texture);
		glBindTexture(GL_TEXTURE_2D, g_Texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 256, 256, 0, GL_RGBA, GL_FLOAT, texData);
	}

	// specify default filtering and wrapping
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

	// set the texture to use slot 0 in the shader
	GLuint texUniformID = glGetUniformLocation(g_Shader,"diffuseTexture");
	glUniform1i(texUniformID,0);

	// cleanup Texture Data:
	delete[] texData;
	texData = nullptr; 

	// now create a quad:
	Vertex aoVertices[4];
	aoVertices[0].m_v4Position = glm::vec4(-2,0,-2,1);
	aoVertices[0].m_v2UV = glm::vec2(0,0);
	aoVertices[0].m_v4Colour.xyzw = 1.0f; // = glm::vec4(0,1,0,1);
	aoVertices[1].m_v4Position = glm::vec4(2,0,-2,1);
	aoVertices[1].m_v2UV = glm::vec2(1,0);
	aoVertices[1].m_v4Colour = glm::vec4(1,0,0,1);
	aoVertices[2].m_v4Position = glm::vec4(2,0,2,1);
	aoVertices[2].m_v2UV = glm::vec2(1,1);
	aoVertices[2].m_v4Colour = glm::vec4(0,1,0,1);
	aoVertices[3].m_v4Position = glm::vec4(-2,0,2,1);
	aoVertices[3].m_v2UV = glm::vec2(0,1);
	aoVertices[3].m_v4Colour = glm::vec4(0,0,1,1);

	unsigned int auiIndex[6] = {
		3,1,0,
		3,2,1 
	};

	// Create VBO/IBO
	glGenBuffers(1, &g_VBO);
	glGenBuffers(1, &g_IBO);
	glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_IBO);

	glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(Vertex), aoVertices, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(unsigned int), auiIndex, GL_STATIC_DRAW);

	// Now do window specific stuff, including:
	// --> Creating a VAO with the VBO/IBO created above!
	// --> Setting Up Projection and View Matricies!
	// --> Specifing OpenGL Options for the window!
	for (auto window : Window::ActiveWindows())
	{
		window->MakeCurrent();
		
		// Setup VAO:
		g_mVAOs[window->ID()] = 0;
		glGenVertexArrays(1, &(g_mVAOs[window->ID()]));
		glBindVertexArray(g_mVAOs[window->ID()]);
		glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_IBO);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), ((char*)0) + 16);
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), ((char*)0) + 24);

		// Setup Matrix:
		window->m_m4Projection = glm::perspective(45.0f, float(window->Width())/float(window->Height()), 0.1f, 1000.0f);
		window->m_m4ViewMatrix = glm::lookAt(glm::vec3(window->ID() * 8,8,8), glm::vec3(0,0,0), glm::vec3(0,1,0));

		// set OpenGL Options:
		//glViewport(0, 0, window->Width(), window->Height());
		glClearColor(0.25f,0.25f,0.25f,1);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
	}

	return EC_NO_ERROR;
}


int MainLoop()
{
	while (!ShouldClose())
	{
		// Keep Running!
		// get delta time for this iteration:
		//float fDeltaTime = (float)glfwGetTime();
		float fTime = (float)glfwGetTime();

		glm::mat4 identity;
		g_ModelMatrix = glm::rotate(identity, fTime, glm::vec3(0.0f, 1.0f, 0.0f));

		// draw each window in sequence:
		for (const auto& window : Window::ActiveWindows())
		{
			window->MakeCurrent();

			// clear the backbuffer to our clear colour and clear the depth buffer
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glUseProgram(g_Shader);

			GLuint ProjectionID = glGetUniformLocation(g_Shader,"Projection");
			GLuint ViewID = glGetUniformLocation(g_Shader,"View");
			GLuint ModelID = glGetUniformLocation(g_Shader,"Model");

			glUniformMatrix4fv(ProjectionID, 1, false, glm::value_ptr(window->m_m4Projection));
			glUniformMatrix4fv(ViewID, 1, false, glm::value_ptr(window->m_m4ViewMatrix));
			glUniformMatrix4fv(ModelID, 1, false, glm::value_ptr(g_ModelMatrix));

			glActiveTexture(GL_TEXTURE0);
			glBindTexture( GL_TEXTURE_2D, g_Texture );
			glBindVertexArray(g_mVAOs[window->ID()]);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

			window->SwapBuffers();
		}

		glfwPollEvents(); // process events! (must be called on the main thread)!!
	}

	return EC_NO_ERROR;
}


int ShutDown()
{
	// cleanup any remaining windows:
	while (Window::ActiveWindows().empty() == false)
	{
		delete Window::ActiveWindows()[0];
	}

	// terminate GLFW:
	glfwTerminate();

	return EC_NO_ERROR;
}


void GLFWErrorCallback(int a_iError, const char* a_szDiscription)
{
	printf("GLFW Error occured, Error ID: %i, Description: %s\n", a_iError, a_szDiscription);
}


bool ShouldClose()
{
	if (Window::ActiveWindows().empty())
		return true;

	std::list<WindowHandle> lToDelete;
	for (const auto& window : Window::ActiveWindows())
	{
		if (window->ShouldClose())
		{
			lToDelete.push_back(window);
		}
	}

	if (lToDelete.empty() == false)
	{
		// we have windows to close, Delete them:
		for (auto& window : lToDelete)
		{
			delete window;
		}
	}

	if (Window::ActiveWindows().empty())
		return true;

	return false;
}