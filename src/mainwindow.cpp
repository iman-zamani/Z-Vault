#include <iostream>
#include <string>
#include <cstring> 
#include "mainwindow.h"
#include <QDateTime>
#include <QVBoxLayout>
#include <QApplication>
#include <QScreen>
#include <QHeaderView>
#include <QClipboard>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QInputDialog>
#include <QShortcut>
#include "encrypt.h"
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    // table of passwords setup
    table = new QTableWidget(this);
    table->setColumnCount(8); 
    QStringList tableHeader{"Website", "User Name", "Password", " ", " ", " ", " ", "Creation Time"};
    table->setHorizontalHeaderLabels(tableHeader);
    table->setRowCount(0);
    table->setEditTriggers(QAbstractItemView::DoubleClicked);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setColumnHidden(7, true);  // hide the creation time column

    // add search bar
    searchBar = new QLineEdit(this);
    searchBar->setPlaceholderText("Search by Website or User Name...");
    connect(searchBar, &QLineEdit::textChanged, this, &MainWindow::searchTable);

    // load table data
    loadTableData();

    //------------------------------------------------------------------------------------
    // UI setup
    addRowButton = new QPushButton("Add Row", this);
    connect(addRowButton, &QPushButton::clicked, this, &MainWindow::addRow);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(searchBar); 
    layout->addWidget(table);
    layout->addWidget(addRowButton);

    QWidget *widget = new QWidget();
    widget->setLayout(layout);
    setCentralWidget(widget);
    QScreen *screen = QApplication::screens().at(0);
    QRect screenSize = screen->availableGeometry();
    resize(screenSize.width() / 2, screenSize.height() / 2);
    // shortcut setup 
    QShortcut *shortcutAddRow = new QShortcut(QKeySequence("Ctrl+T"), this);
    connect(shortcutAddRow, &QShortcut::activated, this, &MainWindow::addRow);
    QShortcut *shortcutFocusSearchBar = new QShortcut(QKeySequence("Ctrl+F"), this);
    connect(shortcutFocusSearchBar, &QShortcut::activated, this, [this]() {
        searchBar->setFocus();
    });
}

MainWindow::~MainWindow()
{

}

void setStringToSpaces(std::string& str) {
    for (char& c : str) {
        c = ' ';  
    }
}

void setQStringToSpaces(QString& qStr) {
    qStr.fill(' ');  
    qStr.clear();
    qStr.squeeze();
}

void MainWindow::closeEvent(QCloseEvent *event ){
    // Set the main window as the parent of the QMessageBox for the cancel option 
    QMessageBox msgBox(this); 
    msgBox.setWindowTitle("Save Data");
    msgBox.setText("Do you want to save the changes to the table?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Yes);

    // center the pop up base on window 
    msgBox.move(this->geometry().center() - msgBox.rect().center());

    int ret = msgBox.exec();

    if (ret == QMessageBox::Yes) {
        saveTableData(event,searchBar);
        event->accept(); // save and then close the app
    } else if (ret == QMessageBox::No) {
        event->accept();  // close the app 
    } else {
        event->ignore();  // keep the app open
    }
}

std::string MainWindow::getUserPassword() {
    QInputDialog inputDialog(this);
    inputDialog.setModal(true); 
    inputDialog.setLabelText(tr("Enter your password:"));
    inputDialog.setWindowTitle(tr("Authentication"));
    inputDialog.setTextEchoMode(QLineEdit::Password);

    QSize screenSize = this->screen()->size();  
    int width = screenSize.width() / 4;
    int height = screenSize.height() / 3;
    int x = (screenSize.width() - width) / 2;
    int y = (screenSize.height() - height) / 2;
    inputDialog.resize(width, height);
    inputDialog.move(x, y);

    bool ok = inputDialog.exec() == QDialog::Accepted;
    QString password = inputDialog.textValue();

    // attempt to clear memory
    std::string stdPassword;
    if (ok && !password.isEmpty()) {
        stdPassword = password.toStdString();
        if (stdPassword == "INVALID"){
            QMessageBox::warning(this, tr("Invalid Input"),
                             tr("You must enter a valid password."));
                             return "INVALID";
        }
        // clear the QString password data
        setQStringToSpaces(password);
        return stdPassword;
    } else {
        QMessageBox::warning(this, tr("Input Required"),
                             tr("You must enter a password to proceed."));
        return "INVALID"; 
    }
    // clear stdPassword
    //std::memset(&stdPassword[0], 0, stdPassword.size());
    //stdPassword.clear();
    //setStringToSpaces(stdPassword);
    return stdPassword;
}

QString copyFileContent(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString(); 
    }

    QTextStream stream(&file);
    QString contents = stream.readAll();
    file.close(); 

    return contents;
}

