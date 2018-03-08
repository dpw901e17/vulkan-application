#include "WmiAccess.h"

void WMIAccessor::Connect(const bstr_t & wmiNamespace)
{
	HRESULT hres;

	// Step 1: --------------------------------------------------
	// Initialize COM. ------------------------------------------

	hres = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (FAILED(hres))
	{
		std::string msg = "Failed to initialize COM library. Error code = " + hres;
		throw std::runtime_error(msg);
		CoUninitialize();
	}

	// Step 3: ---------------------------------------------------
	// Obtain the initial locator to WMI -------------------------

	hres = CoCreateInstance(
		CLSID_WbemLocator, // CLSID associated with data and code used to create the object.
		0,					// pointer to the aggregate object IUnknown interface (can be NULL)
		CLSCTX_INPROC_SERVER, // makes sure this process runs in our c++ context
		IID_IWbemLocator,  // Interface of output parameter
		(LPVOID *)&pLoc); // OUTPUT: Pointer to pointer to data

	if (FAILED(hres))
	{
		std::string msg = "Failed to create IWbemLocator object. Err code = " + hres;
		throw std::runtime_error(msg);
		CoUninitialize();
	}

	// Step 4: -----------------------------------------------------
	// Connect to WMI through the IWbemLocator::ConnectServer method

	// Connect to the root\cimv2 namespace with
	// the current user and obtain pointer pSvc
	// to make IWbemServices calls.
	hres = pLoc->ConnectServer(
		_bstr_t("ROOT\\" + wmiNamespace), // Object path of WMI namespace
		NULL,                    // User name. NULL = current user
		NULL,                    // User password. NULL = current
		0,                       // Locale. NULL indicates current
		NULL,                    // Security flags.
		0,                       // Authority (for example, Kerberos)
		0,                       // Context object 
		&pSvc                    // pointer to IWbemServices proxy
	);

	if (FAILED(hres))
	{

		std::string msg = "Could not connect. Error code  = " + hres;
		throw std::runtime_error(msg);
		pLoc->Release();
		CoUninitialize();
	}
}

WMIAccessor::~WMIAccessor()
{
	if (pSvc) {
		pSvc->Release();
	}

	if (pLoc) {
		pLoc->Release();
	}

	CoUninitialize();
}

std::wstring WMIAccessor::Query(const bstr_t & wmiClass, const bstr_t wmiProperties[], int arrayCount)
{
	// Step 6: --------------------------------------------------
	// Use the IWbemServices pointer to make requests of WMI ----

	// For example, get the name of the operating system
	HRESULT hres;
	IEnumWbemClassObject* pEnumerator = NULL;

	bstr_t query = "SELECT ";
	bool isFirstRun = true;
	for (auto i = 0; i < arrayCount; ++i) {
		if (!isFirstRun) {
			query += ", ";
		}
		else {
			isFirstRun = false;
		}
		query += wmiProperties[i];
	}

	query += " FROM " + wmiClass;

	hres = pSvc->ExecQuery(
		bstr_t("WQL"),
		bstr_t(query),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&pEnumerator);

	if (FAILED(hres))
	{
		std::string msg = "Query for operating system name failed. Error code  = " + hres;
		throw std::runtime_error(msg);

		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
	}

	// Step 7: -------------------------------------------------
	// Get the data from the query in step 6 -------------------

	IWbemClassObject *pclsObj = NULL;
	std::wstring result;

	ULONG uReturn = 0;

	while (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
			&pclsObj, &uReturn);

		if (0 == uReturn)
		{
			break;
		}


		// Get the value of the Identifier property
		_variant_t vtProp;

		isFirstRun = true;
		for (auto i = 0; i < arrayCount; ++i) {
			auto p = wmiProperties[i];


			hr = pclsObj->Get(p,
				0,
				&vtProp,
				0, //<--- CIM_EMPTY
				0);

			//force type to be BSTR (if actual value is a number n, then this call makes it "n")
			vtProp.ChangeType(VT_BSTR);

			//append WMIA_OUTPUT_SEPERATOR only if we had previous data in result
			if (!isFirstRun) {
				result += WMIA_OUTPUT_SEPARATOR;
			}
			isFirstRun = false;

			result += vtProp.bstrVal;
			isFirstRun = false;

			VariantClear(&vtProp);
		}

		pclsObj->Release();
		result += L"\n";
	}

	pEnumerator->Release();
	return result;
}

