#pragma once

#include "table.hpp"
#include "../io/textEncodingTypes.hpp"
#include "../io/EndOfLineTypes.hpp"
#include "../io/DelimitedTextReader.hpp"
#include "../io/DelimitedTextWriter.hpp"
#include "../io/fileToByteContainer.hpp"
#include "../ReadOnlySzParam.hpp"
#include "../io/IfStream.hpp"
#include "../io.hpp"
#include "../io/ImStreamWithEncoding.hpp"
#include "../utf.hpp"
#include "MapVector.hpp"
#include <unordered_map>
#include "CsvConfig.hpp"
#include "../str/stringLiteralCharToValue.hpp"
#include "../io/IfmmStream.hpp"

DFG_ROOT_NS_BEGIN{ 
    
    class DFG_CLASS_NAME(CsvFormatDefinition)
    {
    public:
        //DFG_CLASS_NAME(CsvFormatDefinition)(const char cSep = ::DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::s_nMetaCharAutoDetect, const char cEnc = '"', DFG_MODULE_NS(io)::EndOfLineType eol = DFG_MODULE_NS(io)::EndOfLineTypeN) :
        // Note: default values are questionable because default read vals should have metachars, but default write vals should not.
        DFG_CLASS_NAME(CsvFormatDefinition)(const char cSep/* = ::DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::s_nMetaCharAutoDetect*/,
                                            const char cEnc/* = '"'*/,
                                            DFG_MODULE_NS(io)::EndOfLineType eol/* = DFG_MODULE_NS(io)::EndOfLineTypeN*/,
                                            DFG_MODULE_NS(io)::TextEncoding encoding/* = DFG_MODULE_NS(io)::encodingUTF8*/) :
            m_cSep(cSep),
            m_cEnc(cEnc),
            m_eolType(eol),
            m_textEncoding(encoding),
            m_enclosementBehaviour(DFG_MODULE_NS(io)::EbEncloseIfNeeded),
            m_bWriteHeader(true),
            m_bWriteBom(true)
        {}

        // Reads properties from given config, items not present in config are not modified.
        void fromConfig(const DFG_MODULE_NS(cont)::DFG_CLASS_NAME(CsvConfig)& config);

        void appendToConfig(DFG_MODULE_NS(cont)::DFG_CLASS_NAME(CsvConfig)& config) const;

        template <class Str_T>
        static Str_T csvFilePathToConfigFilePath(const Str_T& str)
        {
            // Note: changing this would likely require changes at least to file extension filters in qt-module.
            return str + ".conf";
        }

        int32 separatorChar() const { return m_cSep; }
        void separatorChar(int32 cSep) { m_cSep = cSep; }
        int32 enclosingChar() const { return m_cEnc; }
        void enclosingChar(int32 cEnc) { m_cEnc = cEnc; }
        int32 eolCharFromEndOfLineType() const { return ::DFG_MODULE_NS(io)::eolCharFromEndOfLineType(eolType()); }

        DFG_MODULE_NS(io)::EndOfLineType eolType() const { return m_eolType; }
        void eolType(DFG_MODULE_NS(io)::EndOfLineType eolType) { m_eolType = eolType; }

        bool headerWriting() const { return m_bWriteHeader; }
        void headerWriting(bool bWriteHeader) { m_bWriteHeader = bWriteHeader; }

        bool bomWriting() const { return m_bWriteBom; }
        void bomWriting(bool bWriteBom) { m_bWriteBom = bWriteBom; }

        DFG_MODULE_NS(io)::TextEncoding textEncoding() const { return m_textEncoding; }
        void textEncoding(DFG_MODULE_NS(io)::TextEncoding encoding) { m_textEncoding = encoding; }

        void setProperty(const DFG_CLASS_NAME(StringViewC)& svKey, const DFG_CLASS_NAME(StringViewC)& svValue)
        {
            m_genericProperties[svKey.toString()] = svValue.toString();
        }

        std::string getProperty(const DFG_CLASS_NAME(StringViewC)& svKey, const DFG_CLASS_NAME(StringViewC)& defaultValue) const
        {
            auto iter = m_genericProperties.find(svKey);
            return (iter != m_genericProperties.cend()) ? iter->second : defaultValue.toString();
        }

        bool hasProperty(const DFG_CLASS_NAME(StringViewC)& svKey) const
        {
            return m_genericProperties.hasKey(svKey);
        }

        DFG_MODULE_NS(io)::EnclosementBehaviour enclosementBehaviour() const { return m_enclosementBehaviour; }
        void enclosementBehaviour(const DFG_MODULE_NS(io)::EnclosementBehaviour eb) { m_enclosementBehaviour = eb; }

        int32 m_cSep;
        int32 m_cEnc;
        //int32 m_cEol;
        DFG_MODULE_NS(io)::EndOfLineType m_eolType;
        DFG_MODULE_NS(io)::TextEncoding m_textEncoding;
        DFG_MODULE_NS(io)::EnclosementBehaviour m_enclosementBehaviour; // Affects only writing.
        bool m_bWriteHeader;
        bool m_bWriteBom;
        ::DFG_MODULE_NS(cont)::MapVectorAoS<std::string, std::string> m_genericProperties; // Generic properties (e.g. if implementation needs specific flags)
    };

    inline void DFG_CLASS_NAME(CsvFormatDefinition)::fromConfig(const DFG_MODULE_NS(cont)::DFG_CLASS_NAME(CsvConfig)& config)
    {
        // Encoding
        {
            auto p = config.valueStrOrNull(DFG_UTF8("encoding"));
            if (p)
                textEncoding(DFG_MODULE_NS(io)::strIdToEncoding(p->c_str().rawPtr()));
        }

        // Enclosing char
        {
            auto p = config.valueStrOrNull(DFG_UTF8("enclosing_char"));
            if (p)
            {
                if (p->empty())
                    enclosingChar(DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::s_nMetaCharNone);
                else
                {
                    auto rv = DFG_MODULE_NS(str)::stringLiteralCharToValue<int32>(p->rawStorage());
                    if (rv.first)
                        enclosingChar(rv.second);
                }
            }
        }

        // Separator char
        {
            auto p = config.valueStrOrNull(DFG_UTF8("separator_char"));
            if (p)
            {
                auto rv = DFG_MODULE_NS(str)::stringLiteralCharToValue<int32>(p->rawStorage());
                if (rv.first)
                    separatorChar(rv.second);
            }
        }

        // EOL-type
        {
            auto p = config.valueStrOrNull(DFG_UTF8("end_of_line_type"));
            if (p)
                eolType(DFG_MODULE_NS(io)::endOfLineTypeFromStr(p->rawStorage()));
        }

        // BOM writing
        {
            auto p = config.valueStrOrNull(DFG_UTF8("bom_writing"));
            if (p)
                bomWriting(DFG_MODULE_NS(str)::strToByNoThrowLexCast<bool>(p->rawStorage()));
        }

        // Properties
        {
            config.forEachStartingWith(DFG_UTF8("properties/"), [&](const DFG_MODULE_NS(cont)::DFG_CLASS_NAME(CsvConfig)::StringViewT& relativeUri, const DFG_MODULE_NS(cont)::DFG_CLASS_NAME(CsvConfig)::StringViewT& value)
            {
                setProperty(relativeUri, value);
            });
        }
    }

    inline void DFG_CLASS_NAME(CsvFormatDefinition)::appendToConfig(DFG_MODULE_NS(cont)::DFG_CLASS_NAME(CsvConfig)& config) const
    {
        // Encoding
        {
            auto psz = DFG_MODULE_NS(io)::encodingToStrId(m_textEncoding);
            if (!DFG_MODULE_NS(str)::isEmptyStr(psz))
                config.setKeyValue(DFG_UTF8("encoding"), SzPtrUtf8(DFG_MODULE_NS(io)::encodingToStrId(m_textEncoding)));
        }

        
        // Enclosing char
        {
            if (m_cEnc == DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::s_nMetaCharNone)
                config.setKeyValue(DFG_UTF8("enclosing_char"), DFG_UTF8(""));
            else if (!DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::isMetaChar(m_cEnc) && m_cEnc >= 0)
            {
                char buffer[32];
                DFG_MODULE_NS(str)::DFG_DETAIL_NS::sprintf_s(buffer, sizeof(buffer), "\\x%x", m_cEnc);
                config.setKeyValue(DFG_UTF8("enclosing_char"), SzPtrUtf8(buffer));
            }
        }

        // Separator char
        if (!DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::isMetaChar(m_cSep) && m_cSep >= 0)
        {
            char buffer[32];
            DFG_MODULE_NS(str)::DFG_DETAIL_NS::sprintf_s(buffer, sizeof(buffer), "\\x%x", m_cSep);
            config.setKeyValue(DFG_UTF8("separator_char"), SzPtrUtf8(buffer));
        }

        // EOL-type
        {
            const auto psz = DFG_MODULE_NS(io)::eolLiteralStrFromEndOfLineType(m_eolType);
            if (!DFG_MODULE_NS(str)::isEmptyStr(psz))
                config.setKeyValue(DFG_UTF8("end_of_line_type"), SzPtrUtf8(psz.c_str()));
        }

        // BOM writing
        {
            config.setKeyValue(DFG_UTF8("bom_writing"), (m_bWriteBom) ? DFG_UTF8("1") : DFG_UTF8("0"));
        }

        // Properties
        // TODO: Generic properties are std::string so must figure out raw bytes to UTF-8 conversion. In practice consider changing properties to UTF-8
        {
            //m_genericProperties
        }
    }
    
    DFG_SUB_NS(cont)
    {

        template <class Char_T, class Index_T, DFG_MODULE_NS(io)::TextEncoding InternalEncoding_T = DFG_MODULE_NS(io)::encodingUTF8>
        class DFG_CLASS_NAME(TableCsv) : public DFG_CLASS_NAME(TableSz)<Char_T, Index_T, InternalEncoding_T>
        {
        public:
            typedef DFG_ROOT_NS::DFG_CLASS_NAME(CsvFormatDefinition) CsvFormatDefinition;
            typedef typename DFG_CLASS_NAME(TableSz)<Char_T, Index_T>::ColumnIndexPairContainer ColumnIndexPairContainer;
            typedef DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::CharBuffer<char> DelimitedTextReaderBufferTypeC;
            typedef DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::CharBuffer<Char_T> DelimitedTextReaderBufferTypeT;

            DFG_CLASS_NAME(TableCsv)()
                : m_readFormat(',', '"', DFG_MODULE_NS(io)::EndOfLineTypeN, DFG_MODULE_NS(io)::encodingUTF8)
                , m_saveFormat(m_readFormat)
            {}

            // TODO: test
            bool isContentAndSizesIdenticalWith(const DFG_CLASS_NAME(TableCsv)& other) const
            {
                const auto nRows = this->rowCountByMaxRowIndex();
                const auto nCols = this->colCountByMaxColIndex();
                if (nRows != other.rowCountByMaxRowIndex() || nCols != other.colCountByMaxColIndex())
                    return false;
                
                // TODO: optimize, this is quickly done, inefficient implementation.
                for (Index_T r = 0; r < nRows; ++r)
                {
                    for (Index_T c = 0; c < nCols; ++c)
                    {
                        auto p0 = (*this)(r, c);
                        auto p1 = other(r, c);
						// TODO: revise logics: implementation below treats null and empty cells as different.
                        if ((!p0 && p1) || (p0 && !p1) || (std::strcmp(toCharPtr_raw(p0), toCharPtr_raw(p1)) != 0)) // TODO: Create comparison function instead of using strcmp().
                            return false;
                    }
                }
                return true;
            }
                

            void readFromFile(const DFG_CLASS_NAME(ReadOnlySzParamC)& sPath) { readFromFileImpl(sPath, defaultReadFormat()); }
            void readFromFile(const DFG_CLASS_NAME(ReadOnlySzParamW)& sPath) { readFromFileImpl(sPath, defaultReadFormat()); }
            void readFromFile(const DFG_CLASS_NAME(ReadOnlySzParamC)& sPath, const CsvFormatDefinition& formatDef) { readFromFileImpl(sPath, formatDef); }
            void readFromFile(const DFG_CLASS_NAME(ReadOnlySzParamW)& sPath, const CsvFormatDefinition& formatDef) { readFromFileImpl(sPath, formatDef); }

            template <class Char_T1>
            void readFromFileImpl(const DFG_CLASS_NAME(ReadOnlySzParam)<Char_T1>& sPath, const CsvFormatDefinition& formatDef)
            {
                bool bRead = false;
                try
                {
                    auto memMappedFile = DFG_MODULE_NS(io)::DFG_CLASS_NAME(FileMemoryMapped)(sPath);
                    readFromMemory(memMappedFile.data(), memMappedFile.size(), formatDef);
                    bRead = true;
                }
                catch (...)
                {}

                if (!bRead)
                {
                    DFG_MODULE_NS(io)::DFG_CLASS_NAME(IfStreamWithEncoding) istrm;
                    istrm.open(sPath);
                    read(istrm, formatDef);
                    m_readFormat.textEncoding(istrm.encoding());
                    m_saveFormat = m_readFormat;
                }
            }

            CsvFormatDefinition defaultReadFormat()
            {
                return CsvFormatDefinition(DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::s_nMetaCharAutoDetect,
                                                            '"',
                                                            DFG_MODULE_NS(io)::EndOfLineTypeN,
                                                            DFG_MODULE_NS(io)::encodingUnknown);
            }

            CsvFormatDefinition readFormat() const
            {
                return m_readFormat;
            }

            CsvFormatDefinition saveFormat() const
            {
                return m_saveFormat;
            }

            void saveFormat(const CsvFormatDefinition& newFormat)
            {
                m_saveFormat = newFormat;
            }

            void readFromMemory(const char* const pData, const size_t nSize)
            {
                readFromMemory(pData, nSize, this->defaultReadFormat());
            }

            void readFromMemory(const char* const pData, const size_t nSize, const CsvFormatDefinition& formatDef)
            {
                DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream) strmBom(pData, nSize);
                const auto streamBom = DFG_MODULE_NS(io)::checkBOM(strmBom);
                const auto encoding = (formatDef.textEncoding() == DFG_MODULE_NS(io)::encodingUnknown) ? streamBom : formatDef.textEncoding();
                
                if (encoding == DFG_MODULE_NS(io)::encodingUnknown)
                {
                    // Encoding of source bytes is unknown -> read as Latin-1.
                    DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream) strm(pData, nSize);
                    read(strm, formatDef);
                }
                else if (encoding == DFG_MODULE_NS(io)::encodingUTF8) // With UTF8 the data can be directly read as bytes.
                {
                    const auto bomSkip = (streamBom == DFG_MODULE_NS(io)::encodingUTF8) ? DFG_MODULE_NS(utf)::bomSizeInBytes(DFG_MODULE_NS(io)::encodingUTF8) : 0;
                    DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream) strm(pData + bomSkip, nSize - bomSkip);
                    if (formatDef.enclosingChar() == DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::s_nMetaCharNone) // If there's no enclosing character, data can be read with StringViewBuffer.
                        read(strm, formatDef, DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::CharAppenderStringViewCBuffer());
                    else // Case: Enclosing character is defined, use default reading since parsing enclosing items may introduce translation making StringViewBuffer unsuitable.
                        read(strm, formatDef, DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::CharAppenderDefault<DelimitedTextReaderBufferTypeC, char>());
                }
                else // Case: Known encoding, read using encoding istream.
                {
                    DFG_MODULE_NS(io)::DFG_CLASS_NAME(ImStreamWithEncoding) strm(pData, nSize, encoding);
                    read(strm, formatDef);
                }
                m_readFormat.textEncoding(encoding);
                m_saveFormat = m_readFormat;
            }

            template <class Strm_T>
            void read(Strm_T& strm, const DFG_CLASS_NAME(CsvFormatDefinition)& formatDef)
            {
                using namespace DFG_MODULE_NS(io);
                read(strm, formatDef, DFG_CLASS_NAME(DelimitedTextReader)::CharAppenderUtf<DelimitedTextReaderBufferTypeC>());
            }

            template <class Strm_T, class CharAppender_T>
            void read(Strm_T& strm, const DFG_CLASS_NAME(CsvFormatDefinition)& formatDef, CharAppender_T)
            {
                using namespace DFG_MODULE_NS(io);
                this->clear();

                auto cellHandler = [=](const size_t nRow, const size_t nCol, const Char_T* pData, const size_t nCount)
                {
                    DFG_STATIC_ASSERT(InternalEncoding_T == DFG_MODULE_NS(io)::encodingUTF8, "Implimentation exists only for UTF8-encoding");
                    // TODO: this effectively assumes that user given input is valid UTF8.
                    this->setElement(nRow, nCol, DFG_CLASS_NAME(StringViewUtf8)(TypedCharPtrUtf8R(pData), nCount));
                };
                typedef DFG_CLASS_NAME(DelimitedTextReader)::ParsingDefinition<char, CharAppender_T> ParseDef;
                const auto& readFormat = DFG_CLASS_NAME(DelimitedTextReader)::readEx(ParseDef(), strm, formatDef.separatorChar(), formatDef.enclosingChar(), formatDef.eolCharFromEndOfLineType(), cellHandler);

                m_readFormat.separatorChar(readFormat.getSep());
                m_readFormat.enclosingChar(readFormat.getEnc());
                if (formatDef.eolType() == DFG_MODULE_NS(io)::EndOfLineTypeRN)
                    m_readFormat.eolType(DFG_MODULE_NS(io)::EndOfLineTypeRN);
                else if (formatDef.eolType() == DFG_MODULE_NS(io)::EndOfLineTypeR)
                    m_readFormat.eolType(DFG_MODULE_NS(io)::EndOfLineTypeR);
                else
                    m_readFormat.eolType(DFG_MODULE_NS(io)::EndOfLineTypeN);
                //m_readFormat.endOfLineChar(readFormat.getEol());
                //m_readFormat.textEncoding(strm.encoding()); // This is set 
                //m_readFormat.headerWriting(); //
                //m_readFormat.bomWriting(); // TODO: This is should be enquiried from the stream whether the stream had BOM.
                m_saveFormat = m_readFormat;
            }

            template <class Stream_T>
            class WritePolicySimple
            {
            public:
                WritePolicySimple(const CsvFormatDefinition& csvFormat) :
                    m_format(csvFormat)
                {
                    const auto cSep = m_format.separatorChar();
                    const std::string sEol = DFG_MODULE_NS(io)::eolStrFromEndOfLineType(m_format.eolType());
                    const auto encoding = m_format.textEncoding();
                    DFG_MODULE_NS(utf)::cpToEncoded(cSep, std::back_inserter(m_bytes), encoding);
                    m_nEncodedSepSizeInBytes = static_cast<decltype(m_nEncodedSepSizeInBytes)>(m_bytes.size());
                    for (auto iter = sEol.cbegin(), iterEnd = sEol.cend(); iter != iterEnd; ++iter)
                        DFG_MODULE_NS(utf)::cpToEncoded(*iter, std::back_inserter(m_bytes), encoding);
                    m_nEncodedEolSizeInBytes = static_cast<decltype(m_nEncodedEolSizeInBytes)>(m_bytes.size()) - m_nEncodedSepSizeInBytes;
                    // Note: set the pointers after all bytes have been written to m_bytes to make sure that 
                    //       there will be no pointer invalidating reallocation.
                    m_pEncodedSep = ptrToContiguousMemory(m_bytes);
                    m_pEncodedEol = m_pEncodedSep + m_nEncodedSepSizeInBytes;
                }

                void writeItemFunc(Stream_T& strm, int c)
                {
                    m_workBytes.clear();
                    DFG_MODULE_NS(utf)::cpToEncoded(c, std::back_inserter(m_workBytes), m_format.textEncoding());
                    DFG_MODULE_NS(io)::writeBinary(strm, ptrToContiguousMemory(m_workBytes), m_workBytes.size());
                }

                void write(Stream_T& strm, const char* pData, const Index_T/*nRow*/, const Index_T/*nCol*/)
                {
                    using namespace DFG_MODULE_NS(io);
                    if (pData == nullptr)
                        return;
                    utf8::unchecked::iterator<const char*> inputIter(pData);
                    utf8::unchecked::iterator<const char*> inputIterEnd(pData + std::strlen(pData));
                    DFG_CLASS_NAME(DelimitedTextCellWriter)::writeCellFromStrIter(strm,
                                                                                 makeRange(inputIter, inputIterEnd),
                                                                                 uint32(m_format.separatorChar()),
                                                                                 uint32(m_format.enclosingChar()),
                                                                                 uint32(eolCharFromEndOfLineType(m_format.eolType())),
                                                                                 m_format.enclosementBehaviour(),
                                                                                 [&](Stream_T& strm, int c) {this->writeItemFunc(strm, c); });
                }

                void writeSeparator(Stream_T& strm, const Index_T /*nRow*/, const Index_T /*nCol*/)
                {
                    DFG_MODULE_NS(io)::writeBinary(strm, m_pEncodedSep, m_nEncodedSepSizeInBytes);
                }

                void writeEol(Stream_T& strm)
                {
                    DFG_MODULE_NS(io)::writeBinary(strm, m_pEncodedEol, m_nEncodedEolSizeInBytes);
                }

                void writeBom(Stream_T& strm)
                {
                    if (m_format.bomWriting())
                    {
                        const auto bomBytes = DFG_MODULE_NS(utf)::encodingToBom(m_format.textEncoding());
                        strm.write(bomBytes.data(), bomBytes.size());
                    }
                }

                DFG_CLASS_NAME(CsvFormatDefinition) m_format;
                std::string m_bytes;
                std::string m_workBytes;
                const char* m_pEncodedSep;
                const char* m_pEncodedEol;
                uint32 m_nEncodedSepSizeInBytes;
                uint32 m_nEncodedEolSizeInBytes;
            }; // class WritePolicySimple

            template <class Stream_T>
            auto createWritePolicy() const -> WritePolicySimple<Stream_T>
            {
                return WritePolicySimple<Stream_T>(m_saveFormat);
            }

            template <class Stream_T>
            static auto createWritePolicy(DFG_CLASS_NAME(CsvFormatDefinition) format) -> WritePolicySimple<Stream_T>
            {
                return WritePolicySimple<Stream_T>(format);
            }
            
            // Strm must have write()-method which writes bytes untranslated.
            //  TODO: test writing non-square data.
            template <class Strm_T, class Policy_T>
            void writeToStream(Strm_T& strm, Policy_T& policy) const
            {
                policy.writeBom(strm);

                if (this->m_colToRows.empty())
                    return;
                // nextColItemRowIters[i] is the valid iterator to the next row entry in column i.
                std::unordered_map<Index_T, typename ColumnIndexPairContainer::const_iterator> nextColItemRowIters;
                this->forEachFwdColumnIndex([&](const Index_T nCol)
                {
                    if (isValidIndex(this->m_colToRows, nCol) && !this->m_colToRows[nCol].empty())
                        nextColItemRowIters[nCol] = this->m_colToRows[nCol].cbegin();
                });
                const auto nMaxColCount = this->colCountByMaxColIndex();
                for (Index_T nRow = 0; !nextColItemRowIters.empty(); ++nRow)
                {
                    for (Index_T nCol = 0; nCol < nMaxColCount; ++nCol)
                    {
                        auto iter = nextColItemRowIters.find(nCol);
                        if (iter != nextColItemRowIters.end() && iter->second->first == nRow) // Case: (row, col) has item
                        {
                            auto& rowEntryIter = iter->second;
                            const auto pData = rowEntryIter->second;
                            policy.write(strm, pData, rowEntryIter->first, nCol);
                            ++rowEntryIter;
                            if (rowEntryIter == this->m_colToRows[nCol].cend())
                                nextColItemRowIters.erase(iter);
                        }
                        if (nCol + 1 < nMaxColCount) // Write separator for all but the last column.
                            policy.writeSeparator(strm, nRow, nCol);
                    }
                    if (!nextColItemRowIters.empty()) // Don't write eol after last line. TODO: make this customisable.
                        policy.writeEol(strm);
                }
            }

            // Convenience overload, see implementation version for comments.
            template <class Strm_T>
            void writeToStream(Strm_T& strm) const
            {
                auto policy = createWritePolicy<Strm_T>();
                writeToStream(strm, policy);
            }

            DFG_CLASS_NAME(CsvFormatDefinition) m_readFormat; // Stores the format of previously read input. If no read is done, stores to default output format.
                                                              // TODO: specify content in case of interrupted read.
            DFG_CLASS_NAME(CsvFormatDefinition) m_saveFormat; // Format to be used when saving
        }; // class CsvTable

} } // module namespace
