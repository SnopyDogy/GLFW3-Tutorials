//Note: these must be defined in order:
#include "GL\glew.h"
#include "GLFW\glfw3.h"
#include "Window.h"
#include "glm\ext.hpp"
#include <algorithm>
#include <assert.h>
#ifdef _DEBUG
#include <string>
#include <cstdio>
#include <iostream>
#endif

// This will get the id of the apps main thread on program startuip (before main exacutes).
std::thread::id g_oMainThread = std::this_thread::get_id();

// Setup Static Window class members:
int Window::m_iWindowCounter = 0;			// the first window will have an ID of 0.
std::vector<Window*> Window::m_aWindows;

// declare our OpenGL callback function.
#ifdef _DEBUG
void APIENTRY GLErrorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, void*  userParam);
#endif

// Define the GLEW context callback.
GLEWContextStruct* glewGetContext()
{
	// get the current context from GLFW:
	auto pGLFWWindow = glfwGetCurrentContext();
	if (pGLFWWindow == nullptr)
		return nullptr;										// NOT NICE, GLEW will crash here!!

	// Use the User Pointer to get get our window class:
	WindowHandle pWindow = reinterpret_cast<Window*>(glfwGetWindowUserPointer(pGLFWWindow));

	// Return the windows GLEW Context:
	return	pWindow->GLEWContext();
}


//////////////////////// Window Constructors //////////////////////////////

Window::Window()
	: m_pWindow(nullptr)
	, m_pGLEWContext(nullptr)
	, m_iWidth(0)
	, m_iHeight(0)
	, m_szTitle("Invalid Window")
{

}


Window::Window(int a_iWidth, int a_iHeight, const std::string& a_szTitle, GLFWmonitor* a_pMonitor, Window* a_pShare, WindowHints* a_pWindowHints)
	: m_pWindow(nullptr)
	, m_pGLEWContext(nullptr)
	, m_iWidth(a_iWidth)
	, m_iHeight(a_iHeight)
	, m_szTitle(a_szTitle)
{
	// if this is the first window created we need to save our "main" thread id:
	if (CalledOnMainThread() == false)
	{
		// We need to check that we are on the main thread... 
		// just in case the user does the wrong thing...
		// For detials of which GLFW functions can only occure on the main thread see: 
		// http://www.glfw.org/docs/latest/group__window.html
		throw NotCalledOnMainThreadException(std::this_thread::get_id(), g_oMainThread);
	}

	// save current active context info so we can restore it later!
	WindowHandle hPreviousContext = Window::CurrentContext();

	// set ID and Increment Counter!
	m_iID = m_iWindowCounter++;		

	// Set Window Hints:
	SetGLFWWindowHints(a_pWindowHints);

	// Create Window:
	if (a_pShare != nullptr) // Check that the Window Handle passed in is valid.
		m_pWindow = glfwCreateWindow(a_iWidth, a_iHeight, a_szTitle.c_str(), a_pMonitor, a_pShare->m_pWindow);  // Window handle is valid, Share its GL Context Data!
	else
		m_pWindow = glfwCreateWindow(a_iWidth, a_iHeight, a_szTitle.c_str(), a_pMonitor, nullptr); // Window handle is invlad, do not share!

	// Confirm window was created successfully:
	if (m_pWindow == nullptr)
	{
		printf("Error: Could not Create GLFW Window!\n");
		return;
	}

	// set window user pointer to ourself
	// Must be done before making context current (else glewInit() fails).
	glfwSetWindowUserPointer(m_pWindow, static_cast<void*>(this));

	// create GLEW Context:
	m_pGLEWContext = new GLEWContextStruct();
	if (m_pGLEWContext == nullptr)
	{
		printf("Error: Could not create GLEW Context!\n");
		return;
	}

	MakeCurrent();			// The new OpenGL/GLEW context must be made current before GLEW can be initialised.

	// Init GLEW for this context:
	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		// a problem occured when trying to init glew, report it:
		printf("GLEW Error occured, Description: %s\n", glewGetErrorString(err));

		// and cleanup:
		delete m_pGLEWContext;
		m_pGLEWContext = nullptr;
		glfwDestroyWindow(m_pWindow);
		m_pWindow = nullptr;
		return;
	}

	// setup callbacks and user pointer:
	SetupCallbacks();

	// add new window to list:
	m_aWindows.push_back(this);

	// now restore previous context:
	MakeContextCurrent(hPreviousContext);
}


