#include <gtest/gtest.h>
#include <plugin_api.h>
#include <config_category.h>
#include <filter_plugin.h>
#include <filter.h>
#include <string.h>
#include <string>
#include <rapidjson/document.h>
#include <reading.h>
#include <reading_set.h>
#include <logger.h>

using namespace std;
using namespace rapidjson;

extern "C"
{
	PLUGIN_INFORMATION *plugin_info();
	void plugin_ingest(void *handle,
					   READINGSET *readingSet);
	PLUGIN_HANDLE plugin_init(ConfigCategory *config,
							  OUTPUT_HANDLE *outHandle,
							  OUTPUT_STREAM output);
	int called = 0;

	void Handler(void *handle, READINGSET *readings)
	{
		called++;
		*(READINGSET **)handle = readings;
	}
};


TEST(CHANGE, configuration)
{
	// Test case : Configuration is working correctly

	PLUGIN_INFORMATION *info = plugin_info();
	ConfigCategory *config = new ConfigCategory("change", info->config);
	ASSERT_NE(config, (ConfigCategory *)NULL);
	config->setItemsValueFromDefault();
	ASSERT_EQ(config->itemExists("enable"), true);
	config->setValue("asset", "test");
	config->setValue("trigger", "test");
	config->setValue("enable", "true");

	ReadingSet *outReadings;
	void *handle = plugin_init(config, &outReadings, Handler);
	vector<Reading *> *readings = new vector<Reading *>;
	
	long testValue1 = 10;
	DatapointValue dpv1(testValue1);
	Datapoint *value1 = new Datapoint("test", dpv1);
	Reading *in1 = new Reading("test", value1);
	readings->push_back(in1);

	long testValue2 = 11;
	DatapointValue dpv2(testValue2);
	Datapoint *value2 = new Datapoint("test", dpv2);
	Reading *in2 = new Reading("test", value2);
	readings->push_back(in2);

	long testValue3 = 12;
	DatapointValue dpv3(testValue3);
	Datapoint *value3 = new Datapoint("test", dpv3);
	Reading *in3 = new Reading("test", value3);
	readings->push_back(in3);

	long testValue4 = 13;
	DatapointValue dpv4(testValue4);
	Datapoint *value4 = new Datapoint("test", dpv4);
	Reading *in4 = new Reading("test", value4);
	readings->push_back(in4);

	long testValue5 = 14;
	DatapointValue dpv5(testValue5);
	Datapoint *value5 = new Datapoint("test", dpv5);
	Reading *in5 = new Reading("test", value5);
	readings->push_back(in5);

	ReadingSet *readingSet = new ReadingSet(readings);
	plugin_ingest(handle, (READINGSET *)readingSet);

	vector<Reading *> results = outReadings->getAllReadings();
	ASSERT_EQ(results.size(), 5);
	
}


TEST(CHANGE, triggerNotMatched)
{
	// Test case : Trigger condition didn't match
	
	PLUGIN_INFORMATION *info = plugin_info();
	ConfigCategory *config = new ConfigCategory("change", info->config);
	ASSERT_NE(config, (ConfigCategory *)NULL);
	config->setItemsValueFromDefault();
	config->setValue("asset", "test");
	config->setValue("trigger", "test");
	config->setValue("change", "40");
	config->setValue("preTrigger", "2000");
	config->setValue("postTrigger", "2000");
	config->setValue("rate", "0");
	config->setValue("rateUnit", "per second");
	config->setValue("enable", "true");
	
	ReadingSet *outReadings;
	void *handle = plugin_init(config, &outReadings, Handler);

	// First Set of readings 
	vector<Reading *> readings1;
	long testValue1 = 11;
	DatapointValue dpv1(testValue1);
	Datapoint *value1 = new Datapoint("test", dpv1);
	Reading *in1 = new Reading("test", value1);
	readings1.push_back(in1);
	ReadingSet *readingSet1 = new ReadingSet(&readings1);

	plugin_ingest(handle, (READINGSET *)readingSet1);
	vector<Reading *> results1 = outReadings->getAllReadings();
	sleep(2);

	// Second Set of readings 
	vector<Reading *> readings2;
	long testValue2 = 12;
	DatapointValue dpv2(testValue2);
	Datapoint *value2 = new Datapoint("test", dpv2);
	Reading *in2 = new Reading("test", value2);
	readings2.push_back(in2);

	long testValue3 = 13;
	DatapointValue dpv3(testValue3);
	Datapoint *value3 = new Datapoint("test", dpv3);
	Reading *in3 = new Reading("test", value3);
	readings2.push_back(in3);
	ReadingSet *readingSet2 = new ReadingSet(&readings2);

	plugin_ingest(handle, (READINGSET *)readingSet2);
	vector<Reading *> results2 = outReadings->getAllReadings();
	sleep(2);
	ASSERT_EQ(results2.size(), 0); // trigger condition didn't meet

	// Third Set of readings 
	vector<Reading *> readings3;
	long testValue4 = 14;
	DatapointValue dpv4(testValue4);
	Datapoint *value4 = new Datapoint("test", dpv4);
	Reading *in4 = new Reading("test", value4);
	readings3.push_back(in4);
	ReadingSet *readingSet3 = new ReadingSet(&readings3);

	plugin_ingest(handle, (READINGSET *)readingSet3);
	vector<Reading *> results3 = outReadings->getAllReadings();
	ASSERT_EQ(results3.size(), 0); // trigger condition didn't meet

	sleep(1);
	
	// Fourth Set of readings 
	vector<Reading *> readings4;
	long testValue5 = 15;
	DatapointValue dpv5(testValue5);
	Datapoint *value5 = new Datapoint("test", dpv5);
	Reading *in5 = new Reading("test", value5);
	readings4.push_back(in5);
	ReadingSet *readingSet4 = new ReadingSet(&readings4);

	plugin_ingest(handle, (READINGSET *)readingSet4);
	vector<Reading *> results4 = outReadings->getAllReadings();
	ASSERT_EQ(results4.size(), 0);  // trigger condition didn't meet
}

