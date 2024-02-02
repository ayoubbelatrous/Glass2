#pragma once

namespace Glass
{
	struct COFF_File_Header
	{
		uint16_t machine;
		uint16_t number_of_sections;
		uint32_t time_date_stamp;
		uint32_t pointer_to_symbol_table;
		uint32_t number_of_symbols;
		uint16_t sizeo_of_optional_header;
		uint16_t characteristics;
	};

	struct COFF_Section_Header {
		u8 name[8];
		uint32_t virtual_size;
		uint32_t virtual_address;
		uint32_t size_of_raw_data;
		uint32_t pointer_to_raw_data;
		uint32_t pointer_to_relocs;
		uint32_t pointer_to_linenos;
		uint16_t relocation_count;
		uint16_t linenos_count;
		uint32_t characteristics;
	};

	struct COFF_Symbol {
		union {
			uint8_t  short_name[8];
			uint32_t long_name[2];
		};
		uint32_t value;
		int16_t  section_number;
		uint16_t type;
		uint8_t  storage_class;
		uint8_t  aux_symbols_count;
	};

	struct COFF_AuxSectionSymbol {
		uint32_t length;       // section length
		uint16_t reloc_count;  // number of relocation entries
		uint16_t lineno_count; // number of line numbers
		uint32_t checksum;     // checksum for communal
		int16_t  number;       // section number to associate with
		uint8_t  selection;    // communal selection type
		uint8_t  reserved;
		int16_t  high_bits;    // high bits of the section number
	};

	struct COFF_Relocation
	{
		uint32_t address;
		uint32_t symbol_table_idx;
		uint16_t type;
	};
}