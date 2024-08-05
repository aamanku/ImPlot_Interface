#ifndef IMPLOT_INTERFACE_HPP
#define IMPLOT_INTERFACE_HPP

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <map>
#include <thread>


namespace ImPlotInterface
{
	static void GLFWErrorCallback(int error, const char* description);

	struct StaticPlotData
	{
		std::string plot_name;
		std::string x_label;
		std::vector<std::string> y_label;

		std::vector<double> x_data;
		std::vector<std::vector<double>> y_data;

		std::mutex mtx;
	};

	struct DynamicPlotData : public StaticPlotData
	{
		std::vector<double> y_data;
		std::string y_label;
		std::size_t x_window_size;
		std::size_t counter = 0;

		void AddData(double x, double y);
	};

	class ImPlotter
	{
	private:
		static constexpr std::chrono::milliseconds kInternalSleepTime = std::chrono::milliseconds(10);
		std::map<std::string, StaticPlotData *> static_plot_map_;
		std::map<std::string, DynamicPlotData *> dynamic_plot_map_;

		bool running_ = false;
		bool paused_ = false;
		bool glfw_freed_ = false;
		bool use_mutex_;
		int window_width_;
		int window_height_;

		std::thread plot_thread_;

		void MainLoop_();
		void Log_(const std::string& message);
		void Exception_(const std::string& message);
		void LockMutex_(std::mutex& mtx);
		void UnLockMutex_(std::mutex& mtx);
	public:
		explicit ImPlotter(int width = 1280, int height = 720, bool use_mutex = true);
		~ImPlotter();

		void RunMainLoop();
		void StopMainLoop();

		void AddStaticPlot(const std::string& plot_name,
			const std::vector<double>& x_data,
			const std::vector<std::vector<double>>& y_data,
			const std::string& x_label,
			const std::vector<std::string>& y_label);

		void AddStaticPlot(const std::string& plot_name,
			const std::vector<double>& x_data,
			const std::vector<double>& y_data,
			const std::string& x_label = "x",
			const std::string& y_label = "y");

		void AddDynamicPlot(const std::string& plot_name,
			double x,
			double y,
			size_t windowSize,
			const std::string& x_label = "x",
			const std::string& y_label = "y");

		void AddDynamicPlot(const std::string& plot_name,
			double y,
			size_t windowSize,
			const std::string& x_label = "x",
			const std::string& y_label = "y");

		void Pauser();

		[[nodiscard]] bool IsRunning() const;
		[[nodiscard]] bool IsPaused() const;
		[[nodiscard]] bool IsGlfwFreed() const;



	};
}

#endif // IMPLOT_INTERFACE_HPP