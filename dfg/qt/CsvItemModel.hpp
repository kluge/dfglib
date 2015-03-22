#pragma once

#include "../dfgDefs.hpp"
#include <dfg/buildConfig.hpp> // To get rid of C4996 "Function call with parameters that may be unsafe" in MSVC.
#include <unordered_set>
#include "qtIncludeHelpers.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QMessageBox>
#include <QAbstractTableModel>
#include <QTextStream>
DFG_END_INCLUDE_QT_HEADERS

#include "../io/DelimitedTextWriter.hpp"
#include <memory>

//class QUndoStack;
class QCompleter;
class QFile;
class QTextStream;

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{

#ifndef DFG_CSV_ITEM_MODEL_ENABLE_DRAG_AND_DROP_TESTS
	#define DFG_CSV_ITEM_MODEL_ENABLE_DRAG_AND_DROP_TESTS  0
#endif

	// TODO: test
	class DFG_CLASS_NAME(CsvItemModel) : public QAbstractTableModel
	{
		Q_OBJECT
	public:
		static const QString s_sEmpty;
		typedef QAbstractTableModel BaseClass;
		
		enum ColType
		{
			ColTypeText,
			ColTypeNumber,
			ColTypeDate
		};
		enum CompleterType
		{
			CompleterTypeNone,
			CompleterTypeTexts,
			CompleterTypeTextsListItems
		};

		struct ColInfo
		{
			ColInfo(QString sName = "", ColType type = ColTypeText, CompleterType complType = CompleterTypeNone) :
				m_name(sName), m_type(type), m_completerType(complType) {}
			QString m_name;
			ColType m_type;
			CompleterType m_completerType;
			std::shared_ptr<QCompleter> m_spCompleter;
		};

		class SaveOptions
		{
		public:
			SaveOptions(const char cSep = ',', const char cEnc = '"', DFG_MODULE_NS(io)::EndOfLineType eol = DFG_MODULE_NS(io)::EndOfLineTypeNative) :
				m_cSep(cSep),
				m_cEnc(cEnc),
				m_eolType(eol),
				m_bSaveHeader(true)
			{}

			char separatorChar() const { return m_cSep; }
			void separatorChar(char cSep) { m_cSep = cSep; }
			char enclosingChar() const { return m_cEnc; }
			void enclosingChar(char cEnc) { m_cEnc = cEnc; }
			DFG_MODULE_NS(io)::EndOfLineType eolType() const { return m_eolType; }
			void eolType(DFG_MODULE_NS(io)::EndOfLineType eolType) { m_eolType = eolType; }

			bool saveHeader() const { return m_bSaveHeader; }
			void saveHeader(bool bSaveHeader) { m_bSaveHeader = bSaveHeader; }

			char m_cSep;
			char m_cEnc;
			DFG_MODULE_NS(io)::EndOfLineType m_eolType;
			bool m_bSaveHeader;
		};

		typedef SaveOptions LoadOptions;

		// Maps valid internal row index [0, rowCount[ to user seen indexing, usually 1-based indexing.
		static int internalRowIndexToVisible(const int nRow) { return nRow + 1; }

		DFG_CLASS_NAME(CsvItemModel)();

		// Calls saveToFile(m_sFilePath) and if saving is succesfull,
		// sets m_sFilePath and resets modified flag.
		// [return] : If true iff. save is succesful.
		bool saveToFile();

		// Tries to save csv-data to given path. If given path is empty,
		// the path will be asked from the user.
		// [return] : Returns true iff. save is successful.
		bool saveToFile(QString sPath, const SaveOptions& options = SaveOptions());

		// Tries to write csv-data to given file.
		// [return] : true if writing was successful, false otherwise.
		bool saveToFile(QFile& file, const SaveOptions& options = SaveOptions());

		bool save(QTextStream& strm, const SaveOptions& options = SaveOptions());

		bool openFile(QString sDbFilePath, const LoadOptions& loadOptions = LoadOptions());
		bool openStream(QTextStream& strm, const LoadOptions& loadOptions = LoadOptions());
		bool openString(QString str, const LoadOptions& loadOptions = LoadOptions());

		bool isModified() const { return m_bModified; }
		void setModifiedStatus(const bool bMod = true);

		int getColumnCount() const { return int(m_vecColInfo.size()); }
		int getRowCount() const { return int(m_vecData.size()); }
		bool isValidRow(int r) const { return r >= 0 && r < getRowCount(); }
		bool isValidColumn(int c) const { return c >= 0 && c < getColumnCount(); }

		const QString& getHeaderName(const int nCol) const { return (isValidColumn(nCol)) ? m_vecColInfo[nCol].m_name : s_sEmpty; }

		ColType getColType(const int nCol) const { return (isValidColumn(nCol) ? m_vecColInfo[nCol].m_type : ColTypeText); }
		CompleterType getColCompleterType(const int nCol) const { return (isValidColumn(nCol) ? m_vecColInfo[nCol].m_completerType : CompleterTypeNone); }

		ColInfo* getColInfo(const int nCol) { return isValidColumn(nCol) ? &m_vecColInfo[nCol] : nullptr; }

		// Appends data model row to string.
		template <class String>
		void rowToString(const int nRow, String& str, const char cDelim, std::unordered_set<int>* pSetIgnoreColumns = nullptr) const;

		QString rowToString(const int nRow, const char cDelim = '\t') const
		{
			QString str;
			rowToString(nRow, str, cDelim);
			return str;
		}

		// Populates @p vecStrings with strings from given column. If nCol is not valid,
		// @p vecStrings will be empty.
		void columnToStrings(const int nCol, std::vector<QString>& vecStrings);

		// Sets cell strings in column @p nCol to those given in @p vecStrings.
		void setColumnCells(const int nCol, const std::vector<QString>& vecStrings);

		void setColumnType(const int nCol, const ColType colType);

		// Tokenizes properly formatted text line and sets the data
		// as cells of given row @p nRow.
		void setRow(const int nRow, QString sLine);

		void setItem(const int nRow, const int nCol, const QString str);

		// Sets data of given index without triggering undo.
		// Note: Given model index must be valid; this function does not check it.
		void setDataNoUndo(const QModelIndex& index, const QString& str);

		void setColumnName(const int nCol, const QString& sName);

		void removeRows(const std::vector<int>& vecIndexesAscSorted);

		template <class String>
		String& dataCellToString(const QString& sSrc, String& sDst, const char cDelim) const;

		const QString& getFilePath() const { return m_sFilePath; }

		// Model Overloads
		int rowCount(const QModelIndex & parent = QModelIndex()) const;
		int columnCount(const QModelIndex& parent = QModelIndex()) const;
		QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
		QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
		bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
		Qt::ItemFlags flags(const QModelIndex& index) const;
		bool insertRows(int position, int rows, const QModelIndex& parent = QModelIndex());
		bool removeRows(int position, int rows, const QModelIndex& parent = QModelIndex());
		bool insertColumns(int position, int columns, const QModelIndex& parent = QModelIndex());
		bool removeColumns(int position, int columns, const QModelIndex& parent = QModelIndex());
#if DFG_CSV_ITEM_MODEL_ENABLE_DRAG_AND_DROP_TESTS
		QStringList mimeTypes() const;
		QMimeData* mimeData(const QModelIndexList& indexes) const;
		bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent);
#endif

	signals:
		void sigModifiedStatusChanged(bool bNewStatus);

	private:
		// Clears internal data. Caller should make sure this call
		// is done within appropriate resetmodel-calls.
		void clear();

	public:
		//QUndoStack* m_pUndoStack;
		std::vector<std::vector<QString>> m_vecData; // TODO: revise the data structure. Use Table?

	private:

		std::vector<ColInfo> m_vecColInfo;
		QString m_sFilePath;
		bool m_bModified;
		bool m_bResetting;
		bool m_bEnableCompleter;
	};

	template <class String>
	void DFG_CLASS_NAME(CsvItemModel)::rowToString(const int nRow, String& str, const char cDelim, std::unordered_set<int>* pSetIgnoreColumns /*= nullptr*/) const
	{
		if (!isValidRow(nRow))
			return;
		const std::vector<QString>& vRow = m_vecData[nRow];
		for (size_t i = 0; i < vRow.size(); ++i)
		{
			if (pSetIgnoreColumns != nullptr && pSetIgnoreColumns->find(i) != pSetIgnoreColumns->end())
				continue;
			if (!str.isEmpty())
				str.push_back(cDelim);
			dataCellToString(vRow[i], str, cDelim);
		}
	}

	template <class String>
	String& DFG_CLASS_NAME(CsvItemModel)::dataCellToString(const QString& sSrc, String& sDst, const char cDelim) const
	{
		QTextStream strm(&sDst);
		DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextCellWriter)::writeCellFromStrStrm(strm,
			sSrc,
			cDelim,
			'"',
			'\n',
			DFG_MODULE_NS(io)::EbEncloseIfNeeded);
		return sDst;
	}

} } // module namespace
