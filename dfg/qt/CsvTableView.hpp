#pragma once

#include "../dfgDefs.hpp"
#include "TableView.hpp"
#include "../cont/TorRef.hpp"
#include "StringMatchDefinition.hpp"
#include <memory>

#include "qtIncludeHelpers.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QPointer>
DFG_END_INCLUDE_QT_HEADERS

class QUndoStack;
class QAbstractProxyModel;
class QItemSelection;
class QItemSelectionRange;
class QMenu;
class QProgressBar;
class QPushButton;


namespace DFG_ROOT_NS
{
    class DFG_CLASS_NAME(CsvFormatDefinition);
}

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{
    class DFG_CLASS_NAME(CsvItemModel);

    class DFG_CLASS_NAME(CsvTableViewBasicSelectionAnalyzerPanel);

    // Analyzes item selection
    class DFG_CLASS_NAME(CsvTableViewSelectionAnalyzer)
    {
    public:
        enum CompletionStatus
        {
            CompletionStatus_started,
            CompletionStatus_completed,
            CompletionStatus_terminatedByTimeLimit,
            CompletionStatus_terminatedByUserRequest
        };

        virtual ~DFG_CLASS_NAME(CsvTableViewSelectionAnalyzer)() {}
        void analyze(QAbstractItemView* pView, const QItemSelection& selection) { analyzeImpl(pView, selection); }
    private:
        virtual void analyzeImpl(QAbstractItemView* pView, const QItemSelection& selection) = 0;
    };

    class DFG_CLASS_NAME(CsvTableViewBasicSelectionAnalyzer) : public DFG_CLASS_NAME(CsvTableViewSelectionAnalyzer)
    {
    public:
        typedef DFG_CLASS_NAME(CsvTableViewBasicSelectionAnalyzerPanel) PanelT;

        DFG_CLASS_NAME(CsvTableViewBasicSelectionAnalyzer)(PanelT* uiPanel);
        ~DFG_CLASS_NAME(CsvTableViewBasicSelectionAnalyzer)() DFG_OVERRIDE_DESTRUCTOR;

        QPointer<DFG_CLASS_NAME(CsvTableViewBasicSelectionAnalyzerPanel)> m_spUiPanel;
    private:
        void analyzeImpl(QAbstractItemView* pView, const QItemSelection& selection) override;

    }; // Class CsvTableViewBasicSelectionAnalyzer

    class DFG_CLASS_NAME(CsvTableViewBasicSelectionAnalyzerPanel) : public QWidget
    {
        Q_OBJECT
    public:
        typedef DFG_CLASS_NAME(CsvTableViewBasicSelectionAnalyzerPanel) ThisClass;
        typedef QWidget BaseClass;
        CsvTableViewBasicSelectionAnalyzerPanel(QWidget *pParent = nullptr);
        virtual ~CsvTableViewBasicSelectionAnalyzerPanel();

        void setValueDisplayString(const QString& s);

        void onEvaluationStarting(bool bEnabled);
        void onEvaluationEnded(const double timeInSeconds, DFG_CLASS_NAME(CsvTableViewSelectionAnalyzer)::CompletionStatus completionStatus);

        double getMaxTimeInSeconds() const;
        bool isStopRequested() const;

    signals:
        void sigEvaluationStartingHandleRequest(bool bEnabled);
        void sigEvaluationEndedHandleRequest(const double timeInSeconds, int completionStatus);
        void sigSetValueDisplayString(const QString& s);

    private slots:
        void onEvaluationStarting_myThread(bool bEnabled);
        void onEvaluationEnded_myThread(const double timeInSeconds, int completionStatus);
        void setValueDisplayString_myThread(const QString& s);

    private:
        std::unique_ptr<QLineEdit>      m_spValueDisplay;
        QPointer<QLineEdit>             m_spTimeLimitDisplay;
        std::unique_ptr<QProgressBar>   m_spProgressBar;
        std::unique_ptr<QPushButton>    m_spStopButton;
    }; // class CsvTableViewBasicSelectionAnalyzerPanel

    // View for showing CsvItemModel.
    class DFG_CLASS_NAME(CsvTableView) : public DFG_CLASS_NAME(TableView)
    {
        Q_OBJECT

    public:
        typedef DFG_CLASS_NAME(TableView) BaseClass;
        typedef DFG_CLASS_NAME(CsvTableView) ThisClass;
        typedef DFG_CLASS_NAME(CsvItemModel) CsvModel;
        typedef DFG_CLASS_NAME(StringMatchDefinition) StringMatchDef;

        enum ModelIndexType
        {
            ModelIndexTypeSource, // Refers to indexes in underlying data model.
            ModelIndexTypeView    // Refers to indexes in view model (which can be the same as source-model in case there is no proxy).
        };

        DFG_CLASS_NAME(CsvTableView)(QWidget* pParent);
        ~DFG_CLASS_NAME(CsvTableView)() DFG_OVERRIDE_DESTRUCTOR;

        // If already present, old undo stack will be destroyed.
        void createUndoStack();
        void clearUndoStack();
        void showUndoWindow();

        void setExternalUndoStack(QUndoStack* pUndoStack);

        void setModel(QAbstractItemModel* pModel) override;
        CsvModel* csvModel();
        const CsvModel* csvModel() const;

        // Returns the smallest row index in the current view selection,
        // row count if no selection exists.
        int getFirstSelectedViewRow() const;

        QModelIndex getFirstSelectedItem(QAbstractProxyModel* pProxy) const;

        // Returns list of row indexes of column @p nCol.
        // If proxy model is given, the returned indexes will correspond
        // to the indexes of the underlying model, otherwise they will be
        // {0, 1,..., rowCount()-1}
        std::vector<int> getRowsOfCol(const int nCol, const QAbstractProxyModel* pProxy) const;

        // Returned list is free of duplicates. If @p pProxy != nullptr,
        // the selected indexes will be mapped by the proxy.
        std::vector<int> getRowsOfSelectedItems(const QAbstractProxyModel* pProxy, const bool bSort = true) const;

        // Returns list of selected indexes as model indexes of underlying data model.
        QModelIndexList getSelectedItemIndexes_dataModel() const;

        // Returns list of selected indexes as model indexes from model(). If there is no proxy model,
        // returns the same as getSelectedItemIndexes_dataModel().
        QModelIndexList getSelectedItemIndexes_viewModel() const;

        std::vector<int> getDataModelRowsOfSelectedItems(const bool bSort = true) const
        {
            return getRowsOfSelectedItems(getProxyModelPtr(), bSort);
        }

        void invertSelection();

        bool isRowMode() const;

        QAbstractProxyModel* getProxyModelPtr();
        const QAbstractProxyModel* getProxyModelPtr() const;

        bool saveToFileImpl(const QString& path, const DFG_CLASS_NAME(CsvFormatDefinition)& formatDef);
        bool saveToFileImpl(const DFG_CLASS_NAME(CsvFormatDefinition)& formatDef);

        void privAddUndoRedoActions(QAction* pAddBefore = nullptr);

        bool generateContentImpl(const CsvModel& csvModel);

        bool getAllowApplicationSettingsUsage() const;
        void setAllowApplicationSettingsUsage(bool b);

        void finishEdits();

        int getFindColumnIndex() const;

        void onFind(const bool forward);

        void addSelectionAnalyzer(std::shared_ptr<DFG_CLASS_NAME(CsvTableViewSelectionAnalyzer)> spAnalyzer);

        // Maps index to view model (i.e. the one returned by model()) assuming that 'index' is either from csvModel() or model().
        QModelIndex mapToViewModel(const QModelIndex& index) const;

        // Maps index to data model (i.e. the one returned by csvModel()) assuming that 'index' is either from csvModel() or model().
        QModelIndex mapToDataModel(const QModelIndex& index) const;

        // Forgets latest find position so that next begins from memoryless situation.
        void forgetLatestFindPosition();

        std::unique_ptr<QMenu> createResizeColumnsMenu();

        template <class Func_T>
        void forEachCsvModelIndexInSelection(Func_T func);

        template <class Func_T>
        void forEachCsvModelIndexInSelection(Func_T func) const;

        template <class Func_T>
        void forEachCsvModelIndexInSelectionRange(const QItemSelectionRange& sr, Func_T func);

        template <class Func_T>
        void forEachCsvModelIndexInSelectionRange(const QItemSelectionRange& sr, Func_T func) const;

        // Returns viewModel->index(r, c) mapped to source model, QModelIndex() if neither pModel or pProxy is available.
        static QModelIndex mapToSource(const QAbstractItemModel* pModel, const QAbstractProxyModel* pProxy, int r, int c);

        bool openFile(const QString& sPath);
        bool openFile(const QString& sPath, const DFG_CLASS_NAME(CsvFormatDefinition)& formatDef);

    private:
        template <class T, class Param0_T>
        bool executeAction(Param0_T&& p0);

        template <class T, class Param0_T, class Param1_T>
        bool executeAction(Param0_T&& p0, Param1_T&& p1);

        template <class T, class Param0_T, class Param1_T, class Param2_T>
        bool executeAction(Param0_T&& p0, Param1_T&& p1, Param2_T&& p2);

        template <class T, class Param0_T>
        void pushToUndoStack(Param0_T&& p0);

        template <class T, class Param0_T, class Param1_T>
        void pushToUndoStack(Param0_T&& p0, Param1_T&& p1);

        template <class T, class Param0_T, class Param1_T, class Param2_T>
        void pushToUndoStack(Param0_T&& p0, Param1_T&& p1, Param2_T&& p2);

        template <class This_T, class Func_T>
        static void forEachViewModelIndexInSelection(This_T& thisItem, Func_T func);

        template <class This_T, class Func_T>
        static void forEachCsvModelIndexInSelection(This_T& thisItem, Func_T func);

        template <class This_T, class Func_T>
        static void forEachIndexInSelection(This_T& thisItem, ModelIndexType indexType, Func_T func);

        template <class This_T, class Func_T>
        static void forEachCsvModelIndexInSelectionRange(This_T& thisItem, const QItemSelectionRange& sr, Func_T func);

        template <class This_T, class Func_T>
        static void forEachIndexInSelectionRange(This_T& thisItem, const QItemSelectionRange& sr, ModelIndexType indexType, Func_T func);

    public slots:
        void createNewTable();
        bool createNewTableFromClipboard();
        bool openFromFile();
        bool openFromFileWithOptions();
        bool mergeFilesToCurrent();
        bool save();
        bool saveToFile();
        bool saveToFileWithOptions();
        bool openAppConfigFile();
        bool openConfigFile();
        bool saveConfigFile();
        bool clearSelected();
        bool insertRowHere();
        bool insertRowAfterCurrent();
        bool insertColumn();
        bool insertColumnAfterCurrent();
        bool paste();
        bool copy();
        bool cut();

        bool deleteCurrentColumn();
        bool deleteCurrentColumn(const int nCol);
        bool deleteSelectedRow();

        bool moveFirstRowToHeader();
        bool moveHeaderToFirstRow();

        bool resizeTable();

        bool generateContent();

        bool diffWithUnmodified();

        void onFindRequested();
        void onFindNext();
        void onFindPrevious();

        void onFilterRequested();

        void setFindText(const StringMatchDef matchDef, const int nCol);

        void onNewSourceOpened();

        void onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
        void onSelectionContentChanged();

        void onColumnResizeAction_toViewEvenly();
        void onColumnResizeAction_toViewContentAware();
        void onColumnResizeAction_content();
        void onColumnResizeAction_fixedSize();

        void onRowResizeAction_content();
        void onRowResizeAction_fixedSize();

        void setRowMode(bool);
        void setUndoEnabled(bool);

        void insertGeneric(const QString& s);
        void insertDate();
        void insertTime();
        void insertDateTime();

        /*
        void pasteColumn();
        void pasteColumn(const int nCol);
        void copyColumn();
        void copyColumn(const int nCol);
        void moveColumnRight();
        void moveColumnLeft();
        void moveRowDown();
        void moveRowUp();
        void renameColumn();
        void renameColumn(const int nCol);
        
        void insertColumnAfterCurrent();
        void insertColumnAfterCurrent(const int nCol);
        void insertColumn(const int nCol);
        void insertAfterCurrent();
        void insert();
        void deleteSelected();
        */

    signals:
        void sigFindActivated();
        void sigFilterActivated();
        void sigSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
        void sigOnAllowApplicationSettingsUsageChanged(bool);

    protected:
        void contextMenuEvent(QContextMenuEvent* pEvent) override;
        QModelIndexList selectedIndexes() const override;

    private:
        template <class Func_T>
        void forEachCompleterEnabledColumnIndex(Func_T func);

        bool getProceedConfirmationFromUserIfInModifiedState(const QString& sTranslatedActionDescription);

    public:
        std::unique_ptr<DFG_MODULE_NS(cont)::DFG_CLASS_NAME(TorRef)<QUndoStack>> m_spUndoStack;
        QStringList m_tempFilePathsToRemoveOnExit;
        QModelIndex m_latestFoundIndex; // Index from underlying model. Invalid if doing first find.
        StringMatchDef m_matchDef;
        int m_nFindColumnIndex;
        std::vector<std::shared_ptr<DFG_CLASS_NAME(CsvTableViewSelectionAnalyzer)>> m_selectionAnalyzers;
        std::unique_ptr<QMenu> m_spResizeColumnsMenu;
        bool m_bUndoEnabled;
    };

    template <class Func_T>
    void DFG_CLASS_NAME(CsvTableView)::forEachCsvModelIndexInSelection(Func_T func)
    {
        forEachCsvModelIndexInSelection(*this, std::forward<Func_T>(func));
    }

    template <class Func_T>
    void DFG_CLASS_NAME(CsvTableView)::forEachCsvModelIndexInSelection(Func_T func) const
    {
       forEachCsvModelIndexInSelection(*this, std::forward<Func_T>(func));
    }

    template <class Func_T>
    void DFG_CLASS_NAME(CsvTableView)::forEachCsvModelIndexInSelectionRange(const QItemSelectionRange& sr, Func_T func)
    {
        forEachCsvModelIndexInSelectionRange(*this, sr, std::forward<Func_T>(func));
    }

    template <class Func_T>
    void DFG_CLASS_NAME(CsvTableView)::forEachCsvModelIndexInSelectionRange(const QItemSelectionRange& sr, Func_T func) const
    {
        forEachCsvModelIndexInSelectionRange(*this, sr, std::forward<Func_T>(func));
    }

    template <class This_T, class Func_T>
    void DFG_CLASS_NAME(CsvTableView)::forEachCsvModelIndexInSelection(This_T& thisItem, Func_T func)
    {
        forEachIndexInSelection(thisItem, ModelIndexTypeSource, std::forward<Func_T>(func));
    }

    template <class This_T, class Func_T>
    void DFG_CLASS_NAME(CsvTableView)::forEachIndexInSelection(This_T& thisItem, const ModelIndexType indexType, Func_T func)
    {
        const auto sm = thisItem.selectionModel();
        const auto selection = (sm) ? sm->selection() : QItemSelection();
        for(auto iter = selection.cbegin(); iter != selection.cend(); ++iter)
        {
            forEachIndexInSelectionRange(thisItem, *iter, indexType, std::forward<Func_T>(func));
        }
    }

    template <class This_T, class Func_T>
    void DFG_CLASS_NAME(CsvTableView)::forEachCsvModelIndexInSelectionRange(This_T& thisItem, const QItemSelectionRange& sr, Func_T func)
    {
        forEachIndexInSelectionRange(thisItem, sr, ModelIndexTypeSource, std::forward<Func_T>(func));
    }

    template <class This_T, class Func_T>
    void DFG_CLASS_NAME(CsvTableView)::forEachIndexInSelectionRange(This_T& thisItem, const QItemSelectionRange& sr, const ModelIndexType indexType, Func_T func)
    {
        auto pProxy = (indexType == ModelIndexTypeSource) ? thisItem.getProxyModelPtr() : nullptr;
        auto pModel = thisItem.model();
        if (!pModel)
            return;
        // TODO: if not having proxy, iterate in the way that is optimal to underlying data structure
        const auto right = sr.right();
        const auto bottom = sr.bottom();
        bool bContinue = true;
        for (int c = sr.left(); c<=right && bContinue; ++c)
        {
            for (int r = sr.top(); r<=bottom && bContinue; ++r)
            {
                func(mapToSource(thisItem.model(), pProxy, r, c), bContinue);
            }
        }
    }

} } // module namespace
