/**
 * 2016-09-19
 * Try to use QThread, but it involves signal and slot. So, I choose C++11 thread instead, for simpler implementation
 * TODO: Need to redesign the "read" and "write" when item change. Not every time it is "write".
 * Possible solution, create property to the object.
 */
#include <iostream>
#include <cstdio>
#include <chrono>
#include <thread>
#include <mutex>
#include <algorithm>
#include <QApplication>
#include <QtUiTools>
#include <QtDebug>

#include "med-qt.hpp"
#include "TreeItem.hpp"
#include "TreeModel.hpp"
#include "StoreTreeModel.hpp"
#include "ComboBoxDelegate.hpp"
#include "CheckBoxDelegate.hpp"
#include "med.hpp"

using namespace std;

/**
 * This will just perform the unlock by force
 */
void tryUnlock(std::mutex &mutex) {
  mutex.try_lock();
  mutex.unlock();
}

class MainUi : public QObject {
  Q_OBJECT

public:
  MainUi() {
    loadUiFiles();
    loadProcessUi();
    setupStatusBar();
    setupScanTreeView();
    setupStoreTreeView();
    setupSignals();
    setupUi();
  }
  Med med;
  std::thread* refreshThread;
  std::mutex scanUpdateMutex;
  std::mutex addressUpdateMutex; //TODO: Rename to storeUpdateMutex

  static void refresh(MainUi* mainUi) {
    while(1) {
      mainUi->refreshScanTreeView();
      mainUi->refreshStoreTreeView();
      std::this_thread::sleep_for(chrono::milliseconds(800));
    }
  }

  void refreshScanTreeView() {
    scanUpdateMutex.lock();
    scanModel->refreshValues();
    scanUpdateMutex.unlock();
  }

  void refreshStoreTreeView() {
    addressUpdateMutex.lock();
    storeModel->refreshValues();
    addressUpdateMutex.unlock();
  }

private slots:
  void onProcessClicked() {
    med.listProcesses();

    processDialog->show();

    //Get the tree widget
    QTreeWidget* procTreeWidget = chooseProc->findChild<QTreeWidget*>("procTreeWidget");

    procTreeWidget->clear(); //Remove all items

    //Add all the process into the tree widget
    for(int i=med.processes.size()-1;i>=0;i--) {
      QTreeWidgetItem* item = new QTreeWidgetItem(procTreeWidget);
      item->setText(0, med.processes[i].pid.c_str());
      item->setText(1, med.processes[i].cmdline.c_str());
    }
  }

  void onProcItemDblClicked(QTreeWidgetItem* item, int column) {
    int index = item->treeWidget()->indexOfTopLevelItem(item); //Get the current row index
    med.selectedProcess = med.processes[med.processes.size() -1 - index];

    //Make changes to the selectedProc and hide the window
    QLineEdit* line = this->mainWindow->findChild<QLineEdit*>("selectedProc");
    line->setText(QString::fromLatin1((med.selectedProcess.pid + " " + med.selectedProcess.cmdline).c_str())); //Do not use fromStdString(), it will append with some unknown characters

    processDialog->hide();
  }

  void onScanClicked() {
    scanUpdateMutex.lock();

    //Get scanned type
    string scanType = mainWindow->findChild<QComboBox*>("scanType")->currentText().toStdString();

    string scanValue = mainWindow->findChild<QLineEdit*>("scanEntry")->text().toStdString();

    if(med.selectedProcess.pid == "") {
      cerr << "No process seelcted " <<endl;
      return;
    }
    try {
      med.scan(scanValue, scanType);
    } catch(MedException &ex) {
      cerr << "scan: "<< ex.what() <<endl;
    }

    scanModel->clearAll();
    if(med.scanAddresses.size() <= 800) {
      scanModel->addScan(scanType);
    }

    updateNumberOfAddresses(mainWindow);
    scanUpdateMutex.unlock();
  }

