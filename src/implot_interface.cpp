#include "implot_interface/implot_interface.hpp"
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "implot.h"
#include "implot_internal.h"
using namespace ImPlotInterface;

//extern bool ImPlotInterface::use_mutex_global;



static void ImPlotInterface::GLFWErrorCallback(int error, const char* description)
{
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

void DynamicPlotData::AddData(double x, double y)
{
	x_data.push_back(x);
	y_data.push_back(y);

	if (x_data.size() > x_window_size)
	{
		x_data.erase(x_data.begin());
		y_data.erase(y_data.begin());
	}
}

ImPlotter::ImPlotter(int width, int height, bool use_mutex) : use_mutex_(use_mutex), window_height_(height), window_width_(width)
{
	std::cout << "ImPlot_Interface" << std::endl;
	// flash ascii art of sine wave
	std::cout << R"(
       ___         ___            ___
      /__/\       /  /\          /__/\
      \__\:\     /  /::\         \__\:\
      /  /::\   /  /:/\:\        /  /::\
   __/  /:/\/  /  /::\ \:\    __/  /:/\/
  /__/\/:/~~  /__/:/\:\_\:\  /__/\/:/~~
  \  \::/     \__\/  \:\/:/  \  \::/
   \  \:\          \  \::/    \  \:\
    \__\/           \__\/      \__\/

	)" << std::endl;
}

ImPlotter::~ImPlotter()
{
	std::cout << "ImPlotter destructor" << std::endl;
	if (!glfw_freed_)
	{
		std::cout << "=====================================" << std::endl;
		std::cout << "||Waiting for the plotter to finish||" << std::endl;
		std::cout << "=====================================" << std::endl;
		paused_ = true;
		while (running_)
		{
			std::this_thread::sleep_for(kInternalSleepTime);
		}
	}
}

