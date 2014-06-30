// Note that the following includes must be defined in order:
#include "Constants.hpp"
#include "GL\glew.h"
#include "GLFW\glfw3.h"

// Note the the following Includes do not need to be defined in order:
#include <cstdio>
#include <map>
#include <list>
#define GLM_SWIZZLE		// Enable GLM Swizzling, must be before glm is included!
#include "glm\glm.hpp"  // Core GLM stuff, same as GLSL math.
#include "glm\ext.hpp"	// GLM extensions.


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

typedef Window*		WindowHandle;

unsigned int				g_uiWindowCounter = 0;

std::list<WindowHandle>		g_lWindows;
WindowHandle				g_hCurrentContext = nullptr;


std::map<unsigned int, unsigned int>	g_mVAOs;
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


int Init();
int MainLoop();
int ShutDown();

void GLFWErrorCallback(int a_iError, const char* a_szDiscription);
void GLFWWindowSizeCallback(GLFWwindow* a_pWindow, int a_iWidth, int a_iHeight);
GLEWContext* glewGetContext();   // This needs to be defined for GLEW MX to work, along with the GLEW_MX define in the perprocessor!
void MakeContextCurrent(WindowHandle a_hWindowHandle);

WindowHandle  CreateWindow(int a_iWidth, int a_iHeight, const std::string& a_szTitle, GLFWmonitor* a_pMonitor, WindowHandle a_hShare);

bool ShouldClose();
void CheckForGLErrors(std::string a_szMessage);

int main()
{
	int iReturnCode =EC_NO_ERROR;

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
	WindowHandle hPrimaryWindow = CreateWindow(c_iDefaultScreenWidth, c_iDefaultScreenHeight, "First", nullptr, nullptr);
	
	if (hPrimaryWindow < 0)
	{
		glfwTerminate();
		return EC_GLFW_FIRST_WINDOW_CREATION_FAIL;
	}

	// Print out GLFW, OpenGL version and GLEW Version:
	int iOpenGLMajor = glfwGetWindowAttrib(hPrimaryWindow->m_pWindow, GLFW_CONTEXT_VERSION_MAJOR);
	int iOpenGLMinor = glfwGetWindowAttrib(hPrimaryWindow->m_pWindow, GLFW_CONTEXT_VERSION_MINOR);
	int iOpenGLRevision = glfwGetWindowAttrib(hPrimaryWindow->m_pWindow, GLFW_CONTEXT_REVISION);
	printf("Status: Using GLFW Version %s\n", glfwGetVersionString());
	printf("Status: Using OpenGL Version: %i.%i, Revision: %i\n", iOpenGLMajor, iOpenGLMinor, iOpenGLRevision);
	printf("Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

	// create our second window:
	CreateWindow(c_iDefaultScreenWidth, c_iDefaultScreenHeight, "second", nullptr, hPrimaryWindow);
	
	MakeContextCurrent(hPrimaryWindow);

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

	CheckForGLErrors("Shader Setup Error");

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

	glGenTextures( 1, &g_Texture );
	glBindTexture( GL_TEXTURE_2D, g_Texture );
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 256, 256, 0, GL_RGBA, GL_FLOAT, texData);

	CheckForGLErrors("Texture Generation Error");

	// specify default filtering and wrapping
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

	// set the texture to use slot 0 in the shader
	GLuint texUniformID = glGetUniformLocation(g_Shader,"diffuseTexture");
	glUniform1i(texUniformID,0);

	CheckForGLErrors("Texture Loading Error");

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
	for (auto window : g_lWindows)
	{
		MakeContextCurrent(window);
		
		// Setup VAO:
		g_mVAOs[window->m_uiID] = 0;
		glGenVertexArrays(1, &(g_mVAOs[window->m_uiID]));
		glBindVertexArray(g_mVAOs[window->m_uiID]);
		glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_IBO);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), ((char*)0) + 16);
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), ((char*)0) + 24);

		CheckForGLErrors("Creating VAO Error");

		// Setup Matrix:
		window->m_m4Projection = glm::perspective(45.0f, float(window->m_uiWidth)/float(window->m_uiHeight), 0.1f, 1000.0f);
		window->m_m4ViewMatrix = glm::lookAt(glm::vec3(window->m_uiID * 8,8,8), glm::vec3(0,0,0), glm::vec3(0,1,0));

		// set OpenGL Options:
		glViewport(0, 0, window->m_uiWidth, window->m_uiHeight);
		glClearColor(0.25f,0.25f,0.25f,1);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);

		CheckForGLErrors("OpenGL Options Error");
	}

	return EC_NO_ERROR;
}


int MainLoop()
{
	while (!ShouldClose())
	{
		// Keep Running!
		// get delta time for this iteration:
		float fDeltaTime = (float)glfwGetTime();

		glm::mat4 identity;
		g_ModelMatrix = glm::rotate(identity, fDeltaTime * 10.0f, glm::vec3(0.0f, 1.0f, 0.0f));

		// draw each window in sequence:
		for (const auto& window : g_lWindows)
		{
			MakeContextCurrent(window);
		
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
			glBindVertexArray(g_mVAOs[window->m_uiID]);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

			glfwSwapBuffers(window->m_pWindow);  

			CheckForGLErrors("Render Error");
		}

		glfwPollEvents(); // process events!
	}

	return EC_NO_ERROR;
}


