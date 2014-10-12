////////////////////////////////////////////////////////////
/// @file		Window.h
/// @details	Header file containing the Window Class.
/// @author		Greg Nott
/// @version	1.1 - Added exceptions
/// @date		9/10/14
////////////////////////////////////////////////////////////

#ifndef _WINDOW_H_
#define _WINDOW_H_

#include <vector>
#include <utility>						// for std::pair
#include <thread>
#include <stdexcept>

#include "glm\glm.hpp"

// Forward declarations:
struct GLFWwindow;
struct GLFWmonitor;
struct GLEWContextStruct;							// AKA GLEWContext;

typedef std::pair<int, int>			WindowHint;		///< Stores a GLFW window hint (in first) and its value (in second).
typedef std::vector<WindowHint>		WindowHints;	///< Array of GLFW window hints (i.e. an array of WindowHint pairs).


/*! @brief Wrapper class for a GLFW window and GLEW Context.
*
*  Wrapper class for a GLFW window and GLEW Context.
*/
class Window
{
public:

	/*! @brief Window Constructor.
	*
	* Constructs a window object by calling the required GLFW and GLEW functions.
	* The resulting object wraps the GLFW window handle and the GLEW context and m,anages their lifetime.
	*
	* @param[in] a_iWidth The desired width, in screen coordinates, of the window.
	* This must be greater than zero.
	* @param[in] a_iHeight The desired height, in screen coordinates, of the window.
	* This must be greater than zero.
	* @param[in] a_szTitle The initial, UTF-8 encoded window title.
	* @param[in] a_pMonitor The monitor to use for full screen mode, or nullptr to use
	* windowed mode.
	* @param[in] a_pShare The window whose context to share resources with, or nullptr
	* to not share resources.
	* @param[in] a_pWindowHints A vector containg the desired GLFW window hints to set
	* before creating the window. The window hints should be stored in a std::pair<int, int>
	* with pair.first storing the hint name and pair.second storing the hint value.
	*
	*  @note This function may only be called from the main thread.
	*/
	Window(int a_iWidth, int a_iHeight, const std::string& a_szTitle, 
			GLFWmonitor* a_pMonitor = nullptr, Window* a_pShare = nullptr, WindowHints* a_pWindowHints = nullptr);
	Window();

	Window(Window&& a_rOther);													///< Move Constructor.
	Window(const Window& a_rOther) = delete;									///< Do not allow Copies.
	Window& operator=(Window&& a_rOther);										///< Move Constructor.
	Window& operator=(const Window& a_rOther) = delete;							///< Do Not Allow Copies.
	~Window();

	// Getters and Setters.
	const int ID() const				{ return m_iID; }						///< Windows unquie ID.
	bool IsValid() const;														///< Use to test if this is a valid window.

	const int Width() const				{ return m_iWidth; }					///< Window Width.
	const int Height() const			{ return m_iHeight; }					///< Window Height.

	const std::string& Title() const	{ return m_szTitle; }					///< Window Title.

	GLFWwindow* GLFWWindowHandle()		{ return m_pWindow; }					///< GLFW window handle.
	GLEWContextStruct* GLEWContext()	{ return m_pGLEWContext; }				///< Pointer to this windows GLEW Context.

	static const std::vector<Window*>& ActiveWindows() { return m_aWindows; }	///< Returns a std::vector contaning a handle to every valid/active window.

	// Interfaces to GLFW Functions
	void SwapBuffers();															///< Calls glfwSwapBuffers() passing it this window instance (assumes the window is already current).
	bool ShouldClose();															///< Checks glfwWindowShouldClose() for this window instance.
	
	// Callbacks for GLFW/GLEW
	static void GLFWWindowSizeCallback(GLFWwindow* a_pWindow, int a_iWidth, int a_iHeight);
	static void GLFWFramebufferSizeCallback(GLFWwindow* a_pWindow, int a_iWidth, int a_iHeight);

