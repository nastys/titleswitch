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
#include <QMovie>
#include <QFile>
#include <QDesktopServices>
#include <QUrl>
#include <QFileDialog>
#include <QMessageBox>
#include <QDirIterator>
#include <QProcess>
#include <QThread>
#include <QStorageInfo>
#include <QTextStream>
#include <QDataStream>
#include <QByteArray>
#include <QCryptographicHash>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QInputDialog>

void logappend(QString text);
void convNCA(QString input, QString output, QString titleid, QString name, QString titlekey="");

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
    void swapHourglass(int anim = 1);

public slots:
    void hideHourglass() {hourglass(false);}

    void resetHourglassAnim();

    void refreshTable() {fillTable();}

private slots:
    void on_commandLinkButton_layeredfs_compatibility_clicked();

    void on_comboBox_layeredfs_target_titleid_activated(int index);

    void on_lineEdit_layeredfs_target_titleid_textEdited(const QString &arg1);

    void openFile(QString file);

    void on_pushButton_layeredfs_infile_browse_clicked();

    void on_pushButton_autodetect_clicked();

    void on_pushButton_layeredfs_browse_clicked();

    void on_tableWidget_layeredfs_folders_itemDoubleClicked(QTableWidgetItem *item);

    void on_pushButton_layeredfs_delete_clicked();

    void on_commandLinkButton_settings_tools_xci2nca_clicked();

    void on_commandLinkButton_layeredfs_start_clicked();

    void on_commandLinkButton_settings_tools_makekeystxt_clicked();

    void on_commandLinkButton_settings_tools_patchMainNpdm_clicked();

    void on_commandLinkButton_settings_tools_saveLog_clicked();

    void on_pushButton_layeredfs_setTarget_clicked();

    void on_pushButton_settings_about_clicked();

    void on_checkBox_layeredfs_patchingEnabled_stateChanged(int arg1);

    void removeTempNCA();

    void on_pushButton_layeredfs_explore_clicked();

    void on_commandLinkButton_plague_compatibility_clicked();

    void on_commandLinkButton_plague_start_clicked();

    void on_pushButton_plague_explore_clicked();

    void on_pushButton_plague_delete_clicked();

    void on_commandLinkButton_plague_migrate_clicked();

    void on_pushButton_layeredfs_rename_clicked();

    void on_pushButton_plague_rename_clicked();

    void on_pushButton_layeredfs_refresh_clicked();

    void on_pushButton_plague_refresh_clicked();

private:
    Ui::MainWindow *ui;
    void fillTable();
    void sd_browse(QString sd="");
    bool toolsExist();
    QString getNameByID(QString id);
    QString getCustomTitleByID(QString id, QString path);
    QMovie *hgmovie[3];

protected:
    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *e);
};

class XCIConverter : public QObject
{
    Q_OBJECT
private:
    QString input, output, ncaoutput, name, titleid, titlekey;
public:
    XCIConverter() {}
    ~XCIConverter() {}
    void setIn(QString in) {input=in;}
    void setOut(QString out) {output=out;}
    void setNCAout(QString ncaout="") {ncaoutput=ncaout;}
    void setN(QString n="") {name=n;}
    void setId(QString id="") {titleid=id;}
    void setTitlekey(QString tkey="") {titlekey=tkey;}
public slots:
    void process();
signals:
    void finished();
    void convFinished();
};

class NCAConverter : public QObject
{
    Q_OBJECT
private:
    QString input, output, name, titleid;
public:
    NCAConverter() {}
    ~NCAConverter() {}
    void setIn(QString in) {input=in;}
    void setOut(QString out) {output=out;}
    void setN(QString n) {name=n;}
    void setId(QString id) {titleid=id;}
public slots:
    void process();
signals:
    void finished();
};

void patchNpdm(QString id, QString path);

#endif // MAINWINDOW_H
