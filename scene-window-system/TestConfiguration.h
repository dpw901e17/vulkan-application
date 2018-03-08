#pragma once
#include <string>
#include <sstream>
#include <vector>

struct TestConfiguration
{
	bool reuseCommandBuffers = false;
	size_t probeInterval = 0;	//make probes every frame. Interval is given in ms.
	size_t seconds = 0;	//run until program is closed by user
	bool exportCsv = false;
	bool pipelineStatistics = false;
	bool openHardwareMonitorData = false;
	bool rotateCubes = false;
	size_t drawThreadCount = 1;
	size_t cubeDimension = 2;
	int cubePadding = 1;
	bool recordFPS = false;

	//TODO: use better pattern than singleton?
	static TestConfiguration& GetInstance() 
	{ 
		return instance; 
	}

	std::string MakeString(std::string separator) {
		std::stringstream ss;
		//header
		ss << "Setting" << separator << "Value" << "\n";
		
		//data
		ss << "Reuse CommandBuffers"	<< separator << force_string(reuseCommandBuffers)		<< "\n";
		ss << "Probe Interval"			<< separator << force_string(probeInterval)				<< "\n";
		ss << "Seconds"					<< separator << force_string(seconds)					<< "\n";
		ss << "Export CSV"				<< separator << force_string(exportCsv)					<< "\n";
		ss << "Pipeline Statistics"		<< separator << force_string(pipelineStatistics)		<< "\n";
		ss << "Open Hardware Monitor"	<< separator << force_string(openHardwareMonitorData)	<< "\n";
		ss << "Rotate Cubes"			<< separator << force_string(rotateCubes)				<< "\n";
		ss << "Draw Thread Count"		<< separator << force_string(drawThreadCount)			<< "\n";
		ss << "Cube Dimension"			<< separator << force_string(cubeDimension)				<< "\n";
		ss << "Cube Padding"			<< separator << force_string(cubePadding)				<< "\n";

		return ss.str();
	}

	static void SetTestConfiguration(const char* exeArgs) {
		auto& testConfig = GetInstance();

		//get exe arguments
		std::vector<std::string> args;

		std::string str(exeArgs);
		std::string arg = "";
		for (char c : str) {
			if (c == ' ') {
				args.push_back(arg);
				arg = "";
			}
			else {
				arg += c;
			}
		}

		args.push_back(arg);

		std::string a = "";
		for (auto i = 0; i < args.size(); ++i) {
			a = args[i];
			if (a == "-csv") {
				testConfig.exportCsv = true;
			}
			else if (a == "-sec") {
				testConfig.seconds = stoi(args[i + 1]);
			}
			else if (a == "-OHM") {
				testConfig.openHardwareMonitorData = true;
			}
			else if (a == "-pipelineStatistics") {
				testConfig.pipelineStatistics = true;
			}
			else if (a == "-pi") {
				testConfig.probeInterval = stoi(args[i + 1]);
			}
			else if (a == "-reuseComBuf") {
				testConfig.reuseCommandBuffers = true;
			}
			else if (a == "-rotateCubes") {
				testConfig.rotateCubes = true;
			}
			else if (a == "-threadCount") {
				testConfig.drawThreadCount = stoi(args[i + 1]);
			}
			else if (a == "-cubeDim") {
				testConfig.cubeDimension = stoi(args[i + 1]);
			}
			else if (a == "-cubePad") {
				testConfig.cubePadding = stoi(args[i + 1]);
			}
			else if (a == "-fps") {
				testConfig.recordFPS = true;
			}
		}
	}

private:
	TestConfiguration() {};
	static TestConfiguration instance;

	template<typename T>
	std::string force_string(T arg)
	{
		std::stringstream ss;
		ss << arg;
		return ss.str();
	}

	template<>
	std::string force_string(bool arg) {
		return arg ? "true" : "false";
	}
};