	// Context control
	static void MakeContextCurrent(Window* a_pWindowHandle);					///< Makes the specified context current, if nullptr is provided then it will ensure no context is bound oin the calling thread.
	static Window* CurrentContext();											///< Retrives the current "context" from GLFW and returns the corrosponding window handle. 
	void MakeCurrent();															///< Makes the context for this window current.

	// to be moved to a camera class:
	glm::mat4			m_m4Projection;											///< Projection matrix for this windows camera.
	glm::mat4			m_m4ViewMatrix;											///< View Matrix for this windows camera.
private:
	GLFWwindow*			m_pWindow;												///< Handle to the GLFW Window.
	GLEWContextStruct*	m_pGLEWContext;											///< Handle to the GLEW Context.
	std::thread::id		m_oBoundThread;											///< ID of the thread to which this windows context is currently bound!
	int					m_iWidth;			
	int					m_iHeight;
	
	std::string			m_szTitle;												///< The window title.
	
	int					m_iID;													///< Unquie ID for this window.
	static int			m_iWindowCounter;										///< Used to gurantee a unquie ID for each window.

	static std::vector<Window*> m_aWindows;										///< Array of all active windows.

	static std::thread::id	m_oMainThread;										///< The id of the programs "main" thread.

	void SetGLFWWindowHints(WindowHints* a_pWindowHints);						///< Sets GLFW window hints.
	void SetupCallbacks();														///< Sets up GLFW and OpenGL callbacks for the window.
	void Destory();																///< Destorys the window completely, used in destructor and copy constructor (if neccesary)
	bool CalledOnMainThread();													///< Convenience function used internally to check that a method was called on the Main Thread. 
	bool CalledOnBoundThread();													///< Convenience function used internally to check that a method has been called on the same thread the context is bound too.
};


typedef Window*		WindowHandle;												///< Window Handle typedef.


inline bool Window::IsValid() const
{
	if (m_pWindow == nullptr || m_pGLEWContext == nullptr)
		return false;

	return true;
}


/*! @brief Returns the currently active/bound GLEW context.
*
*  This needs to be defined for GLEW MX to work, along with the GLEW_MX define in the perprocessor!
*/
GLEWContextStruct* glewGetContext();



//////////////////////// Exceptions thrown by the window class //////////////////////////////


/*! @brief Exception for misuse of the window class. 
*
*  This exception is thrown by the window class whenever the user
*  tries to do something on a thread other then the programs main (first)
*  thread which GLFW requires be done on the main thread only. See the GLFW 
*  window handling documentation for more details.
*/
class NotCalledOnMainThreadException : public std::runtime_error
{
public:
	NotCalledOnMainThreadException(std::thread::id a_oCallingThreadID, std::thread::id a_oMainThreadID)
		: runtime_error("Method was not called on the main thread as required by GFLW.")
		, m_oCallingThreadID(a_oCallingThreadID)
		, m_oMainThreadID(a_oMainThreadID)
	{}

	std::thread::id CallingThreadID() const { return m_oCallingThreadID;	}
	std::thread::id MainThreadID() const	{ return m_oMainThreadID;		}

private:
	std::thread::id m_oCallingThreadID;
	std::thread::id m_oMainThreadID;
};


/*! @brief Exception for misuse of the window class.
*
*  This exception is thrown by the window class whenever the user
*  tries to do something (typiclay SetActive()) when you have 
*  already bound the Window/context on a DIFFERENT thread.
*  See the GLFW window handling documentation for more details.
*/
class NotCalledOnBoundThreadException : public std::runtime_error
{
public:
	NotCalledOnBoundThreadException(std::thread::id a_oCallingThreadID, std::thread::id a_oBoundThreadID)
		: runtime_error("Method was not called on the currently Bound thread as required by GFLW/OpenGL.")
		, m_oCallingThreadID(a_oCallingThreadID)
		, m_oBoundThreadID(a_oBoundThreadID)
	{}

	std::thread::id CallingThreadID() const { return m_oCallingThreadID;	}
	std::thread::id BoundThreadID() const	{ return m_oBoundThreadID;		}

private:
	std::thread::id m_oCallingThreadID;
	std::thread::id m_oBoundThreadID;
};


#endif // _WINDOW_H_