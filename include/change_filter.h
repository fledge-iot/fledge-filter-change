#ifndef _CHANGE_FILTER_H
#define _CHANGE_FILTER_H
/*
 * Fledge change filter plugin.
 *
 * Copyright (c) 2019 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Mark Riddoch           
 */     
#include <filter.h>               
#include <reading_set.h>
#include <config_category.h>
#include <string>                 
#include <logger.h>
#include <list>
#include <vector>
#include <mutex>

/**
 * A filter used to only send information about an asset onwards when a
 * particular datapoint within that asset changes by more than a configured
 * percentage. Data is sent for a period of time before and after the
 * change in the monitored value. The amount of data to send before and
 * after the change is configured in milliseconds, with a value for the
 * pre-change tiem and one for the post-change time.
 *
 * This filter only operates on a single asset, all other assets are passed
 * through the filter unaltered.
 */
class ChangeFilter : public FledgeFilter {
	public:
		ChangeFilter(const std::string& filterName,
                        ConfigCategory& filterConfig,
                        OUTPUT_HANDLE *outHandle,
                        OUTPUT_STREAM out);
		~ChangeFilter();
		const std::string
			getName() const
			{
				return m_name;
			}
		void	setAsset(const std::string& asset)
			{
				m_asset = asset;
			};
		void	setTrigger(const std::string& datapoint)
			{
				m_trigger = datapoint;
			};
		void	setChange(int change)
			{
				m_change = change;
			}
		void	setPreTrigger(int pretrigger)
			{
				m_preTrigger = pretrigger;
			}
		void	setPostTrigger(int posttrigger)
			{
				m_postTrigger = posttrigger;
			}
		void	ingest(std::vector<Reading *> *readings, std::vector<Reading *>& out);
		void	reconfigure(const std::string& newConfig);
	private:
		void	triggeredIngest(std::vector<Reading *> *readings, std::vector<Reading *>& out);
		void	untriggeredIngest(std::vector<Reading *> *readings, std::vector<Reading *>& out);
		void	sendPretrigger(std::vector<Reading *>& out);
		void	sendPretrigger(std::vector<Reading *>& out, Reading *trigger);
		void	bufferPretrigger(Reading *);
		void	addAverageReading(Reading *, std::vector<Reading *>& out);
		void	addDataPoint(const std::string&, double);
		Reading *averageReading(Reading *);
		void	clearAverage();
		bool	evaluate(Reading *);
		void 	handleConfig(const ConfigCategory& conf);
		const std::string	m_name;
		std::string		m_asset;
		std::string		m_trigger;
		int			m_change;
		int			m_preTrigger;
		int			m_postTrigger;
		struct timeval		m_rate;
		bool			m_state;
		double			m_prevValue;
		std::string		m_prevStrValue;
		std::list<Reading *>	m_buffer;
		struct timeval		m_stopTime;
		bool			m_pendingReconfigure;
		std::mutex		m_configMutex;
		int			m_averageCount;
		std::map<std::string, double>
					m_averageMap;
		struct timeval		m_lastSent;
};


#endif
