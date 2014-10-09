// Note that the following includes must be defined in order:
#include "../Window.h"
#include "GL\glew.h"
#include "GLFW\glfw3.h"
#include "catch.hpp"

// Note the the following Includes do not need to be defined in order:
#include <chrono>
#include <thread>

// Test Class to init and shutdown GLFW!!
class WindowTestsFixture
{
public:
	WindowTestsFixture() : m_bGLFWInitialized(false)
	{
		// Init GLFW:
		if (glfwInit())
			m_bGLFWInitialized = true;
	}

	~WindowTestsFixture()
	{
		if (m_bGLFWInitialized)
			glfwTerminate();
	}

protected:
	bool GLFWInitialized() { return m_bGLFWInitialized; }

private:
	bool m_bGLFWInitialized;
};


// actuall tests:
TEST_CASE_METHOD(WindowTestsFixture, "Window Initialisation on Main Thread", "[Init]")
{
	REQUIRE(GLFWInitialized() == true);

	// create our window:
	REQUIRE_NOTHROW(Window(1280, 720, "Test"));
}

TEST_CASE_METHOD(WindowTestsFixture, "Context Bind on Main Thread", "[Bind]")
{
	REQUIRE(GLFWInitialized() == true);

	// create our window:
	Window TestWindow = Window(1280, 720, "Test");

	REQUIRE_NOTHROW(TestWindow.MakeCurrent());
}

//
//
//void Test()
//{
//	Window TestWindow = Window(1280, 720, "test");
//
//	auto thread = std::thread([](Window* win)
//	{
//		Window TestWindow = Window(1280, 720, "test");
//
//		//		win->MakeCurrent();
//		std::chrono::milliseconds dura(2000);
//		std::this_thread::sleep_for(dura);
//		TestWindow.MakeCurrent();
//		std::this_thread::sleep_for(dura);
//
//
//		//Window::MakeContextCurrent(nullptr);
//
//	}, &TestWindow);
//
//	thread.join();
//
//	//window->MakeCurrent();
//
//	//delete window;
//}