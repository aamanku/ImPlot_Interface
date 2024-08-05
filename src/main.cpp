#include <iostream>
#include "implot_interface/implot_interface.hpp"
#include <math.h>
#include <chrono>
using namespace ImPlotInterface;

int main() {
	ImPlotter plotter;
	plotter.RunMainLoop();

	double t_init = std::chrono::system_clock::now().time_since_epoch().count();

	while(plotter.IsRunning())
	{
		double t = std::chrono::system_clock::now().time_since_epoch().count() - t_init;

		// Static plot
		plotter.AddStaticPlot("Test", {1, 2, 3, 4, 5}, {{1, 2, 3, 4, 5}, {1, 4, 9, 16, 25}}, "X", {"Y1", "Y2"});
		// noise plot
		double random_value = std::rand() % 100;
		plotter.AddDynamicPlot("Random", random_value, 100);

		// circle plot
		double radius = 0.5;
		double omega = 0.000000001;
		double x_val = radius * std::cos(omega*t);
		double y_val = radius * std::sin(omega*t);
		plotter.AddDynamicPlot("circle", x_val, y_val, 100);

		// static plot
		std::vector<double> x(1000);
		std::vector<double> y(1000);
		for(int i = 0; i < 1000; i++)
		{
			x[i] = i;
			y[i] = std::sin(omega*t + i*0.01);
		}
		plotter.AddStaticPlot("sin", x, y, "X", "Y");

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	plotter.StopMainLoop();

	return 0;
}