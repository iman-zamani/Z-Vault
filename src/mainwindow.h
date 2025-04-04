#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <iostream>
#include <fstream>
#include <string>
#include <cryptopp/modes.h>
#include <cryptopp/aes.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <cryptopp/pwdbased.h>
#include <cryptopp/sha.h>
#include <cryptopp/osrng.h>
#include <cryptopp/gcm.h>
#include <cryptopp/secblock.h> 

#include <QMainWindow>
#include <QTableWidget>
#include <QPushButton>
#include <QCloseEvent>
#include <QInputDialog>
#include <QString>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow(); 

protected:
    void closeEvent(QCloseEvent *event) override; 
     std::string getUserPassword();
private slots:
    void addRow();
    void removeRow();
    void copyUserNameButton();
    void copyPasswordButton();
    void generatePassword();
    void saveTableData(QCloseEvent *event,QLineEdit *searchBar);
    bool loadTableData();
    void searchTable(const QString &query);

private:
    QLineEdit *searchBar;
    QTableWidget *table;
    QPushButton *addRowButton;
    QString filePath = "password.enc"; 
    std::string userPasswordHash;
    CryptoPP::SecByteBlock userSalt;
    QList<QList<QTableWidgetItem*>> originalRows;
};

#endif 
