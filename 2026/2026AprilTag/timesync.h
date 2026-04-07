#pragma once
#include <span>
#include <thread>
#include <vector>
#include <mutex>

class CTimeSync
{
	int		m_nTeamNum = 0;
	int		m_nPollMS = 0;
	int		m_nSock = -1;
	bool	m_bShutdown = false;
	uint32_t	m_u32RIOAddr = 0;

	int64_t	m_i64Offset = 0;

	uint8_t	m_u8MatchType = 0;
	uint8_t m_u8MatchNum = 0;

	std::thread m_thSync;

	void vSyncThread();
	void vBind();

	void vSendPing();
	void vHandlePong( uint8_t const *pu8, size_t sLen );

public:
	CTimeSync( int nTeamNum, int nPollMS = 10000 );
	~CTimeSync();

	inline int64_t i64Offset() const
	{
		return m_i64Offset;
	}

	inline uint8_t u8MatchType() const
	{
		return m_u8MatchType;
	}

	inline uint8_t u8MatchNum() const
	{
		return m_u8MatchNum;
	}
};