Window::Window(Window&& a_rOther)
	: m_pWindow(nullptr)
	, m_pGLEWContext(nullptr)
	, m_iWidth(0)
	, m_iHeight(0)
{
	// safty check, we can only call this on the Main Thread
	// due to the fact that we call glfwSetWindowUserPointer()
	if (CalledOnMainThread() == false)
		throw NotCalledOnMainThreadException(std::this_thread::get_id(), g_oMainThread);

	// now copy data for the rvalue:
	m_pWindow = a_rOther.m_pWindow;
	m_pGLEWContext = a_rOther.m_pGLEWContext;
	m_iWidth = a_rOther.m_iWidth;
	m_iHeight = a_rOther.m_iHeight;
	m_iID = a_rOther.m_iID;
	m_szTitle = a_rOther.m_szTitle;

	// we need to make sure we arew bound to setup the callbacks:
	auto hPrevContext = Window::CurrentContext();
	this->MakeCurrent();

	// update our glfw callbacks and user pointer:
	glfwSetWindowUserPointer(m_pWindow, static_cast<void*>(this));
	SetupCallbacks();

	// keep our window list up to date:
	for (auto& win : m_aWindows)
	{
		if (win->ID() == a_rOther.ID())
		{
			win = this;
		}
	}

	// nuter old widnow:
	a_rOther.m_pWindow = nullptr;
	a_rOther.m_pGLEWContext = nullptr;
	a_rOther.m_iID = -1;

	// re-set activew/bound context state for this thread:
	MakeContextCurrent(hPrevContext);
}


Window& Window::operator=(Window&& a_rOther)
{
	// safty check, we can only call this on the Main Thread
	// due to the fact that we call glfwSetWindowUserPointer()
	if (CalledOnMainThread() == false)
		throw NotCalledOnMainThreadException(std::this_thread::get_id(), g_oMainThread);

	// first check to see if this (i.e. the left hand side/lvalue) 
	// is valid:
	if (IsValid())
	{
		// if we are valid, first clean up:
		Destory();

		// also, print a warning as this is probably not right...
#ifdef _DEBUG
		printf("WARNING: a window has been destoryed by assignment, was this intended?");
#endif
	}

	// now copy data for the rvalue:
	m_pWindow = a_rOther.m_pWindow;
	m_pGLEWContext = a_rOther.m_pGLEWContext;
	m_iWidth = a_rOther.m_iWidth;
	m_iHeight = a_rOther.m_iHeight;
	m_iID = a_rOther.m_iID;
	m_szTitle = a_rOther.m_szTitle;

	// we need to make sure we arew bound to setup the callbacks:
	auto hPrevContext = Window::CurrentContext();
	this->MakeCurrent();

	// update our glfw callbacks and user pointer:
	glfwSetWindowUserPointer(m_pWindow, static_cast<void*>(this));
	SetupCallbacks();

	// keep our window list up to date:
	for (auto& win : m_aWindows)
	{
		if (win->ID() == a_rOther.ID())
		{
			win = this;
		}
	}

	// nuter old widnow:
	a_rOther.m_pWindow = nullptr;
	a_rOther.m_pGLEWContext = nullptr;
	a_rOther.m_iID = -1;

	// re-set activew/bound context state for this thread:
	MakeContextCurrent(hPrevContext);

	return *this;
}


//////////////////////// Window Destructors //////////////////////////////

Window::~Window()
{
	Destory();
}

// Note that we use asserts in this function instead of the
// exceptions used elsewhere because it is called in the destructor (see above).
// And if we incorrectly call glfwDestroyWindow() then our app is going to crash anyway.
// At lease by calling Assert we controll where it happens.
void Window::Destory()
{
	// if we are destorying a valid window then we need a whole bunch of safety checks:
	if (IsValid())
	{
		// note that because we can only call glfwDestoryWindow() on the
		// "main" thread we should first check that this is the case.
		// see: http://www.glfw.org/docs/latest/group__window.html#gacdf43e51376051d2c091662e9fe3d7b2 for detials.
		// we will use an assert here as we cannot through from the destructor.
		assert(CalledOnMainThread()); 

		// check to see if we are bound on this thread, 
		// if so detach us so we can safly delete ourself.
		// Note that this is not foolproof as it is possible for us to be current on a different thread.
		// note that even though glfw will detach a context bound on the main threads, 
		// we still need to call this to clean up our own state.
		auto pCurrentContext = Window::CurrentContext();
		if (pCurrentContext != nullptr && pCurrentContext == this)
		{
			MakeContextCurrent(nullptr);
		}

		// so what happens if we are not bound on this main thread??
		// well according to the docs (http://www.glfw.org/docs/latest/group__window.html#gacdf43e51376051d2c091662e9fe3d7b2)
		// the context can safley be bound on the main thread, but it cannot be bound on any other thread.
		// so we better make sure that is the case:
#ifdef _DEBUG
		// print nice help message if compiling in debug:
		if (CalledOnBoundThread() == false)			// note that the default ctor for thread::id will have a value that specifes no thread.
		{
			// if it turns out we are on a different thread:
			std::cout << "WARNING: You are destorying a window on a different thread then the one on which it is bound." << std::endl;
			std::cout << "Window Title: " << m_szTitle << std::endl;
			std::cout << "Bound Thread: " << m_oBoundThread << std::endl;
			std::cout << "Calling Thread: " << std::this_thread::get_id() << std::endl;
		}
#endif
		
		// then asset that we are not bound (yes, i know this repeats the above, but the above was to give us a nice helper message).
		assert(CalledOnBoundThread());
	}

	// now we can finally destruct the window (and we know it is safe to do so :P)
	if (m_pGLEWContext != nullptr)
	{
		delete m_pGLEWContext;
		m_pGLEWContext = nullptr;
	}
	if (m_pWindow != nullptr)
	{
		glfwDestroyWindow(m_pWindow);
		m_pWindow = nullptr;
	}

	// remove us from the window list:
	auto itr = std::remove(m_aWindows.begin(), m_aWindows.end(), this);
	if (itr != m_aWindows.end())
		m_aWindows.erase(itr);
}