TEST(CHANGE, PrePostTriggerData)
{
	// Test case : pre and post trigger readings count

	PLUGIN_INFORMATION *info = plugin_info();
	ConfigCategory *config = new ConfigCategory("change", info->config);
	ASSERT_NE(config, (ConfigCategory *)NULL);
	config->setItemsValueFromDefault();
	config->setValue("asset", "test");
	config->setValue("trigger", "test");
	config->setValue("change", "40");
	config->setValue("preTrigger", "2000");
	config->setValue("postTrigger", "2000");
	config->setValue("rate", "0");
	config->setValue("rateUnit", "per second");
	config->setValue("enable", "true");
	
	ReadingSet *outReadings;
	void *handle = plugin_init(config, &outReadings, Handler);

	// First Set of readings 
	vector<Reading *> readings1;
	long testValue1 = 11;
	DatapointValue dpv1(testValue1);
	Datapoint *value1 = new Datapoint("test", dpv1);
	Reading *in1 = new Reading("test", value1);
	readings1.push_back(in1);
	ReadingSet *readingSet1 = new ReadingSet(&readings1);

	plugin_ingest(handle, (READINGSET *)readingSet1);
	vector<Reading *> results1 = outReadings->getAllReadings();
	sleep(2);

	// Second Set of readings 
	vector<Reading *> readings2;
	long testValue2 = 12;
	DatapointValue dpv2(testValue2);
	Datapoint *value2 = new Datapoint("test", dpv2);
	Reading *in2 = new Reading("test", value2);
	readings2.push_back(in2);

	long testValue3 = 13;
	DatapointValue dpv3(testValue3);
	Datapoint *value3 = new Datapoint("test", dpv3);
	Reading *in3 = new Reading("test", value3);
	readings2.push_back(in3);
	ReadingSet *readingSet2 = new ReadingSet(&readings2);

	plugin_ingest(handle, (READINGSET *)readingSet2);
	vector<Reading *> results2 = outReadings->getAllReadings();
	sleep(2);
	ASSERT_EQ(results2.size(), 0); // trigger condition didn't meet

	// Third Set of readings 
	vector<Reading *> readings3;
	long testValue4 = 25;
	DatapointValue dpv4(testValue4);
	Datapoint *value4 = new Datapoint("test", dpv4);
	Reading *in4 = new Reading("test", value4);
	readings3.push_back(in4);
	ReadingSet *readingSet3 = new ReadingSet(&readings3);

	plugin_ingest(handle, (READINGSET *)readingSet3);
	vector<Reading *> results3 = outReadings->getAllReadings();
	ASSERT_EQ(results3.size(), 3); // trigger condition met - PreTrigger Data

	sleep(1);
	
	// Fourth Set of readings 
	vector<Reading *> readings4;
	long testValue5 = 26;
	DatapointValue dpv5(testValue5);
	Datapoint *value5 = new Datapoint("test", dpv5);
	Reading *in5 = new Reading("test", value5);
	readings4.push_back(in5);
	ReadingSet *readingSet4 = new ReadingSet(&readings4);

	plugin_ingest(handle, (READINGSET *)readingSet4);
	vector<Reading *> results4 = outReadings->getAllReadings();
	ASSERT_EQ(results4.size(), 1);  // Post Trigger Data
}

