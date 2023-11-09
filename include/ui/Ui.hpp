#ifndef MED_UI_HPP
#define MED_UI_HPP

#define SCAN_COL_ADDRESS 0
#define SCAN_COL_TYPE 1
#define SCAN_COL_VALUE 2

#define STORE_COL_DESCRIPTION 0
#define STORE_COL_ADDRESS 1
#define STORE_COL_TYPE 2
#define STORE_COL_VALUE 3
#define STORE_COL_LOCK 4

#define SCAN_ADDRESS_VISIBLE_SIZE 800

#include <QTreeWidgetItem>
#include <QStatusBar>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QProgressBar>

#include "ui/TreeModel.hpp"
#include "ui/StoreTreeModel.hpp"
#include "ui/NamedScansController.hpp"
#include "mem/MemEd.hpp"

const int REFRESH_RATE = 800;

const QString MAIN_TITLE = "Med UI";

enum UiState { Idle, Editing };

class EncodingManager; // Forward declaration, because of recursive include

class MedUi : public QObject {
  Q_OBJECT

public:
  explicit MedUi(QApplication* app);
  ~MedUi();

  QWidget* mainWindow;
  QWidget* processSelector;
  QDialog* processDialog; // This is the processSelector container
  QTreeWidget* processTreeWidget;
  QLineEdit* selectedProcessLine;
  QStatusBar* statusBar;
  QComboBox* scanTypeCombo;
  QTreeView* scanTreeView;
  QTreeView* storeTreeView;
  QPlainTextEdit* notesArea;
  StoreTreeModel* storeModel;
  TreeModel* scanModel;

  static void refresh(MedUi* mainUi);
  void refreshScanTreeView();
  void refreshStoreTreeView();

  EncodingManager* encodingManager;
  MemEd* med;
  bool autoRefresh;
  bool fastScan;
  bool forceResume;
  std::thread* refreshThread;

  UiState getScanState();
  void setScanState(UiState);
  UiState getStoreState();
  void setStoreState(UiState);

  std::mutex* scanUpdateMutex;
  std::mutex storeUpdateMutex;

  void setWindowTitle();
  void openFile(QString filename);
  void updateNumberOfAddresses();

public slots:
  void onProcessItemDblClicked(QTreeWidgetItem* item, int column);

private slots:
  void onProcessClicked();
  void onScanClicked();
  void onFilterClicked();
  void onPauseCheckboxClicked(bool checked);

  void onScanTreeViewClicked(const QModelIndex &index);
  void onScanTreeViewDoubleClicked(const QModelIndex &index);

  void onStoreTreeViewDoubleClicked(const QModelIndex &index);
  void onStoreTreeViewClicked(const QModelIndex &index);

  void onScanTreeViewDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles = QVector<int>());
  void onStoreTreeViewDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles = QVector<int>());

  void onScanAddClicked();
  void onScanAddAllClicked();
  void onScanClearClicked();

  void onStoreHeaderClicked(int logicalIndex);

  void onStoreNextClicked();
  void onStorePrevClicked();
  void onStoreShiftClicked();
  void onStoreUnshiftClicked();
  void onStoreMoveClicked();

  void onSaveAsTriggered();
  void onSaveTriggered();
  void onOpenTriggered();
  void onReloadTriggered();
  void onQuitTriggered();
  void onShowNotesTriggered(bool checked);
  void onNotesAreaChanged();
  void onAutoRefreshTriggered(bool checked);
  void onRefreshTriggered();
  void onResumeProcessTriggered(bool checked);
  void onFastScanTriggered(bool checked);
  void onForceResumeTriggered(bool checked);
  void onUnlockAllTriggered();

  void onNewAddressTriggered();
  void onDeleteAddressTriggered();
  void onStoreClearTriggered();

  void onMemEditorTriggered();

  void onScopeStartEdited();
  void onScopeEndEdited();

private:
  void loadUiFiles();
  void loadProcessUi();
  void loadMemEditor();
  void setupStatusBar();
  void setupScanTreeView();
  void setupStoreTreeView();
  void setupSignals();
  void setupUi();

  string getLastDigit();

  ProgressKeeper makeProgressKeeper(QProgressBar* progressBar);

  QApplication* app;
  UiState scanState;
  UiState storeState;
  QWidget* memEditor;

  QString filename;

  NamedScansController *namedScansController;
};


#endif
