#pragma once
#include "frmprocessor.h"
#include "VisLog.h"

class LogPlayer : public VisionLog
{
	FrmProcessor	m_fp;
public:
	LogPlayer( char const *pszFile, FrmProcessor &fp );
	virtual ~LogPlayer();

	void vRewind();
	bool bNextFrame();
};
