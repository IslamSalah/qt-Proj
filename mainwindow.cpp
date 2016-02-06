#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QStandardPaths"
#include "QFileDialog"
#include "QMessageBox"
#include "QScreen"
#include "QScrollBar"
#include "QImageReader"

#include <QInputDialog>
#include <QLineEdit>
#include <QDebug>

#include <iostream>
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    //ui preparation
    ui->setupUi(this);
    connectActions();

    //activate scrolls
    scrollArea = new QScrollArea();
    scrollArea->setBackgroundRole(QPalette::Dark);
    scrollArea->setWidget(ui->imageArea);
    scrollArea->setAlignment(Qt::AlignCenter);
    setCentralWidget(scrollArea);

    //resize window to proper size
    resize(QGuiApplication::primaryScreen()->availableSize() * 3 / 5);

    //
    ui->toolBar->setMovable(false);     //to avoid out of sync. rubberband
    rubberBand = new QRubberBand(QRubberBand::Rectangle, this);

}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::open(void){
    //check if we need to save this file first.
    rubberBand->hide();
    if(isImageLoaded())
        if(isNeedSave())
            checkSave();
    QString imagePath = QFileDialog::getOpenFileName(this,tr("Open File"),"",tr("JPEG (*.jpg *.jpeg);;PNG (*.png);;BMP (*.bmp)" ));
    if (imagePath.isEmpty() || !loadFile(imagePath)){
        QMessageBox msg;
        msg.setText("file not found!");
        msg.exec();
        return;
    }
    ui->imageArea->resize(ui->imageArea->pixmap()->size());
    stack1.clear();
    stack2.clear();
    snapshot();
}
void MainWindow::save(void){
    if(ui->imageArea->pixmap() == NULL){
        QMessageBox msg;
        msg.setText("no image to be saved");
        msg.exec();
        return;
    }
    rubberBand->hide();
    QString imagePath = QFileDialog::getSaveFileName(this,tr("Save File"),"",tr("JPEG (*.jpg *.jpeg);;PNG (*.png);;BMP (*.bmp)"));

    QImage imageObject = ui->imageArea->pixmap()->toImage();
    if(!imageObject.save(imagePath)){
        QMessageBox msg;
        msg.setText("Failed to save ");
        msg.exec();
    }
}

bool MainWindow::loadFile(const QString &fileName){
    QImageReader reader(fileName);
    reader.setAutoTransform(true);
    const QImage image = reader.read();
    if (image.isNull()) {
        setWindowFilePath(QString());
        ui->imageArea->setPixmap(QPixmap());
        ui->imageArea->adjustSize();
        return false;
    }
    scaleFactor = 1;
    ui->imageArea->setPixmap(QPixmap::fromImage(image));
    ui->imageArea->adjustSize();
    setWindowFilePath(fileName);
    return true;
}

void MainWindow::fitToWindow(void){
    if(!isImageLoaded())
        return;
    double sx = 1.0*ui->imageArea->height()/(this->height()-44), sy = 1.0*ui->imageArea->width()/this->width();
    sx = (sx > sy? sx: sy);
    scaleImage(1.0/sx);
    snapshot();
}

void MainWindow::normalSize(void){
    if(!isImageLoaded())
        return;
    scaleImage(1/scaleFactor);
    snapshot();
}

void MainWindow::zoomIn(void){
    if(!isImageLoaded())
        return;
    if(rubberBand->isVisible()){ // zoom to specified region
        zoomToRegion(getSelectedRegOnImg(),false);

    }
    else if(scaleFactor < 6){ // normal zoomIn
        //check if the picture is zoomed enough.
        scaleImage(1.25);
        snapshot();
    }

}

void MainWindow::zoomOut(void){
    if(!isImageLoaded())
        return;
    if(scaleFactor > 0.2){
        //check if the picture is zoomed enough.
        scaleImage(0.8);
        snapshot();
    }
}
void MainWindow::scaleImage(double scale)
{
    scaleFactor *= scale;
    ui->imageArea->resize(scaleFactor*ui->imageArea->pixmap()->size());
    adjustScrollBar(scrollArea->horizontalScrollBar(), scale);
    adjustScrollBar(scrollArea->verticalScrollBar(), scale);
    rubberBand->hide();

}

void MainWindow::snapshot(){ //collect a snapshot of current picture for later undo/redo
    stack2.clear();
    screenshot shot;
    shot.pix = *ui->imageArea->pixmap();
    shot.scale=scaleFactor;
    shot.need_rectangle=false;
    stack1.push(shot);

    if(stack1.size()>20){
        while(stack1.size()>2)
            stack2.push(stack1.pop());
        stack1.pop();
        while(!stack2.isEmpty())
            stack1.push(stack2.pop());
    }
}