void MainWindow::saveTableData(QCloseEvent *event, QLineEdit *searchBar) {
    QString copyFile = copyFileContent(filePath); // backup 
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("Save Error"), tr("Unable to open file for saving. Please check your permissions or disk space."));
        return;
    }
    searchBar->setText("");
    searchTable("");
    QTextStream stream(&file);
    QString longString;

    // a list to store rows along with their creation time
    QList<QPair<QDateTime, QString>> rowsWithTime;

    for (int i = 0; i < table->rowCount(); ++i) {
        QString rowString;
        for (int j = 0; j < 3; ++j) { 
            QTableWidgetItem *item = table->item(i, j);
            if (item) {
                rowString += item->text();
            } else {
                rowString += " ";
            }
            rowString += ";"; // cell separator
        }


        QTableWidgetItem *timeItem = table->item(i, 7); 
        if (timeItem) {
            rowString += timeItem->text();
        } else {
            rowString += " ";
        }

        QDateTime creationTime = QDateTime::fromString(timeItem->text(), Qt::ISODate);
        rowsWithTime.append(QPair<QDateTime, QString>(creationTime, rowString));
    }

    // sort rows based on creation time
    std::sort(rowsWithTime.begin(), rowsWithTime.end(), [](const QPair<QDateTime, QString>& a, const QPair<QDateTime, QString>& b) {
        return a.first < b.first;
    });

    for (int i = 0; i < rowsWithTime.size(); ++i) {
        longString += rowsWithTime[i].second;
        if (i < rowsWithTime.size() - 1) {
            longString += "|";
        }
    }

    // authenticate user 
    bool authenticated = false;
    std::string enteredPassword;
    for (int attempts = 0; attempts < 3; ++attempts) {
        enteredPassword = getUserPassword();
        if (enteredPassword.empty()) {
            QMessageBox::warning(this, tr("Authentication Required"), tr("You must enter a password to proceed."));
            continue;
        }

        if (checkPassword({reinterpret_cast<const unsigned char*>(enteredPassword.data()), enteredPassword.size()}, userPasswordHash, userSalt)) {
            authenticated = true;
            break;
        } else {
            QMessageBox::warning(this, tr("Authentication Failed"), tr("Incorrect password. Please try again."));
        }
    }

    if (!authenticated) {
        QMessageBox::warning(this, tr("Authentication Failed"), tr("Maximum retry attempts reached."));
        stream << copyFile; // restore previous content
        file.close();
        return;
    }

    // encrypt and save the data
    std::string plainText = longString.toStdString();
    std::string encodedCiphertextString = EncryptString(plainText, enteredPassword);
    QString saveString = QString::fromStdString(encodedCiphertextString);
    stream << saveString;

    file.close();

    // clear sensitive data
    setStringToSpaces(plainText);
    setStringToSpaces(enteredPassword);
    setStringToSpaces(encodedCiphertextString);
    setQStringToSpaces(longString);
}

