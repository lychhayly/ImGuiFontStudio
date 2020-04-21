/*
 * Copyright 2020 Stephane Cuillerdier (aka Aiekick)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "Project/GlyphInfos.h"

#include <string>
#include <set>
#include <map>
#include <unordered_map>

#include "sfntly/tag.h"
#include "sfntly/font.h"
#include "sfntly/port/type.h"
#include "sfntly/port/refcount.h"
#include "sfntly/table/core/cmap_table.h"
#include "sfntly/table/core/post_script_table.h"
#include "sfntly/table/truetype/glyph_table.h"
#include "sfntly/table/truetype/loca_table.h"

class MemoryStream
{
public:
	MemoryStream()= default;
	~MemoryStream()= default;

	void WriteByte(uint8_t b)
	{
		m_Datas.push_back(b);
	}
	void WriteBytes(std::vector<uint8_t> *buffer)
	{
		if (buffer)
		{
			m_Datas.insert(m_Datas.end(), buffer->begin(), buffer->end());
		}
	}
	void WriteShort(int32_t i)
	{
		WriteByte((uint8_t)((i >> 8) & 0xff));
		WriteByte((uint8_t)(i & 0xff));
	}
	void WriteInt(int32_t i)
	{
		WriteByte((uint8_t)((i >> 24) & 0xff));
		WriteByte((uint8_t)((i >> 16) & 0xff));
		WriteByte((uint8_t)((i >> 8) & 0xff));
		WriteByte((uint8_t)(i & 0xff));
	}
	uint8_t* Get()
	{
		return m_Datas.data();
	}
	size_t Size()
	{
		return m_Datas.size();
	}

private:
	std::vector<uint8_t> m_Datas;
};

typedef int32_t FontId;
typedef int32_t CodePoint;
typedef int32_t GlyphId;
typedef std::pair<FontId, GlyphId> FontGlyphId;
struct Glyph
{
	CodePoint codepoint = 0;
	GlyphId glyphid = 0;
	FontId fontId = 0;

	Glyph(CodePoint vCdp, GlyphId vGid, FontId vFid)
	{
		codepoint = vCdp;
		glyphid = vGid;
		fontId = vFid;
	}
};
typedef std::pair<int32_t, std::string> CodePointName;

class FontInstance
{
public:

public:
	sfntly::Ptr<sfntly::Font> m_Font;
	sfntly::Ptr<sfntly::CMapTable::CMap> m_CMapTable;
	sfntly::Ptr<sfntly::LocaTable> m_LocaTable;
	sfntly::Ptr<sfntly::GlyphTable> m_GlyfTable;
	std::map<CodePoint, int32_t> m_CharMap; // codepoint to glyph id
	std::map<int32_t, CodePoint> m_ReversedCharMap; // glyph id to codepoint
	std::set<int32_t> m_ResolvedSet;
	std::map<int32_t, std::string> m_NewGlyphNames;
	std::map<CodePoint, CodePoint> m_NewGlyphCodePoints;
	std::map<CodePoint, GlyphInfos> m_NewGlyphInfos;
};

class FontHelper
{
public:
	FontHelper();
	~FontHelper();

public:
	bool OpenFontFile(
		const std::string& vFontFilePathName,
		std::map<CodePoint, std::string> vNewNames,
		std::map<CodePoint, CodePoint> vNewCodePoints,
		std::map<CodePoint, GlyphInfos> vNewGlyphInfos,
		bool vBaseFontFileToMergeIn);
	bool GenerateFontFile(const std::string& vFontFilePathName, bool vUsePostTable);

private:
	size_t m_BaseFontIdx = 0;
	FontInstance* GetBaseFontInstance();

private:
	std::vector<FontInstance> m_Fonts;
	sfntly::Ptr<sfntly::FontFactory> m_FontFactory;
	sfntly::Ptr<sfntly::Font::Builder> m_FontBuilder;
	std::map<CodePoint, FontGlyphId> m_CharMap; // codepoint to glyph id
	std::map<FontGlyphId, CodePoint> m_ReversedCharMap; // glyph id to codepoint
	std::map<CodePoint, std::string> m_GlyphNames;
	std::set<FontGlyphId> m_ResolvedSet; // set of font id / glyph id
	std::map<FontGlyphId, GlyphId> m_OldToNewGlyfId;
	std::map<GlyphId, std::vector<GlyphId>> m_NewToOldGlyfId;

private: // post table - version / count / size / offsets
	const int32_t table_Version = 0x20000;
	const int32_t count_StandardNames = 258;
	const int32_t size_Header = 32;
	std::unordered_map<std::string, int32_t> m_InvertedStandardNames;
	static std::unordered_map<std::string, int32_t> InvertNameMap();
	int32_t MergeCharacterMaps();

public:
	static sfntly::Font* LoadFontFile(const char* font_path);

private: // imported/based or/modified from sfntly
	static void LoadFontFiles(const char* font_path, sfntly::FontFactory* factory, sfntly::FontArray* fonts);
	static bool SerializeFont(const char* font_path, sfntly::Font* font);
	static bool SerializeFont(const char* font_path, sfntly::FontFactory* factory, sfntly::Font* font);
	sfntly::Font* AssembleFont(bool vUsePostTable);

private:
	bool Assemble_Glyf_Loca_Maxp_Tables();
	sfntly::Ptr<sfntly::WritableFontData> ReScale_Glyph(const int32_t& vFontId, const int32_t& vGlyphId, const sfntly::Ptr<sfntly::ReadableFontData>& vReadableFontData);
	static void FillResolvedCompositeGlyphs(FontInstance *vFontInstance, const std::map<CodePoint, int32_t>& chars_to_glyph_ids);

private:
	bool Assemble_CMap_Table();
	static void FillCharacterMap(FontInstance *vFontInstance, std::map<CodePoint, std::string> vSelection);

private:
	bool Assemble_Hmtx_Hhea_Tables();

private:
	bool Assemble_Post_Table(std::map<CodePoint, std::string> vSelection);

private:
	static bool Assemble_Meta_Table();

private:
	static bool Assemble_Head_Table();

private:
	GlyphInfos* GetGlyphInfosFromGlyphId(int32_t vFontId, int32_t vGlyphId);
};

