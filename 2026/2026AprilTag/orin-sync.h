#pragma once
#include <span>
#include <thread>
#include <vector>
#include <mutex>

#include "mbox.h"
#include "ATFrame.h"

class CSecondary
{
	struct sockaddr_in	m_sa;
	bool				m_bCamStarted = false;
public:
	CSecondary( struct sockaddr_in const &sa )
	{
		m_sa = sa;
		m_bCamStarted = false;
	}
	CSecondary( CSecondary const &src )
	{
		m_sa			= src.m_sa;
		m_bCamStarted	= src.m_bCamStarted;
	}
	~CSecondary()
	{
	}

	CSecondary &operator=( CSecondary const &src )
	{
		m_sa			= src.m_sa;
		m_bCamStarted	= src.m_bCamStarted;
		return *this;
	}

	bool &bCamStarted()
	{
		return m_bCamStarted;
	}

	struct sockaddr_in &GetSockaddr()
	{
		return m_sa;
	}
};

class COrinSync
{
	int		m_nSock = -1;
	bool	m_bPrimary = false;
	int		m_nSecondaries = 0;
	bool	m_bShutdown = false;
	std::vector <CSecondary>  m_vSecondaries;
	std::thread	m_thSync;
	std::mutex	m_mtx;
	MailBox<ATFrame *>	&m_mbFrame;

	void vPrimaryWorker();
	void vSyncThread();

	void vBind();

public:
	COrinSync( bool bIsPrimary, MailBox<ATFrame *> &mbFrame, int nSecondaries );
	~COrinSync();

	bool bPrimarySync();

	bool bSecondarySync();
	//bool bWaitForPrimary();
	bool bSendCamStarted();

	bool bSend( std::span<uint8_t> const &spBuf );
};
