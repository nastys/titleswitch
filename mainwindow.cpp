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
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMovie>
#include <QDesktopServices>
#include <QUrl>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QDirIterator>
#include <QProcess>
#include <QThread>
#include <QStorageInfo>
#include <QTextStream>
#include <QDataStream>

QString title_id[12]={"",                 // Custom...
                      "0100C6E00AF2C000", // BlazBlue
                      "010043500A17A000", // Fallout Shelter
                      "010025400AECE000", // Fortnite
                      "0100A66003384000", // Hulu
                      "01000C900A136000", // Kitten Squad
                      "010096000B3EA000", // Octopath Traveler Demo
                      "0100BA3003B70000", // Pac-Man Vs.
                      "0100DB7003828000", // Pinball FX3
                      "01005D100807A000", // Pokémon Quest
                      "0100AE0006474000", // Stern Pinball Arcade
                      "0100CD300880E000", // The Pinball Arcade
                      };

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->hourglass->setVisible(false);
    QMovie* movie=new QMovie(":/anim/res/wait.webp");
    ui->hourglass->setMovie(movie);
    ui->hourglass->setAttribute(Qt::WA_NoSystemBackground);
    movie->start();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void XCIConverter::process()
{
    QDir dir(output+".tmp");
    dir.removeRecursively();
    QFile::remove(output);
    dir.mkpath(output+".tmp");
    QStringList args;
    args << "-x" << "-k" << "keys.txt" << "-t" << "xci" << "--outdir="+output+".tmp" << input;
    QProcess process;
    process.start("hactool", args);
    process.waitForFinished(-1);
    QFile logfile("log.dat");
    logfile.open(QIODevice::ReadWrite);
    QDataStream log(&logfile);
    log << process.program() << process.arguments() << process.readAll();
    logfile.close();
    QDirIterator secdir(output+".tmp/secure/");
    QString largestfile;
    QFile c;
    int largestsize=0;
    for(int i=0; secdir.hasNext(); i++)
    {
        secdir.next();
        c.setFileName(secdir.filePath());
        c.open(QIODevice::ReadOnly);
        if(c.size()>largestsize)
            largestfile=secdir.filePath();
        c.close();
    }
    QFile::rename(largestfile, output);
    dir.removeRecursively();
    emit finished();
}

void NCAConverter::process()
{
    QDir dir(output);
    dir.mkpath(output+"/exefs");
    QStringList args;
    args << "-x" << "-k" << "keys.txt" << "-t" << "nca" << "--romfs="+output+"/romfs" << "--exefsdir="+output+"/exefs" << input;
    QProcess process;
    process.start("hactool", args);
    process.waitForFinished(-1);
    QFile logfile("log.dat");
    logfile.open(QIODevice::ReadWrite);
    QDataStream log(&logfile);
    log << process.program() << process.arguments() << process.readAll();
    logfile.close();
    QFile titlename(output+".txt");
    titlename.open(QIODevice::ReadWrite);
    titlename.resize(0);
    QTextStream textfile(&titlename);
    textfile << name;
    textfile.flush();
    titlename.close();
    emit finished();
}

void MainWindow::hourglass(bool show)
{
    if(show)
    {
        ui->hourglass->raise();
        ui->hourglass->setVisible(true);
    }
    else
    {
        ui->hourglass->setVisible(false);
        ui->hourglass->lower();
    }
}

QString MainWindow::getNameByID(QString id)
{
    if(id=="010000000000100D") return "Gallery";
    else if(id=="0100C6E00AF2C000") return "BlazBlue";
    else if(id=="010043500A17A000") return "Fallout Shelter";
    else if(id=="010025400AECE000") return "Fortnite";
    else if(id=="0100A66003384000") return "Hulu";
    else if(id=="01000C900A136000") return "Kitten Squad";
    else if(id=="010096000B3EA000") return "Octopath Traveler Demo";
    else if(id=="0100BA3003B70000") return "Pac-Man Vs.";
    else if(id=="0100DB7003828000") return "Pinball FX3";
    else if(id=="01005D100807A000") return "Pokémon Quest";
    else if(id=="0100AE0006474000") return "Stern Pinball Arcade";
    else if(id=="0100CD300880E000") return "The Pinball Arcade";
    else if(id=="0100000000010000") return "Super Mario Odyssey";
    else return "(unknown)";
}

