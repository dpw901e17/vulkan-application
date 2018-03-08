#pragma once
//************************************************************************************************************** //
//** based on https://msdn.microsoft.com/en-us/library/aa390423%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396 ** //
//************************************************************************************************************** //

#define _WIN32_DCOM
#include <iostream>
#include <comdef.h>
#include <Wbemidl.h>
#include <vector>
#include <map>
#include <string>
#include <sstream>

#define WMIA_OUTPUT_SEPARATOR L"; "

class CsvExporter
{
public:
	virtual std::string MakeString(std::string seperator) = 0;
};

class WMIDataItem
{
public:
	std::string 
		Id,
		Timestamp,
		FPS,
		ComponentType,
		ComponentID,
		SensorType,
		SensorID,
		Value;
};

class PipelineStatisticsDataItem
{
public:
	std::string 
		CInvocations,
		CPrimitives,
		CSInvocations,
		DSInvocations,
		GSInvocations,
		GSPrimitives,
		HSInvocations,
		IAPrimitives,
		IAVertices,
		PSInvocations,
		VSInvocations,
		CommandListId;
};

template<class T>
class DataCollection : public CsvExporter
{
private:
	std::vector<T> items;

public:
	void Add(T item);
	std::string MakeString(std::string seperator) override;
};


// Comment to linker to depend on this library
#pragma comment(lib, "wbemuuid.lib")

class WMIAccessor {
private:
	IWbemServices *pSvc;
	IWbemLocator *pLoc;

public:
	//"Root\" should not be part of wmiNamespace! 
	WMIAccessor() {};
	void Connect(const bstr_t& wmiNamespace);
	~WMIAccessor();

	//Executes: SELECT [wmiProperties] FROM [wmiClass] and returnes the result in a csv string
	std::wstring Query(const bstr_t& wmiClass, const bstr_t wmiProperties[], int arrayCount);
	std::vector<WMIDataItem> WMIAccessor::QueryItem(const bstr_t & wmiClass, const bstr_t wmiProperties[], const int arrayCount);
};

template<class T>
inline void DataCollection<T>::Add(T item)
{
	items.push_back(item);
}

template<class T>
inline std::string DataCollection<T>::MakeString(std::string seperator)
{
	throw std::runtime_error("Invalid specialization!");
}

template<>
inline std::string DataCollection<WMIDataItem>::MakeString(std::string seperator)
{
	std::stringstream result;

	//CSV headers:
	result << "id" << seperator;
	result << "Timestamp" << seperator;
	result << "FPS" << seperator;
	result << "ComponentType" << seperator;
	result << "ComponentID" << seperator;
	result << "SensorType" << seperator;
	result << "SensorID" << seperator;
	result << "Value" << std::endl;

	//Data:
	for (auto& item : items) {
		result << item.Id << seperator;
		result << item.Timestamp << seperator;
		result << item.FPS << seperator;
		result << item.ComponentType << seperator;
		result << item.ComponentID << seperator;
		result << item.SensorType << seperator;
		result << item.SensorID << seperator;
		result << item.Value << std::endl;
	}

	return result.str();
}

template<>
inline std::string DataCollection<PipelineStatisticsDataItem>::MakeString(std::string seperator)
{
	std::stringstream result;

	//csv headers:
	result << "CommandListId" << seperator;
	result << "CInvocations" << seperator;
	result << "CPrimitives" << seperator;
	result << "CSInvocations" << seperator;
	result << "DSInvocations" << seperator;
	result << "GSInvocations" << seperator;
	result << "GSPrimitives" << seperator;
	result << "HSInvocations" << seperator;
	result << "IAPrimitives" << seperator;
	result << "IAVertices" << seperator;
	result << "PSInvocations" << seperator;
	result << "VSInvocations" << std::endl;

	//data:
	for (auto& item : items) {
		result << item.CommandListId << seperator;
		result << item.CInvocations << seperator;
		result << item.CPrimitives << seperator;
		result << item.CSInvocations << seperator;
		result << item.DSInvocations << seperator;
		result << item.GSInvocations << seperator;
		result << item.GSPrimitives << seperator;
		result << item.HSInvocations << seperator;
		result << item.IAPrimitives << seperator;
		result << item.IAVertices << seperator;
		result << item.PSInvocations << seperator;
		result << item.VSInvocations << std::endl;
	}

	return result.str();
}

void Arrange_OHM_Data(const std::string* dataArr, WMIDataItem* item);