std::vector<WMIDataItem> WMIAccessor::QueryItem(const bstr_t & wmiClass, const bstr_t wmiProperties[], const int arrayCount)
{
	std::vector<WMIDataItem> result;
	
	// Step 6: --------------------------------------------------
	// Use the IWbemServices pointer to make requests of WMI ----

	// For example, get the name of the operating system
	HRESULT hres;
	IEnumWbemClassObject* pEnumerator = NULL;

	bstr_t query = "SELECT ";
	bool isFirstRun = true;
	for (auto i = 0; i < arrayCount; ++i) {
		if (!isFirstRun) {
			query += ", ";
		}
		else {
			isFirstRun = false;
		}
		query += wmiProperties[i];
	}

	query += " FROM " + wmiClass;

	hres = pSvc->ExecQuery(
		bstr_t("WQL"),
		bstr_t(query),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&pEnumerator);

	if (FAILED(hres))
	{
		std::string msg = "Query for operating system name failed. Error code  = " + hres;
		throw std::runtime_error(msg);

		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
	}

	// Step 7: -------------------------------------------------
	// Get the data from the query in step 6 -------------------

	IWbemClassObject *pclsObj = NULL;

	ULONG uReturn = 0;

	while (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

		if (0 == uReturn)
		{
			break;
		}


		std::vector<std::string> propertyData;

		// Get the value of the Identifier property
		_variant_t vtProp;

		for (auto i = 0; i < arrayCount; ++i) {
			auto p = wmiProperties[i];


			hr = pclsObj->Get(p,
				0,
				&vtProp,
				0, //<--- CIM_EMPTY
				0);

			//force type to be BSTR (if actual value is a number n, then this call makes it "n")
			vtProp.ChangeType(VT_BSTR);
			std::string propertyDataVal = _bstr_t(vtProp.bstrVal);

			propertyData.push_back(propertyDataVal);

			VariantClear(&vtProp);
		}

		pclsObj->Release();
		WMIDataItem item;
		Arrange_OHM_Data(propertyData.data(), &item);
		result.push_back(item);
	}

	pEnumerator->Release();
	
	return result;
}

//Determines how the cells (items) in the database look (name = attribute/collumn in db, value = entry in cell)
void Arrange_OHM_Data(const std::string* dataArr, WMIDataItem* item)
{
	/*
	dataArr[0] is Identifyer (when called in this main).
	dataArr[1] is Value
	dataArr[2] is SensorType
	Since Identifyer has the general structure "/[component]/[compId]/[sensorType]/[sensorId]",
	then this can be split into db collumns (parts) like:  [component] | [compId] | [sensorId]
	*/

	//split Identifyer up as described above:
	std::vector<std::string> parts;
	std::string part = "";
	for (char c : dataArr[0]) {
		if (c == '/') {
			if (part != "") {
				parts.push_back(part);
				part = "";
			}
		}
		else {
			part += c;
		}
	}

	//add the last identifyed part:
	parts.push_back(part);

	item->ComponentType = parts[0];
	//some identifiers (/ram/data/[id]) only have 3 parts to them (missing the ComponentID)
	if (parts.size() % 4 == 0) {
		item->ComponentID = parts[1];
	}
	else {
		item->ComponentID = "N/A";
	}
	item->SensorID = parts[parts.size() - 1];
	item->Value = dataArr[1];
	item->SensorType = dataArr[2];
}