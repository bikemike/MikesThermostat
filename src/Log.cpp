#include "Log.h"

void Log::addLine(const String& line)
{
	logSize += line.length();
	logLines.push_back(std::make_pair(current_id++,line));
	while (logSize > maxSize)
	{
		logSize -= logLines.front().second.length();
		logLines.pop_front();
	}
}

String Log::getLines(uint32_t from_idx) const
{
	String lines;
	for (const auto &line : logLines )
	{
		if (line.first >= from_idx)
		{
			lines += line.second;
			lines += "\n";
		}
	}
	return lines;
}