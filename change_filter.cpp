/*
 * FogLAMP "change" filter plugin.
 *
 * Copyright (c) 2019 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Mark Riddoch           
 */     
#include <reading.h>              
#include <reading_set.h>
#include <utility>                
#include <logger.h>
#include <change_filter.h>

using namespace std;
using namespace rapidjson;

/**
 * Construct a ChangeFilter, call the base class constructor and handle the
 * parsing of the configuration category the required change
 */
ChangeFilter::ChangeFilter(const std::string& filterName,
			       ConfigCategory& filterConfig,
                               OUTPUT_HANDLE *outHandle,
                               OUTPUT_STREAM out) :
                                  FogLampFilter(filterName, filterConfig,
                                                outHandle, out),
				  m_name(filterConfig.getName()), m_state(false)
{
	handleConfig(filterConfig);
}

/**
 * Destructor for the filter.
 */
ChangeFilter::~ChangeFilter()
{
}

/**
 * Called with a set of readings, iterate over the readings applying
 * the change filter to create the output readings
 *
 * @param readings	The readings to process
 * @param out		The output readings
 */
void ChangeFilter::ingest(vector<Reading *> *readings, vector<Reading *>& out)
{
	lock_guard<mutex> guard(m_configMutex);

	if (m_state)
	{
		triggeredIngest(readings, out);
	}
	else
	{
		untriggeredIngest(readings, out);
	}
}

/**
 * Called when in the triggered state to forward the readings until the timestamp
 * in the readign becomes greater than the stop time for the forwarding.
 *
 * @param readings	The readings to process
 * @param out		The output readings
 */
void ChangeFilter::triggeredIngest(vector<Reading *> *readings, vector<Reading *>& out)
{
int offset = 0;

	for (vector<Reading *>::const_iterator reading = readings->begin();
						      reading != readings->end();
						      ++reading)
	{
		if ((*reading)->getAssetName().compare(m_asset) == 0)
		{
			evaluate(*reading); 	// A change might occur that causes the m_stopTime to be updated
			struct timeval tm;
			(*reading)->getUserTimestamp(&tm);
			if ((tm.tv_sec > m_stopTime.tv_sec)
					|| (tm.tv_sec == m_stopTime.tv_sec && tm.tv_usec > m_stopTime.tv_usec))
			{
				Logger::getLogger()->debug("Reached the end of the triggered time");
				m_state = false;
				readings->erase(readings->begin(), readings->begin() + offset);
				return untriggeredIngest(readings, out);
			}
		}
		// We either have a different asset that should pass unaltered or
		// we have not reached the end of the post change time period
		out.push_back(*reading);
		offset++;
	}
	readings->clear();
}

/**
 * Called when in the triggered state to average the readings and evaluate the
 * trigger expression. If the state changes to untrigger then the triggerIngest
 * method will be called.
 *
 * @param readings	The readings to process
 * @param out		The output readings
 */
void ChangeFilter::untriggeredIngest(vector<Reading *> *readings, vector<Reading *>& out)
{
int	offset = 0;	// Offset within the vector

	for (vector<Reading *>::const_iterator reading = readings->begin();
						      reading != readings->end();
						      ++reading)
	{
		if ((*reading)->getAssetName().compare(m_asset) == 0)
		{
			if (evaluate(*reading))
			{
				m_state = true;
				clearAverage();
				// Remove the readings we have dealt with
				readings->erase(readings->begin(), readings->begin() + offset);
				sendPretrigger(out);
				Logger::getLogger()->debug("Send the preTrigger buffer");
				return triggeredIngest(readings, out);
			}
			bufferPretrigger(*reading);
			if (m_rate.tv_sec != 0 || m_rate.tv_usec != 0)
			{
				addAverageReading(*reading, out);
			}
			delete *reading;
		}
		else
		{
			out.push_back(*reading);
		}
		offset++;
	}
	readings->clear();
}

/**
 * If we have a preTrigger buffer defined in the configuration then
 * keep a copy of the reading in the pretrigger buffer. Remove any readings
 * that are older than the defined pretrigger age.
 *
 * @param reading	Reading to buffer
 */