  void onFilterClicked() {
    scanUpdateMutex.lock();

    //Get scanned type
    string scanType = mainWindow->findChild<QComboBox*>("scanType")->currentText().toStdString();

    string scanValue = mainWindow->findChild<QLineEdit*>("scanEntry")->text().toStdString();

    med.filter(scanValue, scanType);

    if(med.scanAddresses.size() <= 800) {
      scanModel->addScan(scanType);
    }

    updateNumberOfAddresses(mainWindow);
    scanUpdateMutex.unlock();
  }

  void onScanClearClicked() {
    scanUpdateMutex.lock();
    scanModel->empty();
    mainWindow->findChild<QStatusBar*>("statusbar")->showMessage("Scan cleared");
    scanUpdateMutex.unlock();
  }
  void onAddressClearClicked() {
    addressUpdateMutex.lock();
    storeModel->empty();
    addressUpdateMutex.unlock();
  }

  void onScanAddClicked() {
    auto indexes = mainWindow->findChild<QTreeView*>("scanTreeView")
      ->selectionModel()
      ->selectedRows(SCAN_COL_ADDRESS);
    
    scanUpdateMutex.lock();
    for (int i=0;i<indexes.size();i++) {
      med.addToStoreByIndex(indexes[i].row());
    }

    storeModel->refresh();
    scanUpdateMutex.unlock();
  }

  void onScanAddAllClicked() {
    scanUpdateMutex.lock();
    for(auto i=0;i<med.scanAddresses.size();i++)
      med.addToStoreByIndex(i);
    storeModel->refresh();
    scanUpdateMutex.unlock();
  }

  void onAddressNewClicked() {
    addressUpdateMutex.lock();
    med.addNewAddress();
    storeModel->addRow();
    addressUpdateMutex.unlock();
  }

  void onAddressDeleteClicked() {
    auto indexes = mainWindow->findChild<QTreeView*>("storeTreeView")
      ->selectionModel()
      ->selectedRows(ADDRESS_COL_ADDRESS);

    //Sort and reverse
    sort(indexes.begin(), indexes.end(), [](QModelIndex a, QModelIndex b) {
        return a.row() > b.row();
      });

    addressUpdateMutex.lock();
    for (auto i=0;i<indexes.size();i++) {
      med.deleteAddressByIndex(indexes[i].row());
    }
    storeModel->refresh();
    addressUpdateMutex.unlock();
  }

  void onAddressShiftAllClicked() {
    //Get the from and to
    long shiftFrom, shiftTo, difference;
    try {
      shiftFrom = hexToInt(mainWindow->findChild<QLineEdit*>("shiftFrom")->text().toStdString());
      shiftTo = hexToInt(mainWindow->findChild<QLineEdit*>("shiftTo")->text().toStdString());
      difference = shiftTo - shiftFrom;
    } catch(MedException &e) {
      cout << e.what() << endl;
    }

    //Get PID
    if(med.selectedProcess.pid == "") {
      cerr<< "No PID" <<endl;
      return;
    }
    addressUpdateMutex.lock();
    med.shiftStoreAddresses(difference);
    storeModel->refresh();
    addressUpdateMutex.unlock();
  }

  void onAddressShiftClicked() {
    //Get the from and to
    long shiftFrom, shiftTo, difference;
    try {
      shiftFrom = hexToInt(mainWindow->findChild<QLineEdit*>("shiftFrom")->text().toStdString());
      shiftTo = hexToInt(mainWindow->findChild<QLineEdit*>("shiftTo")->text().toStdString());
      difference = shiftTo - shiftFrom;
    } catch(MedException &e) {
      cout << e.what() << endl;
    }
    
    //Get PID
    if(med.selectedProcess.pid == "") {
      cerr<< "No PID" <<endl;
      return;
    }

    auto indexes = mainWindow->findChild<QTreeView*>("storeTreeView")
      ->selectionModel()
      ->selectedRows(ADDRESS_COL_ADDRESS);

    addressUpdateMutex.lock();
    for (auto i=0;i<indexes.size();i++) {
      med.shiftStoreAddressByIndex(indexes[i].row(), difference);
    }
    storeModel->refresh();
    addressUpdateMutex.unlock();
  }