//////////////////////// Window Class Context Mangment //////////////////////////////

void Window::MakeContextCurrent(Window* a_pWindowHandle)
{
	// first we must do a safty check to make sure we are not bound on a differnt thread:
	// NOTE: std::thread::id's ctor will by default creats an
	// ID which identifies no specific thread, so the below
	// will be true if we are bound to any valid thread.
	if (a_pWindowHandle != nullptr											// moot point if the its a null context :P
		&& a_pWindowHandle->CalledOnBoundThread() == false)
	{
		auto oCallingThread = std::this_thread::get_id();

#ifdef _DEBUG
		std::cout << "ERROR: Cannot make context for window " << a_pWindowHandle->m_szTitle
			<< " current on thread ID: " << oCallingThread
			<< ", as it is alread bound on thead ID: " << a_pWindowHandle->m_oBoundThread << std::endl;
#endif

		throw NotCalledOnBoundThreadException(oCallingThread, a_pWindowHandle->m_oBoundThread);

		//return; // oopps we can't make this context current.
	}

	// set any existing contexts bound thread id to none:
	auto pCurrentContext = Window::CurrentContext();
	if (pCurrentContext != nullptr)
		pCurrentContext->m_oBoundThread = std::thread::id();

	// if we have been passed a nullptr detatch the current context.
	if (a_pWindowHandle == nullptr)
	{
		glfwMakeContextCurrent(nullptr);
		return;
	}

	a_pWindowHandle->m_oBoundThread = std::this_thread::get_id();
	glfwMakeContextCurrent(a_pWindowHandle->m_pWindow);
}



void Window::MakeCurrent()
{
	// first we must do a safty check to make sure we are not bound on a differnt thread:
	auto oCallingThread = std::this_thread::get_id();
	if (CalledOnBoundThread() == false)
	{
#ifdef _DEBUG
		std::cout << "Cannot make context for window " << m_szTitle
			<< " current on thread ID: " << std::this_thread::get_id()
			<< ", as it is alread bound on thead ID: " << m_oBoundThread << std::endl;
#endif

		throw NotCalledOnBoundThreadException(oCallingThread, m_oBoundThread);

		//return; // oopps we can't make this context current.
	}

	m_oBoundThread = std::this_thread::get_id();
	glfwMakeContextCurrent(m_pWindow);
}


Window* Window::CurrentContext()
{
	// get the current context from GLFW:
	GLFWwindow* pGLFWWindow = glfwGetCurrentContext();
	if (pGLFWWindow == nullptr)
		return nullptr;										

	// Use the User Pointer to get get our window class:
	return static_cast<WindowHandle>(glfwGetWindowUserPointer(pGLFWWindow));
}


//////////////////////// GLFW Interface Functions //////////////////////////////

void Window::SwapBuffers()
{
	// we assume that the window has already been made current on the calling thread.
	glfwSwapBuffers(m_pWindow);
}


bool Window::ShouldClose()
{
	int b = glfwWindowShouldClose(m_pWindow);
	if (b == 0)
		return false;

	return true;
}


void Window::SetGLFWWindowHints(WindowHints* a_pWindowHints)
{
	if (a_pWindowHints == nullptr || a_pWindowHints->empty())
	{
		// if no window hints provided, use glfw defaults:
		glfwDefaultWindowHints();
	}
	else
	{
		for (const auto& pair : *a_pWindowHints)
		{
			// pair.first = hint and pair.second = value
			// see http://www.glfw.org/docs/latest/group__window.html#ga4fd9e504bb937e79588a0ffdca9f620b
			glfwWindowHint(pair.first, pair.second);
		}
	}

	// if compiling in debug mode then we want a debug context:
#ifdef _DEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLU_TRUE);
#endif
}