QString MainWindow::getCustomTitleByID(QString id, QString path)
{
    if(QFile::exists(path+id+".txt"))
    {
        QFile file(path+id+".txt");
        file.open(QIODevice::ReadOnly);
        QString contents=file.readAll();
        file.close();
        return contents;
    }
    else if(id=="010000000000100D") return "Homebrew Launcher";
    else return "(unknown/mod)";
}

void MainWindow::fillTable()
{
    hourglass(true);

    ui->tableWidget_layeredfs_folders->setRowCount(0);

    QDirIterator sd_folder(ui->lineEdit_layeredfs_path->text()+"/atmosphere/titles/", QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
    for(int i=0; sd_folder.hasNext(); i++)
    {
        sd_folder.next();
        ui->tableWidget_layeredfs_folders->insertRow(i);
        ui->tableWidget_layeredfs_folders->setItem(i, 0, new QTableWidgetItem(QString(sd_folder.fileName())));
        ui->tableWidget_layeredfs_folders->setItem(i, 1, new QTableWidgetItem(QString(getNameByID(sd_folder.fileName()))));
        ui->tableWidget_layeredfs_folders->setItem(i, 2, new QTableWidgetItem(QString(getCustomTitleByID(sd_folder.fileName(), ui->lineEdit_layeredfs_path->text()+"/atmosphere/titles/"))));
    }

    hourglass(false);
}

void MainWindow::on_commandLinkButton_layeredfs_compatibility_clicked()
{
    QDesktopServices::openUrl(QUrl("https://lfsdb.com/"));
}

void MainWindow::on_comboBox_layeredfs_target_titleid_activated(int index)
{
    ui->lineEdit_layeredfs_target_titleid->setText(title_id[index]);
}

void MainWindow::on_lineEdit_layeredfs_target_titleid_textEdited(const QString &arg1)
{
    if(arg1.toUpper()=="0100C6E00AF2C000") ui->comboBox_layeredfs_target_titleid->setCurrentIndex(1);
    else if(arg1.toUpper()=="010043500A17A000") ui->comboBox_layeredfs_target_titleid->setCurrentIndex(2);
    else if(arg1.toUpper()=="010025400AECE000") ui->comboBox_layeredfs_target_titleid->setCurrentIndex(3);
    else if(arg1.toUpper()=="0100A66003384000") ui->comboBox_layeredfs_target_titleid->setCurrentIndex(4);
    else if(arg1.toUpper()=="01000C900A136000") ui->comboBox_layeredfs_target_titleid->setCurrentIndex(5);
    else if(arg1.toUpper()=="010096000B3EA000") ui->comboBox_layeredfs_target_titleid->setCurrentIndex(6);
    else if(arg1.toUpper()=="0100BA3003B70000") ui->comboBox_layeredfs_target_titleid->setCurrentIndex(7);
    else if(arg1.toUpper()=="0100DB7003828000") ui->comboBox_layeredfs_target_titleid->setCurrentIndex(8);
    else if(arg1.toUpper()=="01005D100807A000") ui->comboBox_layeredfs_target_titleid->setCurrentIndex(9);
    else if(arg1.toUpper()=="0100AE0006474000") ui->comboBox_layeredfs_target_titleid->setCurrentIndex(10);
    else if(arg1.toUpper()=="0100CD300880E000") ui->comboBox_layeredfs_target_titleid->setCurrentIndex(11);
    else ui->comboBox_layeredfs_target_titleid->setCurrentIndex(0);
}

void MainWindow::on_pushButton_layeredfs_infile_browse_clicked()
{
    QString file=QFileDialog::getOpenFileName(this, "Open NCA file", QDir::homePath(), "NCA (*.nca *.xci)");
    if(file==NULL) return;

    if(file.endsWith(".xci", Qt::CaseInsensitive))
    {
        QMessageBox::information(this, "Open NCA file", "This is an XCI file and must be converted before proceeding.");
        ui->tabWidget->setCurrentIndex(0);
        return;
    }

    ui->lineEdit_layeredfs_infile_file->setText(file);
    ui->lineEdit_layeredfs_infile_name->setText((QFileInfo(QFile(file))).fileName());
    ui->commandLinkButton_layeredfs_start->setEnabled(true);
}

void MainWindow::on_pushButton_autodetect_clicked()
{
    hourglass(true);

#if defined(Q_OS_LINUX)
    QDirIterator media_folder("/media/", QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
    for(int i=0; media_folder.hasNext(); i++)
    {
        media_folder.next();
        QDirIterator sd_folder(media_folder.filePath()+"/", QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
        for(int i=0; sd_folder.hasNext(); i++)
        {
            sd_folder.next();
            if(QFile::exists(sd_folder.filePath()+"/atmosphere/titles/"))
            {
                ui->lineEdit_layeredfs_path->setText(sd_folder.filePath()+"/");
                fillTable();
                ui->commandLinkButton_layeredfs_start->setEnabled(true);
                hourglass(false);
                return;
            }
        }
    }
#elif defined(Q_OS_DARWIN)
    QDirIterator sd_folder("/Volumes/", QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
    for(int i=0; sd_folder.hasNext(); i++)
    {
        sd_folder.next();
        if(QFile::exists(sd_folder.filePath()+"/atmosphere/titles/"))
        {
            ui->lineEdit_layeredfs_path->setText(sd_folder.filePath()+"/");
            fillTable();
            ui->commandLinkButton_layeredfs_start->setEnabled(true);
            hourglass(false);
            return;
        }
    }
#elif defined(Q_OS_WIN)
    for(char c='A'; c<'Z'; c++)
    {
        if(QFile::exists(QString(c)+":/atmosphere/titles/"))
        {
            ui->lineEdit_layeredfs_path->setText(QString(c)+":/");
            fillTable();
            ui->commandLinkButton_layeredfs_start->setEnabled(true);
            hourglass(false);
            return;
        }
    }
#endif

    hourglass(false);
    QMessageBox::warning(this, "Detect SD", "SD card not detected. Is Atmosphère installed?");
}

void MainWindow::on_pushButton_layeredfs_browse_clicked()
{
#if defined(Q_OS_DARWIN)
    const QString volumedir="/Volumes/";
#elif defined(Q_OS_LINUX)
    const QString volumedir="/media/";
#else
    const QString volumedir="";
#endif

    QString sd=QFileDialog::getExistingDirectory(this, "Open SD card", volumedir);
    if(sd!=NULL)
    {
        if(QFile::exists(sd+"/atmosphere/titles/"))
        {
            ui->lineEdit_layeredfs_path->setText(sd);
            fillTable();
        }
        else
            QMessageBox::critical(this, "Error", "Could not find \"atmosphere/titles/\".");
    }
}

void MainWindow::on_tableWidget_layeredfs_folders_itemDoubleClicked(QTableWidgetItem *item)
{
    if(ui->tableWidget_layeredfs_folders->column(item)==0)
    {
        ui->lineEdit_layeredfs_target_titleid->setText(item->text());
        on_lineEdit_layeredfs_target_titleid_textEdited(item->text());
    }
}

void MainWindow::on_pushButton_layeredfs_delete_clicked()
{
    if(ui->tableWidget_layeredfs_folders->selectedItems().isEmpty())
        return;

    QString path=ui->lineEdit_layeredfs_path->text()+"/atmosphere/titles/"+ui->tableWidget_layeredfs_folders->item(ui->tableWidget_layeredfs_folders->selectedItems()[0]->row(), 0)->text();
    if(QFile::exists(path))
        if(QMessageBox::question(this, "Delete title", "Are you sure you want to delete this title?\n\n"+
                              ui->tableWidget_layeredfs_folders->item(ui->tableWidget_layeredfs_folders->selectedItems()[0]->row(), 1)->text()+
                        " → "+ui->tableWidget_layeredfs_folders->item(ui->tableWidget_layeredfs_folders->selectedItems()[0]->row(), 2)->text()+
                        "\n\n"+path)==QMessageBox::Yes)
        {
            hourglass(true);
            QDir dir(path);
            dir.removeRecursively();
            QFile::remove(path+".txt");
            fillTable();
            hourglass(false);
        }
}

bool MainWindow::toolsExist()
{
#if defined(Q_OS_WIN)
    const QString hactool="hactool.exe";
#else
    const QString hactool="hactool";
#endif

    if(QFile::exists(hactool)==false)
    {
        QMessageBox::critical(this, "Error", "Could not find "+hactool+" in \""+QDir::currentPath()+"\".");
        return false;
    }

    if(QFile::exists("keys.txt")==false)
    {
        QMessageBox::critical(this, "Error", "Could not find keys.txt in \""+QDir::currentPath()+"\".");
        ui->tabWidget->setCurrentIndex(0);
        return false;
    }

    return true;
}

void MainWindow::on_commandLinkButton_settings_tools_xci2nca_clicked()
{
    if(toolsExist()==false)
        return;

    QString in=QFileDialog::getOpenFileName(this, "Open XCI file", QDir::homePath(), "XCI file (*.xci)");
    if(in==NULL) return;
    QString out=QFileDialog::getSaveFileName(this, "Save NCA file as", QDir::homePath(), "NCA file (*.nca)");
    if(out==NULL) return;

    if(out.endsWith(".nca", Qt::CaseInsensitive)==false)
        out=out+".nca";

    hourglass(true);
    XCIConverter* xciconverter=new XCIConverter;
    xciconverter->setIn(in);
    xciconverter->setOut(out);
    QThread* thread=new QThread;
    xciconverter->moveToThread(thread);
    connect(thread, SIGNAL (started()), xciconverter, SLOT (process()));
    connect(xciconverter, SIGNAL (finished()), this, SLOT (hideHourglass()));
    connect(xciconverter, SIGNAL (finished()), thread, SLOT (quit()));
    connect(xciconverter, SIGNAL (finished()), xciconverter, SLOT (deleteLater()));
    connect(thread, SIGNAL (finished()), thread, SLOT (deleteLater()));
    thread->start();
}

void MainWindow::on_commandLinkButton_layeredfs_start_clicked()
{
    if(toolsExist()==false)
        return;

    if(QFile::exists(ui->lineEdit_layeredfs_path->text()+"/atmosphere/titles/"+ui->lineEdit_layeredfs_target_titleid->text()))
    {
        if(QMessageBox::question(this, "Title Switch", "WARNING: The selected target is currently in use.\nDelete the current title and continue?\n\n"+getCustomTitleByID(ui->lineEdit_layeredfs_target_titleid->text(), ui->lineEdit_layeredfs_path->text()+"/atmosphere/titles/")+" → "+ui->lineEdit_layeredfs_infile_name->text(), QMessageBox::Yes, QMessageBox::No)==QMessageBox::Yes)
        {
            QDir del(ui->lineEdit_layeredfs_path->text()+"/atmosphere/titles/"+ui->lineEdit_layeredfs_target_titleid->text());
            del.removeRecursively();
        }
        else return;
    }

    QStorageInfo storage;
    QFile infile(ui->lineEdit_layeredfs_infile_file->text());
    storage.setPath(ui->lineEdit_layeredfs_path->text());
    if(storage.bytesAvailable()<infile.size())
    {
        QMessageBox::critical(this, "Title Switch", "At least "+QString::number(infile.size()/1000000)+" MB are required for this title.\nSpace available on the destination drive: "+QString::number(storage.bytesAvailable()/1000000)+" MB.");
        infile.close();
        return;
    }
    infile.close();

    hourglass(true);
    NCAConverter* ncaconverter=new NCAConverter;
    ncaconverter->setIn(ui->lineEdit_layeredfs_infile_file->text());
    ncaconverter->setOut(ui->lineEdit_layeredfs_path->text()+"/atmosphere/titles/"+ui->lineEdit_layeredfs_target_titleid->text());
    ncaconverter->setN(ui->lineEdit_layeredfs_infile_name->text());
    QThread* thread=new QThread;
    ncaconverter->moveToThread(thread);
    connect(thread, SIGNAL (started()), ncaconverter, SLOT (process()));
    connect(ncaconverter, SIGNAL (finished()), this, SLOT (hideHourglass()));
    connect(ncaconverter, SIGNAL (finished()), this, SLOT (refreshTable()));
    connect(ncaconverter, SIGNAL (finished()), thread, SLOT (quit()));
    connect(ncaconverter, SIGNAL (finished()), ncaconverter, SLOT (deleteLater()));
    connect(thread, SIGNAL (finished()), thread, SLOT (deleteLater()));
    thread->start();
}

void MainWindow::on_commandLinkButton_settings_tools_makekeystxt_clicked()
{
    QDesktopServices::openUrl(QUrl("https://gbatemp.net/threads/how-to-get-switch-keys-for-hactool-xci-decrypting.506978/"));
#if defined(Q_OS_DARWIN)
    QDesktopServices::openUrl(QUrl("file://"+QDir::currentPath()));
#endif
}