void MainWindow::connectActions(void){
    QAction *action;

    //open function can be accessed through the button, the menu or pressing Ctrl+O.
    action = ui->actionOpen;
    connect(action,SIGNAL(triggered()), this,SLOT(open()));

    //save function.
    action = ui->actionSave;
    connect(action,SIGNAL(triggered()), this,SLOT(save()));

    //zoom in.
    action = ui->actionZoom_2;
    connect(action,SIGNAL(triggered()), this,SLOT(zoomOut()));

    //zoom out.
    action = ui->actionZoom_3;
    connect(action,SIGNAL(triggered()), this,SLOT(zoomIn()));

    //fitToWindow
    action = ui->actionFit_to_window;
    connect(action,SIGNAL(triggered()), this,SLOT(fitToWindow()));

    //normalSize
    action = ui->actionNormal_size;
    connect(action,SIGNAL(triggered()), this,SLOT(normalSize()));

    //crop
    action = ui->actionCrop;
    connect(action,SIGNAL(triggered()), this,SLOT(crop()));

    //rotate
    action = ui->actionRotate;
    connect(action,SIGNAL(triggered()), this,SLOT(rotate()));

    //close file
    action = ui->actionClose_file;
    connect(action,SIGNAL(triggered()), this,SLOT(closeFile()));

    //reset
    action = ui->actionReset;
    connect(action,SIGNAL(triggered()), this,SLOT(reset()));

    //undo
    action = ui->actionUndo;
    connect(action,SIGNAL(triggered()), this,SLOT(undo()));

    //redo
    action = ui->actionRedo;
    connect(action,SIGNAL(triggered()), this,SLOT(redo()));

    //exit
    action = ui->actionExit;
    connect(action,SIGNAL(triggered()), this,SLOT(exit()));
}
void MainWindow::adjustScrollBar(QScrollBar *scrollBar, double factor){
    scrollBar->setValue(int(factor * scrollBar->value()
                            + ((factor - 1) * scrollBar->pageStep()/2)));
}


void MainWindow::undo(void){
    if(stack1.size()>1){
        stack2.push(stack1.pop());
        ui->imageArea->setPixmap(stack1.top().pix);
        scaleFactor=stack1.top().scale;
        ui->imageArea->resize(scaleFactor*ui->imageArea->pixmap()->size());
        if(stack1.top().need_rectangle){
            zoomToRegion(stack1.top().rectangle,true);
        }
    }
    rubberBand->hide();
}
void MainWindow::redo(void){
    if(stack2.size()>0){
        stack1.push(stack2.pop());
        ui->imageArea->setPixmap(stack1.top().pix);
        scaleFactor=stack1.top().scale;
        ui->imageArea->resize(scaleFactor*ui->imageArea->pixmap()->size());
        if(stack1.top().need_rectangle){
            zoomToRegion(stack1.top().rectangle,true);
        }
    }
    rubberBand->hide();
}

void MainWindow::closeFile(void){
    //clear everything here
    if(!isImageLoaded())
        return;
    ui->imageArea->setPixmap(QPixmap());
    scaleImage(1/scaleFactor);
    rubberBand->hide();
    snapshot();
}

void MainWindow::reset(void){
    stack2.clear();
    while(stack1.size()>1){
       stack1.pop();
    }
    ui->imageArea->setPixmap(stack1.top().pix);
    scaleFactor=stack1.top().scale;
    ui->imageArea->resize(scaleFactor*ui->imageArea->pixmap()->size());
    if(stack1.top().need_rectangle){
        zoomToRegion(stack1.top().rectangle,true);
    }
    rubberBand->hide();
}

void MainWindow::rotate(void){
    if(!isImageLoaded()){
        QMessageBox msg;
        msg.setText("no image to be rotated");
        msg.exec();
        return;
    }
    rubberBand->hide();
    bool ok;
    QRegExp re("\\d*"); //regix all integers
    QString text = QInputDialog::getText(this, tr("Angle"), tr("Angle:"),
                                         QLineEdit::Normal,QDir::home().dirName(), &ok);
    if (ok && !text.isEmpty() && re.exactMatch(text)){
        try{
            int angle = text.toInt();
            QPixmap pixmap(*ui->imageArea->pixmap());
            QMatrix rm;
            rm.rotate(angle);
            pixmap = pixmap.transformed(rm);
            ui->imageArea->setPixmap(pixmap);
            ui->imageArea->resize(scaleFactor*ui->imageArea->pixmap()->size());
            snapshot();

        }catch(std::exception &e){
            QMessageBox msgBox;
            msgBox.setText("Please Enter a Valid Angle.");
            msgBox.exec();
        }
    }else{
        QMessageBox msgBox;
        msgBox.setText("Please Enter a Valid Angle.");
        msgBox.exec();
    }
}

