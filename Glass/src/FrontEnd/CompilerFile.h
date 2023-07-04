#pragma once

namespace Glass
{
	class CompilerFile
	{
	public:
		CompilerFile() = default;
		CompilerFile(u64 id, const std::string& source, const fs_path& path);

		u64 GetID() const;
		const std::string& GetSource() const;
		const fs_path& GetPath() const;
	private:
		u64 m_ID = 0;
		std::string m_Source = "";
		fs_path m_Path = "";
	};
}