  void onSaveAsTriggered() {
    if(med.selectedProcess.pid == "") {
      cerr<< "No PID" <<endl;
      return;
    }
    QString filename = QFileDialog::getSaveFileName(mainWindow,
                                                    QString("Save JSON"),
                                                    "./",
                                                    QString("Save JSON (*.json)"));

    med.saveFile(filename.toStdString().c_str());
  }

  void onOpenTriggered() {
    if(med.selectedProcess.pid == "") {
      cerr<< "No PID" <<endl;
      return;
    }
    QString filename = QFileDialog::getOpenFileName(mainWindow,
                                                    QString("Open JSON"),
                                                    "./",
                                                    QString("Open JSON (*.json)"));
    med.openFile(filename.toStdString().c_str());

    addressUpdateMutex.lock();
    storeModel->clearAll();
    storeModel->refresh();
    addressUpdateMutex.unlock();
  }

  void onScanTreeViewClicked(const QModelIndex &index) {
    if(index.column() == SCAN_COL_TYPE) {
      mainWindow->findChild<QTreeView*>("scanTreeView")->edit(index); //Trigger edit by 1 click
    }
  }

  void onScanTreeViewDoubleClicked(const QModelIndex &index) {
    if (index.column() == SCAN_COL_VALUE) {
      scanUpdateMutex.lock();
    }
  }

  void onStoreTreeViewDoubleClicked(const QModelIndex &index) {
    if (index.column() == ADDRESS_COL_VALUE) {
      addressUpdateMutex.lock();
    }
  }

  void onStoreTreeViewClicked(const QModelIndex &index) {
    if (index.column() == ADDRESS_COL_TYPE) {
      mainWindow->findChild<QTreeView*>("storeTreeView")->edit(index);
    }
    else if (index.column() == ADDRESS_COL_LOCK) {
      mainWindow->findChild<QTreeView*>("storeTreeView")->edit(index);
    }
  }

  void onScanTreeViewDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles = QVector<int>()) {
    // qDebug() << topLeft << bottomRight << roles;
    if (topLeft.column() == SCAN_COL_VALUE) {
      tryUnlock(scanUpdateMutex);
    }
  }

  void onStoreTreeViewDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles = QVector<int>()) {
    // qDebug() << topLeft << bottomRight << roles;
    if (topLeft.column() == ADDRESS_COL_VALUE) {
      tryUnlock(addressUpdateMutex);
    }
  }

  void onStoreHeaderClicked(int logicalIndex) {
    if (logicalIndex == ADDRESS_COL_DESCRIPTION) {
      storeModel->sortByDescription();
    }
    else if (logicalIndex == ADDRESS_COL_ADDRESS) {
      storeModel->sortByAddress();
    }
  }