void MainWindow::crop(void){
    if(!isImageLoaded()){
        QMessageBox msg;
        msg.setText("no image to be cropped");
        msg.exec();
        return;
    }
    if(rubberBand->isVisible()){
        rubberBand->hide();
        QPixmap pix = ui->imageArea->pixmap()->copy(getSelectedRegOnImg());
        ui->imageArea->setPixmap(pix);
        ui->imageArea->resize(scaleFactor*ui->imageArea->pixmap()->size());
        snapshot();
    }
}

void MainWindow::exit(void){
    if(isNeedSave()){
        checkSave();
    }
    QApplication::exit();
}

bool MainWindow::isNeedSave(void){
    return true;
}

bool MainWindow::checkSave(void){
    QMessageBox::StandardButton ret;
    ret = QMessageBox::warning(this, tr("Application"),
                 tr("The imae has been modified.\n"
                    "Do you want to save your changes?"),
                 QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (ret == QMessageBox::Save){
        save();
        return true;
    }
    else if (ret == QMessageBox::Cancel)
        return false;
    else
        return true;
}

void MainWindow::closeEvent(QCloseEvent *event){
    if(isNeedSave()){
        if(checkSave()){
            event->accept();
        }else{
            event->ignore();
        }
    }
}

bool MainWindow::isImageLoaded(void){
    return (ui->imageArea->pixmap() != NULL);
}

QRect MainWindow::getSelectedRegOnImg()
{
    QPoint toolBarPoint(0, 44);
    //do mapping of points
    QPoint a = (origin - ui->imageArea->pos()-toolBarPoint)/scaleFactor;
    QPoint b = (end - ui->imageArea->pos()-toolBarPoint)/scaleFactor;

    QPoint topLeft(std::min(a.x(), b.x()) , std::min(a.y(), b.y()));
    QPoint bottomRight(std::max(a.x(), b.x()) , std::max(a.y(), b.y()));

    QRect rect(topLeft, bottomRight);
    return rect;
}

void MainWindow::zoomToRegion(QRect rec,bool undoing)
{
    //scale to required region
    double s;
    if(rec.width() > rec.height()){
        s = 1.0*ui->imageArea->width()/this->width();           // scale first the QLabel:imageArea to fit the window
        s*= 1.0*rec.width()/ui->imageArea->pixmap()->width();   //then scale the specified region
    }else {
        s = 1.0*ui->imageArea->height()/(this->height()-10);
        s*= 1.0*rec.height()/ui->imageArea->pixmap()->height();
    }
    if(!undoing){   //if doing the actual zooming , not undo/redo
        //check scale boundries
        if(scaleFactor/s < 7.5)     // can zoom to selected region
                scaleImage(1/s);
        else                      // can't, so zoom as much as you can
            scaleImage(7.5/scaleFactor);

        //take a shot for undo/redo with true value as we need the rubberband rectangle
        snapshot();
        //setting the rectangle of the snapshot to be the current selected rectangle
        stack1.top().rectangle=rec;
        stack1.top().need_rectangle=true;
    }
    //scroll to required region
    if(rec.width() < rec.height()){
        double a = 1.0*(this->width()-rec.width()*scaleFactor)/2.0;
        rec.setX(rec.x()-a/scaleFactor);
        rec.setX(rec.x()>0?rec.x():0);      // to be non-negative
    }else{
        double a = 1.0*(this->height()-rec.height()*scaleFactor)/2.0;
        rec.setY(rec.y()-a/scaleFactor);
        rec.setY(rec.y()>0?rec.y():0);      // to be non-negative
    }
         // scroll to topleft point of QLabel:imageArea
    scrollArea->horizontalScrollBar()->setValue(scrollArea->horizontalScrollBar()->minimum());
    scrollArea->verticalScrollBar()->setValue(scrollArea->verticalScrollBar()->minimum());
        // scroll to make required place centred on small dimension
    scrollArea->horizontalScrollBar()->setValue(rec.x()*scaleFactor);
    scrollArea->verticalScrollBar()->setValue(rec.y()*scaleFactor);
}

void MainWindow::mousePressEvent(QMouseEvent *e)
{
    if(ui->imageArea->underMouse()){
        origin = e->pos();
        rubberBand->setGeometry(QRect(origin, QSize()));
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *e)
{
    rubberBand->setGeometry(QRect(origin, e->pos()).normalized());
    end = e->pos();
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *)
{
    if(rubberBand->isVisible())
        rubberBand->hide();
    else if(isImageLoaded())
        rubberBand->show();
}

void MainWindow::wheelEvent(QWheelEvent *)
{
    rubberBand->hide();
}