TEST(CHANGE, PrePostTriggerData2)
{
	// Test case : pre and post trigger readings count for different preTrigger and postTrigger values

	PLUGIN_INFORMATION *info = plugin_info();
	ConfigCategory *config = new ConfigCategory("change", info->config);
	ASSERT_NE(config, (ConfigCategory *)NULL);
	config->setItemsValueFromDefault();
	config->setValue("asset", "test");
	config->setValue("trigger", "test");
	config->setValue("change", "50");
	config->setValue("preTrigger", "2000");
	config->setValue("postTrigger", "1000");
	config->setValue("rate", "0");
	config->setValue("rateUnit", "per second");
	config->setValue("enable", "true");

	ReadingSet *outReadings;
	void *handle = plugin_init(config, &outReadings, Handler);

	// First Set of readings
	vector<Reading *> readings1;
	long testValue1 = 11;
	DatapointValue dpv1(testValue1);
	Datapoint *value1 = new Datapoint("test", dpv1);
	Reading *in1 = new Reading("test", value1);
	readings1.push_back(in1);
	ReadingSet *readingSet1 = new ReadingSet(&readings1);

	plugin_ingest(handle, (READINGSET *)readingSet1);
	vector<Reading *> results1 = outReadings->getAllReadings();
	sleep(2);

	// Second Set of readings
	vector<Reading *> readings2;
	long testValue2 = 12;
	DatapointValue dpv2(testValue2);
	Datapoint *value2 = new Datapoint("test", dpv2);
	Reading *in2 = new Reading("test", value2);
	readings2.push_back(in2);

	long testValue3 = 13;
	DatapointValue dpv3(testValue3);
	Datapoint *value3 = new Datapoint("test", dpv3);
	Reading *in3 = new Reading("test", value3);
	readings2.push_back(in3);
	ReadingSet *readingSet2 = new ReadingSet(&readings2);

	plugin_ingest(handle, (READINGSET *)readingSet2);
	vector<Reading *> results2 = outReadings->getAllReadings();
	ASSERT_EQ(results2.size(), 0); // trigger condition didn't meet
	sleep(2);

	// Third Set of readings
	vector<Reading *> readings3;
	long testValue4 = 25;
	DatapointValue dpv4(testValue4);
	Datapoint *value4 = new Datapoint("test", dpv4);
	Reading *in4 = new Reading("test", value4);
	readings3.push_back(in4);
	ReadingSet *readingSet3 = new ReadingSet(&readings3);

	plugin_ingest(handle, (READINGSET *)readingSet3);
	vector<Reading *> results3 = outReadings->getAllReadings();
	ASSERT_EQ(results3.size(), 3); // trigger condition met - PreTrigger Data

	// Fourth Set of readings
	vector<Reading *> readings4;
	long testValue5 = 26;
	DatapointValue dpv5(testValue5);
	Datapoint *value5 = new Datapoint("test", dpv5);
	Reading *in5 = new Reading("test", value5);
	readings4.push_back(in5);
	ReadingSet *readingSet4 = new ReadingSet(&readings4);

	plugin_ingest(handle, (READINGSET *)readingSet4);
	vector<Reading *> results4 = outReadings->getAllReadings();
	ASSERT_EQ(results4.size(), 1);  // Post Trigger Data
}

TEST(CHANGE, PrePostTriggerStringData)
{
	// Test case : String data point

	PLUGIN_INFORMATION *info = plugin_info();
	ConfigCategory *config = new ConfigCategory("change", info->config);
	ASSERT_NE(config, (ConfigCategory *)NULL);
	config->setItemsValueFromDefault();
	config->setValue("asset", "test");
	config->setValue("trigger", "test");
	config->setValue("change", "40");
	config->setValue("preTrigger", "1000");
	config->setValue("postTrigger", "1000");
	config->setValue("rate", "2");
	config->setValue("rateUnit", "per second");
	config->setValue("enable", "true");

	ReadingSet *outReadings;
	void *handle = plugin_init(config, &outReadings, Handler);

	vector<Reading *> readings;
	std::string testValue2 = "Floor1";
	DatapointValue dpv2(testValue2);
	Datapoint *value2 = new Datapoint("test", dpv2);
	Reading *in2 = new Reading("test", value2);
	readings.push_back(in2);

	std::string testValue3 = "Floor2";
	DatapointValue dpv3(testValue3);
	Datapoint *value3 = new Datapoint("test", dpv3);
	Reading *in3 = new Reading("test", value3);
	readings.push_back(in3);
	ReadingSet *readingSet = new ReadingSet(&readings);

	plugin_ingest(handle, (READINGSET *)readingSet);
	vector<Reading *> results = outReadings->getAllReadings();
	ASSERT_EQ(results.size(), 2); // trigger condition meet
}

TEST(CHANGE, average)
{
	// Test average value for Reduced collection rate

	// TODO : FOGL-8042 - Fix average functionality. Test case for average functionality will be added later
	
}