void ChangeFilter::bufferPretrigger(Reading *reading)
{
Reading	*t;
struct timeval	now, t1, t2, res;

	if (m_preTrigger == 0)	// No pretrigger buffering
	{
		return;
	}
	m_buffer.push_back(new Reading(*reading));

	/*
	 * Remove the entries from the front of the pretrigger buffer taht are
	 * older than the pre trigger time.
	 */
	t2.tv_sec = m_preTrigger / 1000;
	t2.tv_usec = (m_preTrigger % 1000) * 1000;
	for (;;)
	{
		t = m_buffer.front();
		t->getUserTimestamp(&t1);
		reading->getUserTimestamp(&now);
		timersub(&now, &t1, &res);
		if (timercmp(&res, &t2, >))
		{
			t = m_buffer.front();
			delete t;
			m_buffer.pop_front();
		}
		else
		{
			break;
		}
	}
}

/**
 * Send the pretigger buffer data
 *
 * @param out	The output buffer
 */
void ChangeFilter::sendPretrigger(vector<Reading *>& out)
{
	while (!m_buffer.empty())
	{
		Reading *r = m_buffer.front();
		out.push_back(r);
		m_buffer.pop_front();
	}
}


/**
 * Add a reading to the average data. If the period has enxpired in which
 * to send a reading then the average will be calculated and added to the
 * out buffer.
 *
 * @param reading	The reading to add
 * @param out		The output buffer to add any average to.
 */
void ChangeFilter::addAverageReading(Reading *reading, vector<Reading *>& out)
{
	vector<Datapoint *>	datapoints = reading->getReadingData();
	for (auto it = datapoints.begin(); it != datapoints.end(); it++)
	{
		DatapointValue& dpvalue = (*it)->getData();
		if (dpvalue.getType() == DatapointValue::T_INTEGER)
		{
			addDataPoint((*it)->getName(), (double)dpvalue.toInt());
		}
		if (dpvalue.getType() == DatapointValue::T_FLOAT)
		{
			addDataPoint((*it)->getName(), dpvalue.toDouble());
		}
	}
	m_averageCount++;
	
	struct timeval t1, res;
	reading->getUserTimestamp(&t1);
	timeradd(&m_lastSent, &m_rate, &res);
	if (timercmp(&t1, &res, >))
	{
		out.push_back(averageReading(reading));
		m_lastSent = t1;
	}
}

/**
 * Add a data point to the average data
 *
 * @param name	The datapoint name
 * @param value	The datapoint value
 */
void ChangeFilter::addDataPoint(const string& name, double value)
{
	map<string, double>::iterator it = m_averageMap.find(name);
	if (it != m_averageMap.end())
	{
		it->second += value;
	}
	else
	{
		m_averageMap.insert(pair<string, double>(name, value));
	}
}

/**
 * Create a average reading using the asset name and times from the reading
 * passed in and the data accumulated in the average map
 *
 * @param reading	The reading to take the asset name and times from
 */
Reading *ChangeFilter::averageReading(Reading *templateReading)
{
string			asset = templateReading->getAssetName();
vector<Datapoint *>	datapoints;

	for (map<string, double>::iterator it = m_averageMap.begin();
				it != m_averageMap.end(); it++)
	{
		DatapointValue dpv(it->second / m_averageCount);
		it->second = 0.0;
		datapoints.push_back(new Datapoint(it->first, dpv));
	}
	m_averageCount = 0;
	Reading	*rval = new Reading(asset, datapoints);
	struct timeval tm;
	templateReading->getUserTimestamp(&tm);
	rval->setUserTimestamp(tm);
	templateReading->getTimestamp(&tm);
	rval->setTimestamp(tm);
	return rval;
}

/**
 * Clear the average data having triggered a change of state
 *
 */
void ChangeFilter::clearAverage()
{
	for (map<string, double>::iterator it = m_averageMap.begin();
				it != m_averageMap.end(); it++)
	{
		it->second = 0.0;
	}
}