int ShutDown()
{
	// cleanup any remaining windows:
	for (auto& window :g_lWindows)
	{
		delete window->m_pGLEWContext;
		glfwDestroyWindow(window->m_pWindow);

		delete window;
	}

	// terminate GLFW:
	glfwTerminate();

	return EC_NO_ERROR;
}


void GLFWErrorCallback(int a_iError, const char* a_szDiscription)
{
	printf("GLFW Error occured, Error ID: %i, Description: %s\n", a_iError, a_szDiscription);
}


void GLFWWindowSizeCallback(GLFWwindow* a_pWindow, int a_iWidth, int a_iHeight)
{
	// find the window data corrosponding to a_pWindow;
	WindowHandle window = nullptr;
	for (auto& itr : g_lWindows)
	{
		if (itr->m_pWindow == a_pWindow)
		{
			window = itr;
			window->m_uiWidth = a_iWidth;
			window->m_uiHeight = a_iHeight;
			window->m_m4Projection = glm::perspective(45.0f, float(a_iWidth)/float(a_iHeight), 0.1f, 1000.0f);
		}
	}

	WindowHandle previousContext = g_hCurrentContext;
	MakeContextCurrent(window);
	glViewport(0, 0, a_iWidth, a_iHeight);
	MakeContextCurrent(previousContext);
}


GLEWContext* glewGetContext()
{
	return g_hCurrentContext->m_pGLEWContext;
}

WindowHandle CreateWindow(int a_iWidth, int a_iHeight, const std::string& a_szTitle, GLFWmonitor* a_pMonitor, WindowHandle a_hShare)
{
	// save current active context info so we can restore it later!
	WindowHandle hPreviousContext = g_hCurrentContext;

	// create new window data:
	WindowHandle newWindow = new Window();
	if (newWindow == nullptr)
		return nullptr;

	newWindow->m_pGLEWContext = nullptr;
	newWindow->m_pWindow = nullptr;
	newWindow->m_uiID = g_uiWindowCounter++;		// set ID and Increment Counter!
	newWindow->m_uiWidth = a_iWidth;
	newWindow->m_uiHeight = a_iHeight;

	// Create Window:
	if (a_hShare != nullptr) // Check that the Window Handle passed in is valid.
	{
		newWindow->m_pWindow = glfwCreateWindow(a_iWidth, a_iHeight, a_szTitle.c_str(), a_pMonitor, a_hShare->m_pWindow);  // Window handle is valid, Share its GL Context Data!
	}
	else
	{
		newWindow->m_pWindow = glfwCreateWindow(a_iWidth, a_iHeight, a_szTitle.c_str(), a_pMonitor, nullptr); // Window handle is invlad, do not share!
	}
	
	// Confirm window was created successfully:
	if (newWindow->m_pWindow == nullptr)
	{
		printf("Error: Could not Create GLFW Window!\n");
		delete newWindow;
		return nullptr;
	}

	// create GLEW Context:
	newWindow->m_pGLEWContext = new GLEWContext();
	if (newWindow->m_pGLEWContext == nullptr)
	{
		printf("Error: Could not create GLEW Context!\n");
		delete newWindow;
		return nullptr;
	}

	glfwMakeContextCurrent(newWindow->m_pWindow);   // Must be done before init of GLEW for this new windows Context!
	MakeContextCurrent(newWindow);					// and must be made current too :)
	
	// Init GLEW for this context:
	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		// a problem occured when trying to init glew, report it:
		printf("GLEW Error occured, Description: %s\n", glewGetErrorString(err));
		glfwDestroyWindow(newWindow->m_pWindow);
		delete newWindow;
		return nullptr;
	}
	
	// setup callbacks:
	// setup callback for window size changes:
	glfwSetWindowSizeCallback(newWindow->m_pWindow, GLFWWindowSizeCallback);

	// add new window to list:
	g_lWindows.push_back(newWindow);

	// now restore previous context:
	MakeContextCurrent(hPreviousContext);

	return newWindow;
}

void MakeContextCurrent(WindowHandle a_hWindowHandle)
{
	if (a_hWindowHandle != nullptr)
	{
		glfwMakeContextCurrent(a_hWindowHandle->m_pWindow);
		g_hCurrentContext = a_hWindowHandle;
	}
}

bool ShouldClose()
{
	if (g_lWindows.empty())
		return true;

	std::list<WindowHandle> lToDelete;
	for (const auto& window : g_lWindows)
	{
		if (glfwWindowShouldClose(window->m_pWindow))
		{
			lToDelete.push_back(window);
		}
	}

	if (!lToDelete.empty())
	{
		// we have windows to delete, Delete them:
		for (auto& window : lToDelete)
		{
			delete window->m_pGLEWContext;
			glfwDestroyWindow(window->m_pWindow);

			delete window;

			g_lWindows.remove(window);
		}
	}

	if (g_lWindows.empty())
		return true;

	return false;
}

void CheckForGLErrors(std::string a_szMessage)
{
	GLenum error = glGetError();
	while (error != GL_NO_ERROR)
	{
		printf("Error: %s, ErrorID: %i: %s\n", a_szMessage.c_str(), error, gluErrorString(error));
		error = glGetError(); // get next error if any.
	}
}