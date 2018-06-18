/*
 * Copyright © 2018 nastys
 *
 * This file is part of Title Switch.
 * Title Switch is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Title Switch is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Title Switch.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QTableWidgetItem>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void hourglass(bool show = true);

public slots:
    void hideHourglass() {hourglass(false);}

    void refreshTable() {fillTable();}

private slots:
    void on_commandLinkButton_layeredfs_compatibility_clicked();

    void on_comboBox_layeredfs_target_titleid_activated(int index);

    void on_lineEdit_layeredfs_target_titleid_textEdited(const QString &arg1);

    void on_pushButton_layeredfs_infile_browse_clicked();

    void on_pushButton_autodetect_clicked();

    void on_pushButton_layeredfs_browse_clicked();

    void on_tableWidget_layeredfs_folders_itemDoubleClicked(QTableWidgetItem *item);

    void on_pushButton_layeredfs_delete_clicked();

    void on_commandLinkButton_settings_tools_xci2nca_clicked();

    void on_commandLinkButton_layeredfs_start_clicked();

    void on_commandLinkButton_settings_tools_makekeystxt_clicked();

private:
    Ui::MainWindow *ui;
    void fillTable();
    bool toolsExist();
    QString getNameByID(QString id);
    QString getCustomTitleByID(QString id, QString path);
};

class XCIConverter : public QObject
{
    Q_OBJECT
private:
    QString input, output;
public:
    XCIConverter() {}
    ~XCIConverter() {}
    void setIn(QString in) {input=in;}
    void setOut(QString out) {output=out;}
public slots:
    void process();
signals:
    void finished();
};

class NCAConverter : public QObject
{
    Q_OBJECT
private:
    QString input, output, name;
public:
    NCAConverter() {}
    ~NCAConverter() {}
    void setIn(QString in) {input=in;}
    void setOut(QString out) {output=out;}
    void setN(QString n) {name=n;}
public slots:
    void process();
signals:
    void finished();
};

#endif // MAINWINDOW_H