bool ChangeFilter::evaluate(Reading *reading)
{
static bool firstCall = true;
double	value;
string  strValue;
bool	isString = false;
const std::vector<Datapoint *>  datapoints = reading->getReadingData();

	for (auto itr = datapoints.cbegin(); itr != datapoints.cend(); ++itr)
	{
		if ((*itr)->getName().compare(m_trigger) == 0)
		{
			if ((*itr)->getData().getType() == DatapointValue::T_INTEGER)
			{
				value = (*itr)->getData().toInt();
			}
			else if ((*itr)->getData().getType() == DatapointValue::T_FLOAT)
			{
				value = (*itr)->getData().toDouble();
			}
			else if ((*itr)->getData().getType() == DatapointValue::T_STRING)
			{
				strValue = (*itr)->getData().toString();
				isString = true;
			}
			else if (firstCall)
			{
				Logger::getLogger()->fatal(
					"Filter %s can not monitor changes on the asset %s, datapoint %s, it is not a simple value",
						m_name, m_asset.c_str(), m_trigger.c_str());
			}
			if (isString)
			{
				if (firstCall)
				{
					m_prevStrValue = strValue;
					firstCall = false;
				}
				else if (strValue.compare(m_prevStrValue))
				{
					// Triggered set to state and the stop time
					m_state = true;
					gettimeofday(&m_stopTime, NULL);
					m_stopTime.tv_usec += ((m_postTrigger % 1000) * 1000);
					m_stopTime.tv_sec += (m_postTrigger / 1000);
					m_prevStrValue = strValue;
				}
			}
			else
			{
				double tolarance = (m_prevValue * m_change) / 100;
				if (firstCall)
				{
					m_prevValue = value;
					firstCall = false;
				}
				else if ((m_change == 0 && m_prevValue != value) || fabs(m_prevValue - value) >= tolarance)
				{
					// Triggered set to state and the stop time
					m_state = true;
					gettimeofday(&m_stopTime, NULL);
					m_stopTime.tv_usec += ((m_postTrigger % 1000) * 1000);
					m_stopTime.tv_sec += (m_postTrigger / 1000);
					m_prevValue = value;
				}
			}
		}
	}
	if (m_state)
	{
		Logger::getLogger()->debug("Change filter %s has triggered", m_name.c_str());
	}
	return m_state;
}

/**
 * Handle a reconfiguration request
 *
 * @param newConfig	The new configuration
 */
void ChangeFilter::reconfigure(const string& newConfig)
{
	lock_guard<mutex> guard(m_configMutex);
	setConfig(newConfig);
	handleConfig(m_config);
	m_pendingReconfigure = true;
}


/**
 * Handle the configuration of the plugin.
 *
 *
 * @param conf	The configuration category for the filter.
 */
void ChangeFilter::handleConfig(const ConfigCategory& config)
{
	if (config.itemExists("asset"))
	{
		setAsset(config.getValue("asset"));
	}
	else
	{
		Logger::getLogger()->fatal("No configuration item named asset");
	}
	if (config.itemExists("trigger"))
	{
		setTrigger(config.getValue("trigger"));
	}
	else
	{
		Logger::getLogger()->fatal("No configuration item named trigger");
	}
	if (config.itemExists("change"))
	{
		m_change = strtol(config.getValue("change").c_str(), NULL, 10);
	}
	else
	{
		Logger::getLogger()->fatal("No configuration item named change");
	}
	if (config.itemExists("preTrigger"))
	{
		m_preTrigger = strtol(config.getValue("preTrigger").c_str(), NULL, 10);
	}
	else
	{
		Logger::getLogger()->fatal("No configuration item named preTrigger");
	}
	if (config.itemExists("postTrigger"))
	{
		m_postTrigger = strtol(config.getValue("postTrigger").c_str(), NULL, 10);
	}
	else
	{
		Logger::getLogger()->fatal("No configuration item named postTrigger");
	}

	if (config.itemExists("rate") && config.itemExists("rateUnit"))
	{
		int rate = strtol(config.getValue("rate").c_str(), NULL, 10);
		string unit = config.getValue("rateUnit");
		if (rate == 0)
		{
			m_rate.tv_sec = 0;
			m_rate.tv_usec = 0;
		}
		else if (unit.compare("per second") == 0)
		{
			m_rate.tv_sec = 0;
			m_rate.tv_usec = 1000000 / rate;
		}
		else if (unit.compare("per minute") == 0)
		{
			m_rate.tv_sec = 60 / rate;
			m_rate.tv_usec = 0;
		}
		else if (unit.compare("per hour") == 0)
		{
			m_rate.tv_sec = 3600 / rate;
			m_rate.tv_usec = 0;
		}
		else if (unit.compare("per day") == 0)
		{
			m_rate.tv_sec = (24 * 60 * 60) / rate;
			m_rate.tv_usec = 0;
		}
	}
	else
	{
		Logger::getLogger()->fatal("No configuration items named rate and rateUnit");
	}
}
