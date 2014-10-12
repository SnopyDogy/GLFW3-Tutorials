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
TEST_CASE_METHOD(WindowTestsFixture, "Window Initialisation", "[Init]")
{
	REQUIRE(GLFWInitialized() == true);

	SECTION("On Main Thread")
	{
		// declare here to stop it exiting scope inside catch and messing with later Requires.
		Window TestWindow;

		REQUIRE_NOTHROW(
			TestWindow = Window(1280, 720, "Test");
		);

		// test that window is valid:
		REQUIRE(TestWindow.IsValid() == true);

		// There should be no bound/active context after the ctor finishes:
		REQUIRE((Window::CurrentContext() == nullptr) == true);

		// there should be one active window:
		REQUIRE((Window::ActiveWindows().size() == 1) == true);
	}
	SECTION("NOT on Main Thread")
	{
		// Create a thread to test init:
		auto thread = std::thread([]()
		{
			REQUIRE_THROWS_AS(Window(1280, 720, "Test-Threaded"), NotCalledOnMainThreadException);
		});

		// join the thread:
		thread.join();

		// there should be no active windows (as the above ctor should fail):
		REQUIRE((Window::ActiveWindows().size() == 0) == true);
	}
}

TEST_CASE_METHOD(WindowTestsFixture, "Window Move Constructors", "[Move]")
{
	REQUIRE(GLFWInitialized() == true);

	SECTION("Move Assignment")
	{
		// create initial window:
		Window TestWindow = Window(1280, 720, "Test1");

		// there should be one active window:
		REQUIRE((Window::ActiveWindows().size() == 1) == true);

		// We will use a lambda to make sure the move occures:
		// this will also destory the old window:
		auto lambda = []() { return Window(1280, 720, "Test2"); };
		REQUIRE_NOTHROW(
			TestWindow = lambda());

		// test that window is valid:
		REQUIRE(TestWindow.IsValid() == true);

		// There should be no bound/active context after the move finishes:
		REQUIRE((Window::CurrentContext() == nullptr) == true);

		// and there should still be one active window:
		REQUIRE((Window::ActiveWindows().size() == 1) == true);
	}
	SECTION("Move Ctor")
	{
		// We will use a lambda to make sure the move occures:
		auto lambda = []() { return Window(1280, 720, "Test"); };
		REQUIRE_NOTHROW(
			Window TestWindow = Window(lambda()));
	}
}

// Note that we cannot thest this on a thread other then  main,
// doing so will trigger an assert and crash the test suite.
TEST_CASE_METHOD(WindowTestsFixture, "Window Destruction on Main Thread", "[Shutdown]")
{
	REQUIRE(GLFWInitialized() == true);

	// create our window:
	WindowHandle window = nullptr;
	REQUIRE_NOTHROW(window = new Window(1280, 720, "Test"));
	REQUIRE((window != nullptr) == true);

	SECTION("Destruction when not bound")
	{
		// note that this will never through so should always pass.
		// however if there is something is wrong in the destructor, 
		// such as an incorrect call to glfw, the test suit may crash.
		REQUIRE_NOTHROW(delete window);		

		// there should be no active windows left:
		REQUIRE((Window::ActiveWindows().size() == 0) == true);
	}
	SECTION("Destruction when Bound")
	{
		REQUIRE_NOTHROW(window->MakeCurrent());

		// note that this will never through so should always pass.
		// however if there is something is wrong in the destructor, 
		// such as an incorrect call to glfw, the test suit may crash.
		REQUIRE_NOTHROW(delete window);

		// There should be no bound/active context after the dtor finishes:
		REQUIRE((Window::CurrentContext() == nullptr) == true);

		// there should be no active windows left:
		REQUIRE((Window::ActiveWindows().size() == 0) == true);
	}
}


TEST_CASE_METHOD(WindowTestsFixture, "Context Bind & Unbind", "[Bind]")
{
	REQUIRE(GLFWInitialized() == true);

	// create our window:
	Window TestWindow = Window(1280, 720, "Test");

	SECTION("On Main Thread")
	{
		SECTION("Test Context Bind")
		{
			REQUIRE_NOTHROW(TestWindow.MakeCurrent());

			// check the window is bound/current:
			REQUIRE((Window::CurrentContext() == &TestWindow) == true);

			SECTION("Test re-bind of already bound context")
			{
				REQUIRE_NOTHROW(TestWindow.MakeCurrent());

				// check the window is bound/current:
				REQUIRE((Window::CurrentContext() == &TestWindow) == true);
			}
			SECTION("Test re-bind of already bound context using static method")
			{
				REQUIRE_NOTHROW(Window::MakeContextCurrent(&TestWindow));

				// check the window is bound/current:
				REQUIRE((Window::CurrentContext() == &TestWindow) == true);
			}
			SECTION("Test Context Unbind")
			{
				REQUIRE_NOTHROW(Window::MakeContextCurrent(nullptr));

				// check the window is not bound/current:
				REQUIRE((Window::CurrentContext() == nullptr) == true);

				SECTION("Test Context Unbbind when no Context is bound")
				{
					REQUIRE_NOTHROW(Window::MakeContextCurrent(nullptr));

					// check the window is not bound/current:
					REQUIRE((Window::CurrentContext() == nullptr) == true);
				}
			}
		}
		SECTION("Test Context Bind using static method")
		{
			REQUIRE_NOTHROW(Window::MakeContextCurrent(&TestWindow));

			// check the window is bound/current:
			REQUIRE((Window::CurrentContext() == &TestWindow) == true);
		}
	}
	SECTION("On Child Thread")
	{
		SECTION("Test Context Bind and Unbind")
		{
			auto thread = std::thread([](Window* win)
			{
				REQUIRE_NOTHROW(win->MakeCurrent());					// test bind

				// check the window is bound/current:
				REQUIRE((Window::CurrentContext() == win) == true);

				REQUIRE_NOTHROW(Window::MakeContextCurrent(nullptr));	// test unbind

				// check the window is not bound/current:
				REQUIRE((Window::CurrentContext() == nullptr) == true);

			}, &TestWindow);

			thread.join();
		}
		SECTION("Test bound Context re-bind on a different thread")
		{
			REQUIRE_NOTHROW(TestWindow.MakeCurrent());  // bind on main thread.

			auto thread = std::thread([](Window* win)
			{
				REQUIRE_THROWS_AS(win->MakeCurrent(), NotCalledOnBoundThreadException);	// test rebind on different thread.

				// check the window is not bound/current (as the above should fail):
				REQUIRE((Window::CurrentContext() == nullptr) == true);
			}, &TestWindow);

			thread.join();
		}
		SECTION("Test bound Context re-bind on a different thread using static method")
		{
			REQUIRE_NOTHROW(TestWindow.MakeCurrent());  // bind on main thread.

			auto thread = std::thread([](Window* win)
			{
				REQUIRE_THROWS_AS(Window::MakeContextCurrent(win), NotCalledOnBoundThreadException);	// test rebind on different thread.

				// check the window is not bound/current (as the above should fail):
				REQUIRE((Window::CurrentContext() == nullptr) == true);
			}, &TestWindow);

			thread.join();
		}
	}
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