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

QByteArray logarray;
QFile logfiletmp(QDir::tempPath()+"/title_switch.log");
QString hactoolpath="hactool", keychain="keys.txt", titledatafile, tempfolder;
bool autoPatchMainNpdm=true;
QTabWidget** tabWidgetPtr;

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

void logappend(QString text)
{
    logarray.append(text);
    logfiletmp.resize(0);
    logfiletmp.write(logarray);
    logfiletmp.flush();
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    hactoolpath=QApplication::applicationDirPath()+"/hactool";
    keychain=QApplication::applicationDirPath()+"/keys.txt";
    ui->hourglass->setVisible(false);
    hgmovie[0] = new QMovie(":/anim/res/wait0.webp");
    hgmovie[1] = new QMovie(":/anim/res/wait1.webp");
    hgmovie[2] = new QMovie(":/anim/res/wait2.webp");
    for(int i=2; i>=0; i--) hgmovie[i]->start();

    logfiletmp.open(QIODevice::ReadWrite);
    logfiletmp.resize(0);

    tabWidgetPtr=&(ui->tabWidget);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::resetHourglassAnim()
{
    if((*tabWidgetPtr)->currentIndex()==2)
        swapHourglass(2);
    else
        swapHourglass(0);
}

void XCIConverter::process()
{
    QDir dir(output+".tmp");
    dir.removeRecursively();
    QFile::remove(output);
    dir.mkpath(output+".tmp");
    QString type="xci", ncadir="/secure";
    if(input.endsWith(".nsp", Qt::CaseInsensitive))
    {
        type="pfs0";
        ncadir="";
    }
    QStringList args;
    args << "-x" << "-k" << keychain << "-t" << type << "--outdir="+output+".tmp" << input;
    QProcess process;
    process.start(hactoolpath, args);
    process.waitForFinished(-1);
    logappend(process.program());
    logappend(process.arguments().join(" "));
    logappend(process.readAll());
    QDirIterator secdir(output+".tmp/"+ncadir+"/", QStringList() << "*.nca", QDir::Files);
    QString largestfile;
    qint64 largestsize=0;
    for(int i=0; secdir.hasNext(); i++)
    {
        secdir.next();
        QFile c(secdir.filePath());
        c.open(QIODevice::ReadOnly);
        if(c.size()>largestsize)
        {
            largestfile=secdir.filePath();
            largestsize=c.size();
        }
        c.close();
    }
    QFile::rename(largestfile, output);
    dir.removeRecursively();

    if(ncaoutput!="")
    {
        emit convFinished();

        convNCA(output, ncaoutput, titleid, name, titlekey);

        QFile infile(output);
        infile.remove();
    }
    emit finished();
}

void NCAConverter::process()
{
    convNCA(input, output, titleid, name);
    emit finished();
}

void convNCA(QString input, QString output, QString titleid, QString name, QString titlekey)
{
    QDir dir(output);
    dir.mkpath(output+"/exefs");
    QStringList args[2];
    args[0] << "-x" << "-k" << keychain << "-t" << "nca" << "--romfs="+output+"/romfs" << input;
    if(titlekey=="") args[1] << "-x" << "-k" << keychain << "-t" << "nca" << "--exefsdir="+output+"/exefs" << input; // XCI, NCA
    else args[1] << "-x" << "-k" << keychain << "-t" << "nca" << "--titlekey="+titlekey << "--section0dir="+output+"/exefs" << input; // NSP
    for(int i=0; i<2; i++)
    {
        QProcess process;
        process.start(hactoolpath, args[i]);
        process.waitForFinished(-1);
        logappend(process.program()+" ");
        logappend(process.arguments().join(" "));
        logappend(process.readAll());
        process.close();
    }
    if((*tabWidgetPtr)->currentIndex()==1)
    {
        QFile titlename(output+".txt");
        titlename.open(QIODevice::ReadWrite);
        titlename.resize(0);
        titlename.write(name.toUtf8());
        titlename.flush();
        titlename.close();
    }
    if(QFile::exists(output+"/exefs/main.npdm")==false)
    {
        logappend("COULD NOT FIND "+output+"/exefs/main.npdm"+"\n");
        QMessageBox::critical(nullptr, "Title Switch", "Could not find main.npdm. Check your keys.txt and backup files.");
        return;
    }
    if(autoPatchMainNpdm && (*tabWidgetPtr)->currentIndex()==1)
        patchNpdm(titleid, output+"/exefs/main.npdm");
}

void MainWindow::hourglass(bool show)
{
    if(show)
    {
        ui->hourglass->raise();
        ui->hourglass->setVisible(true);
        ui->hourglass->setMovie(hgmovie[0]);
    }
    else
    {
        ui->hourglass->setVisible(false);
        ui->hourglass->lower();
    }
}

void MainWindow::swapHourglass(int anim)
{
    ui->hourglass->setMovie(hgmovie[anim]);
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

    ui->tableWidget_plague_folders->setRowCount(0);

    QDirIterator sd_folder2(ui->lineEdit_layeredfs_path->text()+"/backups/", QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
    for(int i=0; sd_folder2.hasNext(); i++)
    {
        sd_folder2.next();
        ui->tableWidget_plague_folders->insertRow(i);
        ui->tableWidget_plague_folders->setItem(i, 0, new QTableWidgetItem(QString(sd_folder2.fileName())));
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

void MainWindow::openFile(QString file)
{
    if(file.endsWith(".nca", Qt::CaseInsensitive)||file.endsWith(".xci", Qt::CaseInsensitive))
    {
        ui->lineEdit_layeredfs_infile_file->setText(file);
        ui->lineEdit_layeredfs_infile_name->setText((QFileInfo(QFile(file))).fileName().section(".", 0, 0));
    }
}

void MainWindow::on_pushButton_layeredfs_infile_browse_clicked()
{
    QString file=QFileDialog::getOpenFileName(this, "Open backup file", QDir::homePath(), "Nintendo Switch backup (*.nca *.xci)");
    if(file==NULL) return;

    openFile(file);
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
                ui->commandLinkButton_layeredfs_start->setEnabled(true);
                ui->commandLinkButton_plague_start->setEnabled(true);
                if(QFile::exists(sd_folder.filePath()+"/backups/")==false)
                    QDir().mkdir(sd_folder.filePath()+"/backups/");
                fillTable();
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
            ui->commandLinkButton_layeredfs_start->setEnabled(true);
            ui->commandLinkButton_plague_start->setEnabled(true);
            if(QFile::exists(sd_folder.filePath()+"/backups/")==false)
                QDir().mkdir(sd_folder.filePath()+"/backups/");
            fillTable();
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
            ui->commandLinkButton_layeredfs_start->setEnabled(true);
            ui->commandLinkButton_plague_start->setEnabled(true);
            if(QFile::exists(QString(c)+":/backups/")==false)
                QDir().mkdir(QString(c)+":/backups/");
            fillTable();
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
    sd_browse();
}

void MainWindow::sd_browse(QString sd)
{
#if defined(Q_OS_DARWIN)
    const QString volumedir="/Volumes/";
#elif defined(Q_OS_LINUX)
    const QString volumedir="/media/";
#else
    const QString volumedir="";
#endif

    if(sd=="") sd=QFileDialog::getExistingDirectory(this, "Open SD card", volumedir);

    if(sd!=NULL)
    {
        if(QFile::exists(sd+"/atmosphere/titles/"))
        {
            ui->lineEdit_layeredfs_path->setText(sd);
            ui->commandLinkButton_layeredfs_start->setEnabled(true);
            ui->commandLinkButton_plague_start->setEnabled(true);
            if(QFile::exists(sd+"/backups/")==false)
                QDir().mkdir(sd+"/backups/");
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
    QString bakdir="/atmosphere/titles/", delmsg;
    QTableWidget** table=&(ui->tableWidget_layeredfs_folders);
    if(ui->tabWidget->currentIndex()==2)
    {
        bakdir="/backups/";
        table=&(ui->tableWidget_plague_folders);
        if((*table)->selectedItems().isEmpty())
            return;
        delmsg="Are you sure you want to delete this backup?\n\n"+ui->tableWidget_plague_folders->item(ui->tableWidget_plague_folders->selectedItems()[0]->row(), 0)->text();
    }
    else
    {
        if((*table)->selectedItems().isEmpty())
            return;
        delmsg="Are you sure you want to delete this title?\n\n"+
            ui->tableWidget_layeredfs_folders->item(ui->tableWidget_layeredfs_folders->selectedItems()[0]->row(), 1)->text()+
      " → "+ui->tableWidget_layeredfs_folders->item(ui->tableWidget_layeredfs_folders->selectedItems()[0]->row(), 2)->text();
    }

    QString path=ui->lineEdit_layeredfs_path->text()+bakdir+(*table)->item((*table)->selectedItems()[0]->row(), 0)->text();
    if(QFile::exists(path) && QMessageBox::question(this, "Delete title", delmsg+"\n\n"+path)==QMessageBox::Yes)
    {
        hourglass(true);
        QDir dir(path);
        dir.removeRecursively();
        if(ui->tabWidget->currentIndex()==2) QFile::remove(path+".txt");
        fillTable();
        hourglass(false);
    }
}

bool MainWindow::toolsExist()
{
    QString hactoolext="";
#if defined(Q_OS_WIN)
    hactoolext=".exe";
#endif

    if(QFile::exists(hactoolpath+hactoolext)==false)
    {
        QString p=QFileDialog::getOpenFileName(this, "Open hactool"+hactoolext, QDir::homePath(), "hactool"+hactoolext+" (hactool"+hactoolext+")");
        if(p!=NULL)
        {
            if(QFile::copy(p, QApplication::applicationDirPath()+"/hactool"+hactoolext)==false)
            {
                hactoolpath=p;
                logappend("RUNNING HACTOOL FROM: "+hactoolpath);
                QMessageBox::warning(this, "Title Switch", "Could not copy hactool"+hactoolext+" to the application directory. Hactool will be run directly from the path you selected.");
            }
        }
        else return false;
    }

    if(QFile::exists(keychain)==false)
    {
        QString p=QFileDialog::getOpenFileName(this, "Open keys.txt", QDir::homePath(), "keys.txt (keys.txt)");
        if(p!=NULL)
        {
            if(QFile::copy(p, QApplication::applicationDirPath()+"/keys.txt")==false)
            {
                keychain=p;
                logappend("READING KEYCHAIN FROM: "+keychain);
                QMessageBox::warning(this, "Title Switch", "Could not copy keys.txt to the application directory. The keys will be read directly from the path you selected.");
            }
        }
        else return false;
    }

    return true;
}

void MainWindow::on_commandLinkButton_settings_tools_xci2nca_clicked()
{
    QString in=ui->lineEdit_layeredfs_infile_file->text();

    if(!(in.endsWith(".xci", Qt::CaseInsensitive))||in=="")
        on_pushButton_layeredfs_infile_browse_clicked();

    if(!(in.endsWith(".xci", Qt::CaseInsensitive))||!(QFile::exists(in) && toolsExist()))
        return;

    QString out=QFileDialog::getSaveFileName(this, "Save NCA file as", QDir::homePath(), "NCA file (*.nca)");
    if(out==NULL) return;

    if(out.endsWith(".nca", Qt::CaseInsensitive)==false)
        out.append(".nca");

    hourglass(true);
    swapHourglass(1);
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
    if(ui->lineEdit_layeredfs_infile_file->text()=="")
        on_pushButton_layeredfs_infile_browse_clicked();

    if(ui->lineEdit_layeredfs_infile_file->text()==""||!(QFile::exists(ui->lineEdit_layeredfs_infile_file->text()) && toolsExist()))
        return;

    /*if(ui->lineEdit_layeredfs_infile_file->text().endsWith(".nsp", Qt::CaseInsensitive)&&ui->lineEdit_layeredfs_infile_titlekey->text().length()<32)
    {
        ui->lineEdit_layeredfs_infile_titlekey->setFocus();
        return;
    }*/

    QString bakdir, delmsg;
    if(ui->tabWidget->currentIndex()==2)
    {
        bakdir="/backups/"+ui->lineEdit_layeredfs_infile_name->text();
        delmsg="WARNING: "+ui->lineEdit_layeredfs_infile_file->text()+" already exists.\nDelete it and continue?";
    }
    else
    {
        bakdir="/atmosphere/titles/"+ui->lineEdit_layeredfs_target_titleid->text();
        delmsg="WARNING: The selected target is currently in use.\nDelete the current title and continue?\n\n"+getCustomTitleByID(ui->lineEdit_layeredfs_target_titleid->text(), ui->lineEdit_layeredfs_path->text()+"/atmosphere/titles/")+" → "+ui->lineEdit_layeredfs_infile_name->text();
    }

    if(QFile::exists(ui->lineEdit_layeredfs_path->text()+bakdir))
    {
        if(QMessageBox::question(this, "Title Switch", delmsg, QMessageBox::Yes, QMessageBox::No)==QMessageBox::Yes)
        {
            QDir del(ui->lineEdit_layeredfs_path->text()+bakdir);
            del.removeRecursively();
            fillTable();
        }
        else return;
    }

    QStorageInfo tmpstorage(QDir::tempPath());
    tempfolder=QDir::tempPath();
    QStorageInfo destorage(ui->lineEdit_layeredfs_path->text());
    QFile infile(titledatafile=ui->lineEdit_layeredfs_infile_file->text());
    qint64 spcreqondst=infile.size();

    if(ui->lineEdit_layeredfs_infile_file->text().endsWith(".nca", Qt::CaseInsensitive)==false)
    {
        QFile writetest(tempfolder+"/title_switch.tmp");
        writetest.open(QIODevice::ReadWrite);
        if(!(writetest.resize(0) && tmpstorage.bytesAvailable()>=spcreqondst))
        {
            tempfolder=ui->lineEdit_layeredfs_path->text();
            spcreqondst=spcreqondst*2;
        }
        else if(destorage.device()==tmpstorage.device())
            spcreqondst=spcreqondst*2;
        titledatafile=tempfolder+"/title_switch_temp.nca";
    }

    if(destorage.bytesAvailable()<spcreqondst)
    {
        QMessageBox::critical(this, "Title Switch", "At least "+QString::number(spcreqondst/1000000)+" MB are required for this title.\nSpace available on the destination drive: "+QString::number(destorage.bytesAvailable()/1000000)+" MB.");
        return;
    }

    hourglass(true);
    if(ui->lineEdit_layeredfs_infile_file->text().endsWith(".nca", Qt::CaseInsensitive)==false)
    {
        swapHourglass(1);
        XCIConverter* xciconverter=new XCIConverter;
        xciconverter->setIn(ui->lineEdit_layeredfs_infile_file->text());
        xciconverter->setOut(titledatafile);
        if(ui->tabWidget->currentIndex()==2)
            xciconverter->setNCAout(ui->lineEdit_layeredfs_path->text()+"/backups/"+ui->lineEdit_layeredfs_infile_name->text());
        else
            xciconverter->setNCAout(ui->lineEdit_layeredfs_path->text()+"/atmosphere/titles/"+ui->lineEdit_layeredfs_target_titleid->text());
        xciconverter->setN(ui->lineEdit_layeredfs_infile_name->text());
        xciconverter->setId(ui->lineEdit_layeredfs_target_titleid->text());
        xciconverter->setTitlekey(ui->lineEdit_layeredfs_infile_titlekey->text());
        QThread* thread=new QThread;
        xciconverter->moveToThread(thread);
        connect(thread, SIGNAL (started()), xciconverter, SLOT (process()));
        connect(xciconverter, SIGNAL (convFinished()), this, SLOT (resetHourglassAnim()));
        connect(xciconverter, SIGNAL (finished()), this, SLOT (hideHourglass()));
        connect(xciconverter, SIGNAL (finished()), this, SLOT (refreshTable()));
        connect(xciconverter, SIGNAL (finished()), thread, SLOT (quit()));
        connect(xciconverter, SIGNAL (finished()), xciconverter, SLOT (deleteLater()));
        connect(thread, SIGNAL (finished()), thread, SLOT (deleteLater()));
        thread->start();
    }
    else
    {
        if(ui->tabWidget->currentIndex()==2) swapHourglass(2);
        NCAConverter* ncaconverter=new NCAConverter;
        ncaconverter->setIn(titledatafile);
        if(ui->tabWidget->currentIndex()==2)
            ncaconverter->setOut(ui->lineEdit_layeredfs_path->text()+"/backups/"+ui->lineEdit_layeredfs_infile_name->text());
        else
            ncaconverter->setOut(ui->lineEdit_layeredfs_path->text()+"/atmosphere/titles/"+ui->lineEdit_layeredfs_target_titleid->text());
        ncaconverter->setN(ui->lineEdit_layeredfs_infile_name->text());
        ncaconverter->setId(ui->lineEdit_layeredfs_target_titleid->text());
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
}

void MainWindow::on_commandLinkButton_settings_tools_makekeystxt_clicked()
{
    QDesktopServices::openUrl(QUrl("https://gbatemp.net/threads/how-to-get-switch-keys-for-hactool-xci-decrypting.506978/"));
    toolsExist();
}

void patchNpdm(QString id, QString path)
{
    logappend("PATCHING: " + path + " ID: " + id + "\n");

    if(QFile::exists(path)==false)
    {
        logappend("FILE NOT FOUND\n");
        QMessageBox::critical(nullptr, "Error", "Could not patch main.npdm: file not found ("+path+").\nPlease check your keys.txt file and your NCA file.");
        return;
    }

    QFile mainfile(path);
    QByteArray idbytes, filebytes;
    int aci0rawoffset=0;

    mainfile.open(QIODevice::ReadWrite);
    filebytes=mainfile.readAll();
    logappend("ORIGINAL: " + filebytes.toHex() + "\n");

    while(!(filebytes.at(aci0rawoffset)=='A' && filebytes.at(aci0rawoffset+1)=='C' && filebytes.at(aci0rawoffset+2)=='I' && filebytes.at(aci0rawoffset+3)=='0') && aci0rawoffset<filebytes.length()-1)
        aci0rawoffset++;

    if(aci0rawoffset==filebytes.length()-1)
    {
        logappend("'ACI0' NOT FOUND\n");
        QMessageBox::critical(nullptr, "Error", "Could not patch main.npdm: 'ACI0' not found.\nPlease check your keys.txt file.");
        return;
    }

    logappend("FOUND ACI0 AT OFFSET " + QString::number(aci0rawoffset) + "\n");

    idbytes=QByteArray::fromHex(id.toLatin1());

    QByteArray patched;
    for(int i=0; i<aci0rawoffset+16; i++)
    {
        patched.append(filebytes.at(i));
    }

    for(int i=7; i>=0; i--)
    {
        patched.append(idbytes.at(i));
    }

    for(int i=aci0rawoffset+24; i<filebytes.length(); i++)
    {
        patched.append(filebytes.at(i));
    }

    logappend("PATCHED: " + patched.toHex() + "\n");

    if(filebytes.length()==patched.length())
        logappend("LENGTH MATCH\n");
    else
    {
        logappend("LENGTH MISMATCH\n");
        QMessageBox::critical(nullptr, "Error", "Could not patch main.npdm: length mismatch.\nPlease save the log file.");
        return;
    }

    QFile patchedfile(path);
    patchedfile.open(QIODevice::ReadWrite);
    patchedfile.resize(0);
    patchedfile.write(patched);
    patchedfile.flush();
    patchedfile.close();

    logappend("ORIGINAL MD5: " + QCryptographicHash::hash(filebytes, QCryptographicHash::Md5).toHex() + "\nPATCHED MD5: " + QCryptographicHash::hash(patched, QCryptographicHash::Md5).toHex() + "\n");
}

void MainWindow::on_commandLinkButton_settings_tools_patchMainNpdm_clicked()
{
    ui->tabWidget->setCurrentIndex(1);
    QMessageBox::information(this, "Patch main.npdm", "NOTE: Since version 1.1, Title Switch patches main.npdm automatically.", QMessageBox::Ok);
    if(QMessageBox::question(this, "Patch main.npdm", "Use this target title ID?\n"+ui->lineEdit_layeredfs_target_titleid->text())==QMessageBox::Ok)
    {
        QString mainfile=QFileDialog::getOpenFileName(this, "Patch main.npdm", QDir::homePath(), "main.npdm (main.npdm)");
        if(mainfile!=NULL)
            patchNpdm(ui->lineEdit_layeredfs_target_titleid->text(), mainfile);
    }
}

void MainWindow::on_commandLinkButton_settings_tools_saveLog_clicked()
{
    QString logpath=QFileDialog::getSaveFileName(this, "Save log file as...", QDir::homePath(), "Text file (*.txt)");
    if(logpath==NULL) return;
    if(logpath.endsWith(".txt", Qt::CaseInsensitive)==false)
        logpath.append(".txt");
    QFile logfile(logpath);
    logfile.open(QIODevice::ReadWrite);
    logfile.resize(0);
    logfile.write(logarray);
    logfile.flush();
    logfile.close();
}

void MainWindow::on_pushButton_layeredfs_setTarget_clicked()
{
    if(ui->tableWidget_layeredfs_folders->selectedItems().isEmpty())
        return;

    QString path=ui->lineEdit_layeredfs_path->text()+"/atmosphere/titles/"+ui->tableWidget_layeredfs_folders->item(ui->tableWidget_layeredfs_folders->selectedItems()[0]->row(), 0)->text();
    if(QFile::exists(ui->lineEdit_layeredfs_path->text()+"/atmosphere/titles/"+ui->lineEdit_layeredfs_target_titleid->text()))
    {
        QMessageBox::critical(this, "Change target title ID", "The target title ID you have selected is already in use.");
        return;
    }
    if(QFile::exists(path))
        if(QMessageBox::question(this, "Change target title ID", "Are you sure you want to change this custom title's target title ID?\n\n"+
                              ui->tableWidget_layeredfs_folders->item(ui->tableWidget_layeredfs_folders->selectedItems()[0]->row(), 1)->text()+" ["+ui->tableWidget_layeredfs_folders->item(ui->tableWidget_layeredfs_folders->selectedItems()[0]->row(), 0)->text()+"]"+
                        " → "+getNameByID(ui->lineEdit_layeredfs_target_titleid->text()) + " ["+ui->lineEdit_layeredfs_target_titleid->text()+"]"+
                        "\n\n"+ui->tableWidget_layeredfs_folders->item(ui->tableWidget_layeredfs_folders->selectedItems()[0]->row(), 2)->text())==QMessageBox::Yes)
        {
            hourglass(true);
            QString newpath=ui->lineEdit_layeredfs_path->text()+"/atmosphere/titles/"+ui->lineEdit_layeredfs_target_titleid->text();
            QDir dir(path);
            dir.rename(path, newpath);
            QFile namefile(path+".txt");
            namefile.rename(newpath+".txt");
            if(autoPatchMainNpdm) patchNpdm(ui->lineEdit_layeredfs_target_titleid->text(), newpath+"/exefs/main.npdm");
            fillTable();
            hourglass(false);
        }
}

void MainWindow::on_pushButton_settings_about_clicked()
{
    QMessageBox::about(this, "About Title Switch", "Title Switch 1.1 by nastys — Nintendo Switch backup manager for GNU/Linux, macOS, and Windows.\n\nLicensed under GPLv3.");
    QMessageBox::aboutQt(this, "About Qt");
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasUrls()) {
        e->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *e)
{
    QString drop=e->mimeData()->urls()[0].toLocalFile();
    if(QFile::exists(drop))
    {
        if(QFileInfo(drop).isDir())
            sd_browse(drop);
        else
            openFile(drop);
    }
}

void MainWindow::on_checkBox_layeredfs_patchingEnabled_stateChanged(int arg1)
{
    autoPatchMainNpdm=arg1;
}

void MainWindow::removeTempNCA()
{
    if((ui->lineEdit_layeredfs_infile_file->text().endsWith(".xci", Qt::CaseInsensitive)) || (ui->lineEdit_layeredfs_infile_file->text().endsWith(".nsp", Qt::CaseInsensitive)))
        QFile::remove(titledatafile);
}

void MainWindow::on_pushButton_layeredfs_explore_clicked()
{
    if(ui->lineEdit_layeredfs_path->text()=="") return;

    QString bakdir="/atmosphere/titles/";
    QTableWidget** table=&(ui->tableWidget_layeredfs_folders);

    if(ui->tabWidget->currentIndex()==2)
    {
        bakdir="/backups/";
        table=&(ui->tableWidget_plague_folders);
    }

    if((*table)->selectedItems().isEmpty())
        QDesktopServices::openUrl(QUrl("file://"+ui->lineEdit_layeredfs_path->text()+bakdir));
    else
        QDesktopServices::openUrl(QUrl("file://"+ui->lineEdit_layeredfs_path->text()+bakdir+(*table)->item((*table)->selectedItems()[0]->row(), 0)->text()));
}

void MainWindow::on_commandLinkButton_plague_compatibility_clicked()
{
    on_commandLinkButton_layeredfs_compatibility_clicked();
}

void MainWindow::on_commandLinkButton_plague_start_clicked()
{
    on_commandLinkButton_layeredfs_start_clicked();
}

void MainWindow::on_pushButton_plague_explore_clicked()
{
    on_pushButton_layeredfs_explore_clicked();
}

void MainWindow::on_pushButton_plague_delete_clicked()
{
    on_pushButton_layeredfs_delete_clicked();
}

void MainWindow::on_commandLinkButton_plague_migrate_clicked()
{
    if(ui->lineEdit_layeredfs_path->text()!="" && QMessageBox::warning(this, "Migrate from LayeredFS", "WARNING: This action cannot be undone automatically. Continue?", QMessageBox::Yes, QMessageBox::No)==QMessageBox::Yes)
    {
        QDirIterator sd_folder(ui->lineEdit_layeredfs_path->text()+"/atmosphere/titles/", QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
        bool yesToAll=false;
        for(int i=0; sd_folder.hasNext(); i++)
        {
            sd_folder.next();
            if(QFile::exists(sd_folder.filePath()+".txt"))
            {
                QString newname=getCustomTitleByID(sd_folder.fileName(), ui->lineEdit_layeredfs_path->text()+"/atmosphere/titles/");
                if(QFile::exists(ui->lineEdit_layeredfs_path->text()+"/backups/"+newname))
                {
                    if(yesToAll)
                        QFile::remove(ui->lineEdit_layeredfs_path->text()+"/backups/"+newname);
                    else
                    {
                        QMessageBox fileexistswarning;
                        fileexistswarning.addButton(QMessageBox::Yes);
                        fileexistswarning.addButton(QMessageBox::YesToAll);
                        fileexistswarning.addButton(QMessageBox::No);
                        fileexistswarning.addButton(QMessageBox::Cancel);
                        fileexistswarning.setText(newname+" already exists. Delete it and continue?");
                        fileexistswarning.setWindowTitle("Migrate from LayeredFS");

                        switch(fileexistswarning.exec())
                        {
                        case QMessageBox::YesToAll:
                            yesToAll=true;
                        case QMessageBox::Yes:
                            QDir(ui->lineEdit_layeredfs_path->text()+"/backups/"+newname).removeRecursively();
                            break;
                        case QMessageBox::Cancel:
                            return;
                        default:
                            break;
                        }
                    }
                }

                if(QFile::exists(ui->lineEdit_layeredfs_path->text()+"/backups/"+newname)==false)
                {
                    QFile::rename(sd_folder.filePath(), ui->lineEdit_layeredfs_path->text()+"/backups/"+newname);
                    QFile::remove(sd_folder.filePath()+".txt");
                }
            }
        }

        fillTable();
    }
}

void MainWindow::on_pushButton_layeredfs_rename_clicked()
{
    QString bakdir="/atmosphere/titles/", curname;
    QTableWidget** table=&(ui->tableWidget_layeredfs_folders);
    int col=2;
    if(ui->tabWidget->currentIndex()==2)
    {
        bakdir="/backups/";
        table=&(ui->tableWidget_plague_folders);
        col=0;
    }

    if((*table)->selectedItems().isEmpty())
        return;

    curname=(*table)->item((*table)->selectedItems()[0]->row(), col)->text();

    QString newname=QInputDialog::getText(this, "Rename", "New name:", QLineEdit::Normal, curname);

    if(newname==NULL) return;

    if(ui->tabWidget->currentIndex()==1)
    {
        QFile txt(ui->lineEdit_layeredfs_path->text()+bakdir+ui->tableWidget_layeredfs_folders->item(ui->tableWidget_layeredfs_folders->selectedItems()[0]->row(), 0)->text()+".txt");
        txt.open(QIODevice::ReadWrite);
        txt.resize(0);
        txt.write(newname.toLatin1());
        txt.flush();
        txt.close();
    }
    else
    {
        if(QFile::exists(ui->lineEdit_layeredfs_path->text()+bakdir+newname))
        {
            if(QMessageBox::warning(this, "Rename", newname+" already exists. Delete it and continue?", QMessageBox::Yes, QMessageBox::No)==QMessageBox::Yes)
                QDir(ui->lineEdit_layeredfs_path->text()+bakdir+newname).removeRecursively();
            else return;
        }

        QFile::rename(ui->lineEdit_layeredfs_path->text()+bakdir+curname, ui->lineEdit_layeredfs_path->text()+bakdir+newname);
    }

    fillTable();
}

void MainWindow::on_pushButton_plague_rename_clicked()
{
    on_pushButton_layeredfs_rename_clicked();
}

void MainWindow::on_pushButton_layeredfs_refresh_clicked()
{
    fillTable();
}

void MainWindow::on_pushButton_plague_refresh_clicked()
{
    on_pushButton_layeredfs_refresh_clicked();
}
