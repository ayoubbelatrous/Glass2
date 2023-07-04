#include "pch.h"

#include "FrontEnd/CompilerFile.h"

namespace Glass {

	CompilerFile::CompilerFile(u64 id, const std::string& source, const fs_path& path)
		: m_ID(id), m_Source(source), m_Path(path)
	{}

	u64 CompilerFile::GetID() const
	{
		return m_ID;
	}

	const std::string& CompilerFile::GetSource() const
	{
		return m_Source;
	}

	const fs_path& CompilerFile::GetPath() const
	{
		return m_Path;
	}
}