void ImPlotter::MainLoop_()
{
	glfwSetErrorCallback(GLFWErrorCallback);
	if (!glfwInit())
	{
		throw std::runtime_error("Failed to initialize GLFW");
	}

	// GL 3.0 + GLSL 130
	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	// Create window with graphics context
	GLFWwindow* window = glfwCreateWindow(window_width_, window_height_, "ImPlotter", nullptr, nullptr);
	if (window == nullptr)
	{
		throw std::runtime_error("Failed to create GLFW window");
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	// ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Our state
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	while (!glfwWindowShouldClose(window) && running_)
	{
		glfwMakeContextCurrent(window);
		glfwPollEvents();

		// check key press to pause
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		{
			paused_ = !paused_;
		}

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		{

			// iterate over all static plots
			{
				for (auto& plotMap : static_plot_map_)
				{
					auto plot = plotMap.second;
					ImGui::Begin("Static");
					LockMutex_(plot->mtx);
					if (!paused_)
					{ // if not paused, set next axes to fit
						ImPlot::SetNextAxesToFit();
					}

					if (ImPlot::BeginPlot(plot->plot_name.c_str()))
					{
						ImPlot::SetupLegend(ImPlotLocation_East, ImPlotLegendFlags_Outside);
					 	ImPlot::SetupAxes(plot->x_label.c_str(), "val");
						for (size_t i = 0; i < plot->y_data.size(); i++)
						{
							ImPlot::PlotLine(plot->y_label[i].c_str(), plot->x_data.data(), plot->y_data[i].data(), plot->x_data.size());
						}
						ImPlot::EndPlot();
					}
					UnLockMutex_(plot->mtx);
					ImGui::End();
				}
			}

			// iterate over all dynamic plots
			{
				for (auto& plotMap : dynamic_plot_map_)
				{
					auto plot = plotMap.second;
					ImGui::Begin("Dynamic");

					LockMutex_(plot->mtx);

					if (!paused_)
					{ // if not paused, set next axes to fit
						ImPlot::SetNextAxesToFit();
					}
					if (ImPlot::BeginPlot(plot->plot_name.c_str()))
					{
					// add x and y labels
					ImPlot::SetupAxes(plot->x_label.c_str(), plot->y_label.c_str());
						// setup axis to auto fit
						ImPlot::PlotLine(plot->plot_name.c_str(), plot->x_data.data(), plot->y_data.data(), plot->x_data.size());
						ImPlot::EndPlot();
					}
					UnLockMutex_(plot->mtx);
					ImGui::End();
				}
			}
		}

		// Rendering
		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	// Cleanup
	glfwMakeContextCurrent(window);
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	ImPlot::DestroyContext();

	glfwDestroyWindow(window);

	running_ = false;
	glfw_freed_ = true;
}

void ImPlotter::RunMainLoop()
{
	plot_thread_ = std::thread(&ImPlotter::MainLoop_, this);
	plot_thread_.detach();
	running_ = true;
}

bool ImPlotter::IsRunning() const
{
	return running_;
}

bool ImPlotter::IsPaused() const
{
	return paused_;
}

bool ImPlotter::IsGlfwFreed() const
{
	return glfw_freed_;
}

void ImPlotter::StopMainLoop()
{
	running_ = false;
}

void ImPlotter::Pauser()
{
	while (paused_)
	{
		std::this_thread::sleep_for(kInternalSleepTime);
	}
}

void ImPlotter::AddStaticPlot(const std::string& plot_name,
	const std::vector<double>& x_data,
	const std::vector<std::vector<double>>& y_data,
	const std::string& x_label,
	const std::vector<std::string>& y_label)
{
	if (!IsRunning())
	{ return; }
	Pauser();
	if (static_plot_map_.find(plot_name) == static_plot_map_.end())
	{
		// Plot does not exist, create it
		Log_("Creating new static plot: " + plot_name);
		static_plot_map_[plot_name] = new StaticPlotData();
		LockMutex_(static_plot_map_[plot_name]->mtx);
		static_plot_map_[plot_name]->plot_name = plot_name;
		static_plot_map_[plot_name]->x_label = x_label;
		static_plot_map_[plot_name]->y_label = y_label;
		static_plot_map_[plot_name]->x_data = x_data;
		static_plot_map_[plot_name]->y_data = y_data;
		// check all sizes
		for (size_t i = 0; i < y_data.size(); i++)
		{
			if (x_data.size() != y_data[i].size())
			{
				Exception_("x_data and y_data[" + std::to_string(i) + "] have different sizes");
			}
		}
		UnLockMutex_(static_plot_map_[plot_name]->mtx);
	}
	else
	{
		// Plot exists, update it
		auto& plot = static_plot_map_[plot_name];
		LockMutex_(plot->mtx);
		plot->x_data = x_data;
		plot->y_data = y_data;
		UnLockMutex_(plot->mtx);
	}
}

void ImPlotter::AddStaticPlot(const std::string& plot_name,
	const std::vector<double>& x_data,
	const std::vector<double>& y_data,
	const std::string& x_label,
	const std::string& y_label)
{
	AddStaticPlot(plot_name, x_data, std::vector<std::vector<double>>{ y_data }, x_label, std::vector<std::string>{ y_label });
}

void ImPlotter::AddDynamicPlot(const std::string& plot_name,
	double x,
	double y,
	size_t window_size,
	const std::string& x_label,
	const std::string& y_label)
{
	if (!IsRunning())
	{ return; }
	Pauser();
	if (dynamic_plot_map_.find(plot_name) == dynamic_plot_map_.end())
	{
		// Plot does not exist, create it
		Log_("Creating new dynamic plot: " + plot_name);
		dynamic_plot_map_[plot_name] = new DynamicPlotData();
		LockMutex_(dynamic_plot_map_[plot_name]->mtx);
		dynamic_plot_map_[plot_name]->plot_name = plot_name;
		dynamic_plot_map_[plot_name]->x_label = x_label;
		dynamic_plot_map_[plot_name]->y_label = y_label;
		dynamic_plot_map_[plot_name]->x_window_size = window_size;
		dynamic_plot_map_[plot_name]->x_data.reserve(window_size);
		dynamic_plot_map_[plot_name]->y_data.reserve(window_size);
		dynamic_plot_map_[plot_name]->AddData(x, y);
		UnLockMutex_(dynamic_plot_map_[plot_name]->mtx);
	}
	else
	{
		// Plot exists, update it
		auto& plot = dynamic_plot_map_[plot_name];
		LockMutex_(plot->mtx);
		if (plot->x_window_size != window_size)
		{
			Log_("Resizing windowSize plot: " + plot_name);
			plot->x_window_size = window_size;
			plot->x_data.reserve(window_size);
			plot->x_data.reserve(window_size);
		}
		plot->AddData(x, y);
		UnLockMutex_(plot->mtx);
	}
}

void ImPlotter::AddDynamicPlot(const std::string& plot_name,
	double y,
	size_t window_size,
	const std::string& x_label,
	const std::string& y_label)
{
	if (!IsRunning())
	{ return; }
	Pauser();
	if (dynamic_plot_map_.find(plot_name) == dynamic_plot_map_.end())
	{
		// Plot does not exist, create it
		Log_("Creating new dynamic plot: " + plot_name);
		dynamic_plot_map_[plot_name] = new DynamicPlotData();
		LockMutex_(dynamic_plot_map_[plot_name]->mtx);
		dynamic_plot_map_[plot_name]->plot_name = plot_name;
		dynamic_plot_map_[plot_name]->x_label = x_label;
		dynamic_plot_map_[plot_name]->y_label = y_label;
		dynamic_plot_map_[plot_name]->x_window_size = window_size;
		dynamic_plot_map_[plot_name]->x_data.reserve(window_size);
		dynamic_plot_map_[plot_name]->y_data.reserve(window_size);
		double x = (double)dynamic_plot_map_[plot_name]->counter;
		dynamic_plot_map_[plot_name]->AddData(x, y);
		dynamic_plot_map_[plot_name]->counter++;
		UnLockMutex_(dynamic_plot_map_[plot_name]->mtx);
	}
	else
	{
		// Plot exists, update it
		auto& plot = dynamic_plot_map_[plot_name];
		LockMutex_(plot->mtx);
		if (plot->x_window_size != window_size)
		{
			std::cout << "Resizing windowSize plot: " << plot_name << std::endl;
			plot->x_window_size = window_size;
			plot->x_data.reserve(window_size);
			plot->y_data.reserve(window_size);
		}
		double x = (double)dynamic_plot_map_[plot_name]->counter;
		plot->AddData(x, y);
		dynamic_plot_map_[plot_name]->counter++;
		UnLockMutex_(plot->mtx);
	}
}

inline void ImPlotter::Log_(const std::string& message)
{
	const std::string log_prefix = "[ImPlotter] ";
	std::cout << log_prefix << message << std::endl;
}

inline void ImPlotter::Exception_(const std::string& message)
{
	const std::string exception_prefix = "[ImPlotter] Exception: ";
	throw std::runtime_error(exception_prefix + message);
}

inline void ImPlotter::LockMutex_(std::mutex& mtx)
{
	if (use_mutex_)
	{
		mtx.lock();
	}
}

inline void ImPlotter::UnLockMutex_(std::mutex& mtx)
{
	if (use_mutex_)
	{
		mtx.unlock();
	}
}