void Window::SetupCallbacks()
{
	// setup callback for framebuffer size changes:
	glfwSetFramebufferSizeCallback(m_pWindow, GLFWFramebufferSizeCallback);

	// setup openGL Error callback:
#ifdef _DEBUG
	if (GLEW_ARB_debug_output) // test to make sure we can use the new callbacks, they wer added as an extgension in 4.1 and as a core feture in 4.3
	{
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);												// this allows us to set a break point in the callback function.
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, 0, GL_TRUE);		// tell openGl what errors we want (all).
		glDebugMessageCallback(GLErrorCallback, static_cast<void*>(this));					// define the callback function.
	}
#endif
}


//////////////////////// GLFW Callbacks //////////////////////////////

void Window::GLFWWindowSizeCallback(GLFWwindow* a_pWindow, int a_iWidth, int a_iHeight)
{
	auto pWindow = static_cast<WindowHandle>(glfwGetWindowUserPointer(a_pWindow));

	if (pWindow != nullptr)
	{
		pWindow->m_iHeight = a_iWidth;
		pWindow->m_iWidth = a_iHeight;
	}
}


void Window::GLFWFramebufferSizeCallback(GLFWwindow* a_pWindow, int a_iWidth, int a_iHeight)
{
	auto pWindow = static_cast<WindowHandle>(glfwGetWindowUserPointer(a_pWindow));

	if (pWindow != nullptr)
	{
		pWindow->m_m4Projection = glm::perspective(45.0f, float(a_iWidth) / float(a_iHeight), 0.1f, 1000.0f);
	}

	// NOTE: the below code could cause a problem if the context is already bound
	// on a different thread to the calling thread of this function...
	auto previousContext = Window::CurrentContext();
	pWindow->MakeCurrent();
	glViewport(0, 0, a_iWidth, a_iHeight);
	MakeContextCurrent(previousContext);
}


//////////////////////// OpenGL Callback //////////////////////////////

// Original version sourced form: http://blog.nobel-joergensen.com/2013/02/17/debugging-opengl-part-2-using-gldebugmessagecallback/
#ifdef _DEBUG
void APIENTRY GLErrorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei /* length */, const GLchar* message, void*  userParam)
{
	// get window var:
	Window* pWindow = static_cast<Window*>(userParam);

	printf("---------------------opengl-callback-start------------\n");
	if (pWindow != nullptr)
	{
		printf("Message from Window ID: %d, Title: %s\n", pWindow->ID(), pWindow->Title().c_str());
	}
	else
	{
		printf("Message from Unknown Window\n");
	}

	printf("Message: %s\n", message);

	printf("Source: ");
	switch (source)
	{
	case GL_DEBUG_SOURCE_API:
		printf("GL_DEBUG_SOURCE_API\n");
		break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		printf("GL_DEBUG_SOURCE_SHADER_COMPILER\n");
		break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		printf("GL_DEBUG_SOURCE_THIRD_PARTY\n");
		break;
	case GL_DEBUG_SOURCE_APPLICATION:
		printf("GL_DEBUG_SOURCE_APPLICATION\n");
		break;
	case GL_DEBUG_SOURCE_OTHER:
		printf("GL_DEBUG_SOURCE_API\n");
		break;
	}
	
	printf("Type: ");
	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:
		printf("ERROR\n");
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		printf("DEPRECATED_BEHAVIOR\n");
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		printf("UNDEFINED_BEHAVIOR\n");
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		printf("PORTABILITY\n");
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		printf("PERFORMANCE\n");
		break;
	case GL_DEBUG_TYPE_OTHER:
		printf("OTHER\n");
		break;
	}

	printf("ID: %d, Severity: ", id);
	switch (severity)
	{
	case GL_DEBUG_SEVERITY_LOW:
		printf("LOW\n");
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		printf("MEDIUM\n");
		break;
	case GL_DEBUG_SEVERITY_HIGH:
		printf("HIGH\n");
		break;
	}

	printf("---------------------opengl-callback-end--------------\n");
}
#endif


//////////////////////// Convenience functions //////////////////////////////

bool Window::CalledOnMainThread()
{
	if (g_oMainThread == std::this_thread::get_id())	// if called on the main thread
		return true;									// return true.

	return false;										// else return false.
}


bool Window::CalledOnBoundThread()
{
	if ( m_oBoundThread == std::thread::id()			// if not bound or
		|| m_oBoundThread == std::this_thread::get_id())// bound to the calling thread
	{
		return true;									// then return true.
	}

	return false;										// else return false.
}