private:
  QWidget* mainWindow;
  QWidget* chooseProc;
  QDialog* processDialog;

  TreeModel* scanModel;
  StoreTreeModel * storeModel; //Previously is the address model, but named as "store" is better

  void loadUiFiles() {
    QUiLoader loader;
    QFile file("./main-qt.ui");
    file.open(QFile::ReadOnly);
    mainWindow = loader.load(&file);
    file.close();
  }

  void loadProcessUi() {
    QUiLoader loader;
    //Cannot put the followings to another method
    processDialog = new QDialog(mainWindow); //If put this to another method, then I cannot set the mainWindow as the parent
    QFile processFile("./process.ui");
    processFile.open(QFile::ReadOnly);

    chooseProc = loader.load(&processFile, processDialog);
    processFile.close();

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(chooseProc);
    processDialog->setLayout(layout);
    processDialog->setModal(true);
    processDialog->resize(400, 400);

    //Add signal
    QTreeWidget* procTreeWidget = chooseProc->findChild<QTreeWidget*>("procTreeWidget");
    QObject::connect(procTreeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(onProcItemDblClicked(QTreeWidgetItem*, int)));

    procTreeWidget->installEventFilter(this);
  }

  void setupStatusBar() {
    //Statusbar message
    QStatusBar* statusBar = mainWindow->findChild<QStatusBar*>("statusbar");
    statusBar->showMessage("Tips: Left panel is scanned address. Right panel is stored address.");
  }

  void setupScanTreeView() {
    //Tree model
    scanModel = new TreeModel(&med, mainWindow);
    mainWindow->findChild<QTreeView*>("scanTreeView")->setModel(scanModel);
    ComboBoxDelegate* delegate = new ComboBoxDelegate();
    mainWindow->findChild<QTreeView*>("scanTreeView")->setItemDelegateForColumn(SCAN_COL_TYPE, delegate);
    QObject::connect(mainWindow->findChild<QTreeView*>("scanTreeView"),
                     SIGNAL(clicked(QModelIndex)),
                     this,
                     SLOT(onScanTreeViewClicked(QModelIndex)));
    QObject::connect(mainWindow->findChild<QTreeView*>("scanTreeView"),
                     SIGNAL(doubleClicked(QModelIndex)),
                     this,
                     SLOT(onScanTreeViewDoubleClicked(QModelIndex)));

    QObject::connect(scanModel,
                     SIGNAL(dataChanged(QModelIndex, QModelIndex, QVector<int>)),
                     this,
                     SLOT(onScanTreeViewDataChanged(QModelIndex, QModelIndex, QVector<int>)));
    mainWindow->findChild<QTreeView*>("scanTreeView")->setSelectionMode(QAbstractItemView::ExtendedSelection);
  }

  void setupStoreTreeView() {
    storeModel = new StoreTreeModel(&med, mainWindow);
    mainWindow->findChild<QTreeView*>("storeTreeView")->setModel(storeModel);
    ComboBoxDelegate* storeDelegate = new ComboBoxDelegate();
    mainWindow->findChild<QTreeView*>("storeTreeView")->setItemDelegateForColumn(ADDRESS_COL_TYPE, storeDelegate);
    CheckBoxDelegate* storeLockDelegate = new CheckBoxDelegate();
    mainWindow->findChild<QTreeView*>("storeTreeView")->setItemDelegateForColumn(ADDRESS_COL_LOCK, storeLockDelegate);
    QObject::connect(mainWindow->findChild<QTreeView*>("storeTreeView"),
                     SIGNAL(clicked(QModelIndex)),
                     this,
                     SLOT(onStoreTreeViewClicked(QModelIndex)));
    QObject::connect(mainWindow->findChild<QTreeView*>("storeTreeView"),
                     SIGNAL(doubleClicked(QModelIndex)),
                     this,
                     SLOT(onStoreTreeViewDoubleClicked(QModelIndex)));

    QObject::connect(storeModel,
                     SIGNAL(dataChanged(QModelIndex, QModelIndex, QVector<int>)),
                     this,
                     SLOT(onStoreTreeViewDataChanged(QModelIndex, QModelIndex, QVector<int>)));

    auto* header = mainWindow->findChild<QTreeView*>("storeTreeView")->header();
    header->setSectionsClickable(true);
    QObject::connect(header,
                     SIGNAL(sectionClicked(int)),
                     this,
                     SLOT(onStoreHeaderClicked(int)));

    mainWindow->findChild<QTreeView*>("storeTreeView")->setSelectionMode(QAbstractItemView::ExtendedSelection);
  }

  void setupSignals() {
    //Add signal to the process
    QObject::connect(mainWindow->findChild<QWidget*>("process"),
                     SIGNAL(clicked()),
                     this,
                     SLOT(onProcessClicked()));

    //Add signal to scan
    QObject::connect(mainWindow->findChild<QWidget*>("scanButton"),
                     SIGNAL(clicked()),
                     this,
                     SLOT(onScanClicked()));

    QObject::connect(mainWindow->findChild<QWidget*>("filterButton"),
                     SIGNAL(clicked()),
                     this,
                     SLOT(onFilterClicked()));

    QObject::connect(mainWindow->findChild<QWidget*>("scanClear"),
                     SIGNAL(clicked()),
                     this,
                     SLOT(onScanClearClicked())
                     );

    QObject::connect(mainWindow->findChild<QPushButton*>("scanAddAll"),
                     SIGNAL(clicked()),
                     this,
                     SLOT(onScanAddAllClicked())
                     );

    QObject::connect(mainWindow->findChild<QPushButton*>("scanAdd"),
                      SIGNAL(clicked()),
                      this,
                      SLOT(onScanAddClicked())
                      );

    QObject::connect(mainWindow->findChild<QPushButton*>("addressNew"),
                     SIGNAL(clicked()),
                     this,
                     SLOT(onAddressNewClicked()));
    QObject::connect(mainWindow->findChild<QPushButton*>("addressDelete"),
                     SIGNAL(clicked()),
                     this,
                     SLOT(onAddressDeleteClicked()));
    QObject::connect(mainWindow->findChild<QPushButton*>("addressClear"),
                     SIGNAL(clicked()),
                     this,
                     SLOT(onAddressClearClicked()));

    QObject::connect(mainWindow->findChild<QPushButton*>("addressShiftAll"),
                     SIGNAL(clicked()),
                     this,
                     SLOT(onAddressShiftAllClicked()));
    
    QObject::connect(mainWindow->findChild<QPushButton*>("addressShift"),
                     SIGNAL(clicked()),
                     this,
                     SLOT(onAddressShiftClicked()));

    
    QObject::connect(mainWindow->findChild<QAction*>("actionSaveAs"),
                     SIGNAL(triggered()),
                     this,
                     SLOT(onSaveAsTriggered()));
    QObject::connect(mainWindow->findChild<QAction*>("actionOpen"),
                     SIGNAL(triggered()),
                     this,
                     SLOT(onOpenTriggered()));

  }

  void setupUi() {

    //Set default scan type
    mainWindow->findChild<QComboBox*>("scanType")->setCurrentIndex(1);

    //TODO: center
    mainWindow->show();

    qRegisterMetaType<QVector<int>>(); //For multithreading.

    //Multi-threading
    refreshThread = new std::thread(MainUi::refresh, this);
  }

  bool eventFilter(QObject* obj, QEvent* ev) {
    QTreeWidget* procTreeWidget = chooseProc->findChild<QTreeWidget*>("procTreeWidget");
    if(obj == procTreeWidget && ev->type() == QEvent::KeyRelease) {
      if(static_cast<QKeyEvent*>(ev)->key() == Qt::Key_Return) { //Use Return instead of Enter
        onProcItemDblClicked(procTreeWidget->currentItem(), 0); //Just use the first column
      }
    }
  }

  void updateNumberOfAddresses(QWidget* mainWindow) {
    char message[128];
    sprintf(message, "%ld addresses found", med.scanAddresses.size());
    mainWindow->findChild<QStatusBar*>("statusbar")->showMessage(message);
  }

  /**
   * @deprecated
   */
  static QModelIndex getTreeViewSelectedModelIndex(QTreeView* treeView) {
    QModelIndexList selectedIndexes = treeView->selectionModel()->selectedIndexes();
    if (selectedIndexes.count() == 0) {
      return QModelIndex();
    }
    return selectedIndexes.first();
  }

  /**
   * @deprecated
   */
  static int getTreeViewSelectedIndex(QTreeView* treeView) {
    QModelIndex modelIndex = MainUi::getTreeViewSelectedModelIndex(treeView);
    if (!modelIndex.isValid()) {
      return -1;
    }
    return modelIndex.row();
  }

};

int main(int argc, char **argv) {
  QApplication app(argc, argv);
  MainUi* mainUi = new MainUi();
  return app.exec();
}

#include "med-qt.moc"