bool MainWindow::loadTableData() {
    std::string userPassword = getUserPassword();

    // if the password was not entered or it was invalid
    if (userPassword == "INVALID") {
        exit(1);
    }

    // save password hash
    if (!userPassword.empty()) {
        userSalt = generateSalt();
        CryptoPP::SecByteBlock password(reinterpret_cast<const unsigned char*>(userPassword.data()), userPassword.size());
        userPasswordHash = savePassword(password, userSalt);
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QTextStream stream(&file);

    // Read the entire file content into a single string
    QString encodedCiphertextQString = stream.readAll();
    file.close();
    std::string encodedCiphertextString = encodedCiphertextQString.toStdString();

    // Decrypt the file content
    std::string decodedCiphertextString = DecryptString(encodedCiphertextString, userPassword);
    if (decodedCiphertextString.empty()) {
        // If decryption fails, return false
        return false;
    }

    QString longString = QString::fromStdString(decodedCiphertextString);

    // split into rows
    QStringList rows = longString.split("|");

    for (QString &row : rows) {
        QStringList values = row.split(";");

        if (values.size() < 4) {
            continue;
        }

        addRow();
        int lastRow = table->rowCount() - 1;

        // set website, username, and password
        for (int i = 0; i < 3; ++i) {
            QTableWidgetItem *item = new QTableWidgetItem(values.at(i));
            table->setItem(lastRow, i, item);
        }

        // set the creation time in the hidden column 7
        QTableWidgetItem *timeItem = new QTableWidgetItem(values.at(3));
        table->setItem(lastRow, 7, timeItem);
    }

    // clear sensitive data
    setStringToSpaces(decodedCiphertextString);
    setStringToSpaces(userPassword);

    return true;
}

void MainWindow::addRow() {
    int rowCount = table->rowCount();
    table->insertRow(rowCount);

    // store the creation time part 
    QDateTime currentTime = QDateTime::currentDateTime();
    QTableWidgetItem *timeItem = new QTableWidgetItem(currentTime.toString(Qt::ISODate));
    table->setItem(rowCount, 7, timeItem); 


    QPushButton *removeButton = new QPushButton("Remove Row");
    removeButton->setStyleSheet("background-color: red;");
    connect(removeButton, &QPushButton::clicked, this, &MainWindow::removeRow);
    table->setCellWidget(rowCount, 3, removeButton);

    QPushButton *copyUserNameButton = new QPushButton("Copy User Name");
    copyUserNameButton->setStyleSheet("background-color: darkblue; color: white;");
    connect(copyUserNameButton, &QPushButton::clicked, this, &MainWindow::copyUserNameButton);
    table->setCellWidget(rowCount, 4, copyUserNameButton);

    QPushButton *copyPasswordButton = new QPushButton("Copy Password");
    copyPasswordButton->setStyleSheet("background-color: darkblue; color: white;");
    connect(copyPasswordButton, &QPushButton::clicked, this, &MainWindow::copyPasswordButton);
    table->setCellWidget(rowCount, 5, copyPasswordButton);

    QPushButton *generatePassword = new QPushButton("Generate Password");
    generatePassword->setStyleSheet("background-color: green; color: white;");
    connect(generatePassword, &QPushButton::clicked, this, &MainWindow::generatePassword);
    table->setCellWidget(rowCount, 6, generatePassword);
}

void MainWindow::copyUserNameButton(){
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    int row = table->indexAt(button->pos()).row();
    if (row != -1) {
        QTableWidgetItem *item = table->item(row, 1);  // username is index 1 
        if (item) {
            QString text = item->text();
            QApplication::clipboard()->setText(text);
        }
    }
}

void MainWindow::copyPasswordButton(){
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    int row = table->indexAt(button->pos()).row();
    if (row != -1) {
        QTableWidgetItem *item = table->item(row, 2);  // password is index 2
        if (item) {
            QString text = item->text();
            QApplication::clipboard()->setText(text);
        }
    }
}

void MainWindow::generatePassword(){
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (!button) return;

    int row = table->indexAt(button->pos()).row();
    int passwordColumn = 2;  // password is index 2

    if (row != -1) {
        QTableWidgetItem *item = table->item(row, passwordColumn);
        if (!item) {
            // If the item does not exist, create it
            item = new QTableWidgetItem();
            table->setItem(row, passwordColumn, item);
        }
        std::string temp = generateRandomPassword(40);
        QString pass =  QString::fromStdString(temp);
        item->setText(pass);
        setStringToSpaces(temp);
        setQStringToSpaces(pass);
    }
}

void MainWindow::removeRow(){
    searchBar->setText("");
    searchTable("");
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (!button) return;

    int row = table->indexAt(button->pos()).row();
    if (row != -1) {
        table->removeRow(row);
    }
}

void MainWindow::searchTable(const QString &query) {
    QList<QPair<int, int>> matchingRows; // index, relevance
    QList<int> nonMatchingRows;

    // temporarily store all rows
    QList<QList<QTableWidgetItem*>> allRows;

    // store creation time for each row (for sorting)
    QList<QPair<QDateTime, QList<QTableWidgetItem*>>> rowsWithTime;

    for (int i = 0; i < table->rowCount(); ++i) {
        QList<QTableWidgetItem*> rowItems;
        for (int col = 0; col < table->columnCount(); ++col) {
            rowItems.append(table->takeItem(i, col));
        }

        // add creation time for sorting if necessary (column 7)
        QTableWidgetItem *timeItem = rowItems.at(7); // Column 7 is for creation time
        if (timeItem) {
            // Parse the creation time as ISODate (Qt::ISODate)
            QDateTime creationTime = QDateTime::fromString(timeItem->text(), Qt::ISODate);
            rowsWithTime.append(QPair<QDateTime, QList<QTableWidgetItem*>>(creationTime, rowItems));
        }

        allRows.append(rowItems);
    }

    // clear all rows from the table
    table->setRowCount(0);

    if (!query.isEmpty()) {
        for (int i = 0; i < allRows.size(); ++i) {
            QTableWidgetItem *websiteItem = allRows[i].at(0);
            QTableWidgetItem *userItem = allRows[i].at(1);

            int relevance = 0;

            if (websiteItem && websiteItem->text().contains(query, Qt::CaseInsensitive)) {
                relevance += 1;
            }

            if (userItem && userItem->text().contains(query, Qt::CaseInsensitive)) {
                relevance += 1;
            }

            if (relevance > 0) {
                // store index and relevance
                matchingRows.append(QPair<int, int>(i, relevance));  
            } else {
                // rows that don't match the query
                nonMatchingRows.append(i);  
            }
        }

        // sort matching rows by relevance
        std::sort(matchingRows.begin(), matchingRows.end(), [](const QPair<int, int> &a, const QPair<int, int> &b) {
            return a.second > b.second;
        });

        // insert matching rows back into the table
        for (const auto &rowPair : matchingRows) {
            int rowIdx = rowPair.first;
            int newRow = table->rowCount();
            table->insertRow(newRow);

            for (int col = 0; col < table->columnCount(); ++col) {
                table->setItem(newRow, col, allRows[rowIdx].at(col));
            }

            // add buttons to the row
            QPushButton *removeButton = new QPushButton("Remove Row");
            removeButton->setStyleSheet("background-color: red;");
            connect(removeButton, &QPushButton::clicked, this, &MainWindow::removeRow);
            table->setCellWidget(newRow, 3, removeButton);

            QPushButton *copyUserNameButton = new QPushButton("Copy User Name");
            copyUserNameButton->setStyleSheet("background-color: darkblue; color: white;");
            connect(copyUserNameButton, &QPushButton::clicked, this, &MainWindow::copyUserNameButton);
            table->setCellWidget(newRow, 4, copyUserNameButton);

            QPushButton *copyPasswordButton = new QPushButton("Copy Password");
            copyPasswordButton->setStyleSheet("background-color: darkblue; color: white;");
            connect(copyPasswordButton, &QPushButton::clicked, this, &MainWindow::copyPasswordButton);
            table->setCellWidget(newRow, 5, copyPasswordButton);

            QPushButton *generatePasswordButton = new QPushButton("Generate Password");
            generatePasswordButton->setStyleSheet("background-color: green; color: white;");
            connect(generatePasswordButton, &QPushButton::clicked, this, &MainWindow::generatePassword);
            table->setCellWidget(newRow, 6, generatePasswordButton);
        }

        // insert non matching rows (unfiltered ones)
        for (int rowIdx : nonMatchingRows) {
            int newRow = table->rowCount();
            table->insertRow(newRow);

            for (int col = 0; col < table->columnCount(); ++col) {
                table->setItem(newRow, col, allRows[rowIdx].at(col));
            }

            // add buttons to non matching rows
            QPushButton *removeButton = new QPushButton("Remove Row");
            removeButton->setStyleSheet("background-color: red;");
            connect(removeButton, &QPushButton::clicked, this, &MainWindow::removeRow);
            table->setCellWidget(newRow, 3, removeButton);

            QPushButton *copyUserNameButton = new QPushButton("Copy User Name");
            copyUserNameButton->setStyleSheet("background-color: darkblue; color: white;");
            connect(copyUserNameButton, &QPushButton::clicked, this, &MainWindow::copyUserNameButton);
            table->setCellWidget(newRow, 4, copyUserNameButton);

            QPushButton *copyPasswordButton = new QPushButton("Copy Password");
            copyPasswordButton->setStyleSheet("background-color: darkblue; color: white;");
            connect(copyPasswordButton, &QPushButton::clicked, this, &MainWindow::copyPasswordButton);
            table->setCellWidget(newRow, 5, copyPasswordButton);

            QPushButton *generatePasswordButton = new QPushButton("Generate Password");
            generatePasswordButton->setStyleSheet("background-color: green; color: white;");
            connect(generatePasswordButton, &QPushButton::clicked, this, &MainWindow::generatePassword);
            table->setCellWidget(newRow, 6, generatePasswordButton);
        }
    } else {
        // if the query is empty, sort by creation time
        std::sort(rowsWithTime.begin(), rowsWithTime.end(), [](const QPair<QDateTime, QList<QTableWidgetItem*>> &a, const QPair<QDateTime, QList<QTableWidgetItem*>> &b) {
            return a.first < b.first;
        });

        // insert rows sorted by creation time
        for (const auto &rowPair : rowsWithTime) {
            int newRow = table->rowCount();
            table->insertRow(newRow);

            for (int col = 0; col < table->columnCount(); ++col) {
                table->setItem(newRow, col, rowPair.second.at(col));
            }

            // add buttons to the row
            QPushButton *removeButton = new QPushButton("Remove Row");
            removeButton->setStyleSheet("background-color: red;");
            connect(removeButton, &QPushButton::clicked, this, &MainWindow::removeRow);
            table->setCellWidget(newRow, 3, removeButton);

            QPushButton *copyUserNameButton = new QPushButton("Copy User Name");
            copyUserNameButton->setStyleSheet("background-color: darkblue; color: white;");
            connect(copyUserNameButton, &QPushButton::clicked, this, &MainWindow::copyUserNameButton);
            table->setCellWidget(newRow, 4, copyUserNameButton);

            QPushButton *copyPasswordButton = new QPushButton("Copy Password");
            copyPasswordButton->setStyleSheet("background-color: darkblue; color: white;");
            connect(copyPasswordButton, &QPushButton::clicked, this, &MainWindow::copyPasswordButton);
            table->setCellWidget(newRow, 5, copyPasswordButton);

            QPushButton *generatePasswordButton = new QPushButton("Generate Password");
            generatePasswordButton->setStyleSheet("background-color: green; color: white;");
            connect(generatePasswordButton, &QPushButton::clicked, this, &MainWindow::generatePassword);
            table->setCellWidget(newRow, 6, generatePasswordButton);
        }
    }
}
