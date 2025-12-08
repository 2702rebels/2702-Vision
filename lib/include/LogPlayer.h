#pragma once
#include "frmprocessor.h"
#include "VisionLog.h"

class LogPlayer : public VisionLog
{
	FrmProcessor	&m_fp;
	VisionLogReader m_logReader;
public:
	LogPlayer( fs::path logfilePath, FrmProcessor &fp );

	bool bNextFrame();
	VisionLogReader& GetLogReader();
	void vRewind();
};
