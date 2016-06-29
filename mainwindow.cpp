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
#include <QVBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDebug>
#include <QPainter>
#include <QtMath>
#include <QString>

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

    this->setWindowTitle(tr("Image Viewer"));


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
            if(!checkSave())
                return;
    rotation = 0;
    QString imagePath = QFileDialog::getOpenFileName(this,tr("Open File"),"",tr("all(*.jpg *.jpeg *.png *bmp);;JPEG (*.jpg *.jpeg);;PNG (*.png);;BMP (*.bmp)" ));
    if(imagePath.isEmpty()){
        return;
    }else if (!loadFile(imagePath)){
        QMessageBox msg;
        msg.setText("file not found!");
        msg.exec();
        return;
    }
    ui->imageArea->resize(ui->imageArea->pixmap()->size());
    ui->imageArea->setFrameStyle(QFrame::Box);

    orgImage = new QPixmap(*ui->imageArea->pixmap());
    rotateOrgImage = new QPixmap(* orgImage);
    stack1.clear();
    stack2.clear();
}

void MainWindow::save(void){
    if(ui->imageArea->pixmap() == NULL){
        QMessageBox msg;
        msg.setText("no image to be saved");
        msg.exec();
        return;
    }
    enterFunction();
    rubberBand->hide();
    QString imagePath = QFileDialog::getSaveFileName(this,tr("Save File"),"",tr("JPEG (*.jpg *.jpeg);;PNG (*.png);;BMP (*.bmp)"));

    QImage imageObject = ui->imageArea->pixmap()->toImage();
    if(!imageObject.save(imagePath)){
        QMessageBox msg;
        msg.setText("Failed to save ");
        msg.exec();
    }else{
        isSaved = true;
    }
    exitFunction();
}

bool MainWindow::loadFile(const QString &fileName){
    enterFunction();
    QImageReader reader(fileName);
    reader.setAutoTransform(true);
    const QImage image = reader.read();
    if (image.isNull()) {
        setWindowFilePath(QString());
        ui->imageArea->setPixmap(QPixmap());
        ui->imageArea->adjustSize();
        exitFunction();
        return false;
    }
    scaleFactor = 1;
    ui->imageArea->setPixmap(QPixmap::fromImage(image));
    ui->imageArea->adjustSize();
    setWindowFilePath(fileName);
    exitFunction();
    return true;
}

void MainWindow::fitToWindow(void){
    enterFunction();
    if(!isImageLoaded())
        return;
    double sx = 1.0*ui->imageArea->height()/(this->height()-44), sy = 1.0*ui->imageArea->width()/this->width();
    sx = (sx > sy? sx: sy);
    scaleImage(1.0/sx);
    if(!undoing){
        QRect rec;
        snapshot("fit to window", rec, 0);
    }
    exitFunction();
}

void MainWindow::normalSize(void){
    if(!isImageLoaded())
        return;
    enterFunction();
    scaleImage(1/scaleFactor);
    if(!undoing){
        QRect rec;
        snapshot("normal size", rec, 0);
    }
    exitFunction();
}

void MainWindow::zoomIn(void){
    if(!isImageLoaded())
        return;
    int width = ui->imageArea->width();
    int height = ui->imageArea->height();


    if(rubberBand->isVisible()){ // zoom to specified region
        zoomToRegion(getSelectedRegOnImg());

    }
    else if(width*height*ZOOM_FACTOR*ZOOM_FACTOR < MAX_IMG_AREA){ // normal zoomIn
        //check if the picture is zoomed enough.
        scaleImage(ZOOM_FACTOR);
        if(!undoing){
            QRect rec;
            snapshot("zoom in", rec, 0);
        }
    }

}

void MainWindow::zoomOut(void){
    if(!isImageLoaded())
        return;
    int width = ui->imageArea->width();
    int height = ui->imageArea->height();

    if(width*height > MIN_IMG_AREA){
        //check if the picture is zoomed enough.
        scaleImage(1/ZOOM_FACTOR);
        if(!undoing){
            QRect rec;
            snapshot("zoom out", rec, 0);
        }
    }
}
void MainWindow::scaleImage(double scale)
{
    enterFunction();

    scaleFactor *= scale;
    ui->imageArea->resize(scaleFactor*ui->imageArea->pixmap()->size());
    adjustScrollBar(scrollArea->horizontalScrollBar(), scale);
    adjustScrollBar(scrollArea->verticalScrollBar(), scale);
    rubberBand->hide();

    exitFunction();
}

void MainWindow::snapshot(QString operation, QRect rectangle, double angle){ //collect a snapshot of current picture for later undo/redo
    stack2.clear();
    screenshot shot;
    shot.operation = operation;
    shot.rectangle = rectangle;
    shot.angle = angle;
    stack1.push(shot);

    //new things has been done to image, it needs to be saved
    isSaved = false;
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
    QRect rec;
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

void MainWindow::doStack1(){
    while(stack1.size()>0){
        temp.push(stack1.pop());
    }

    while(temp.size()>0){
        if(QString::compare(temp.top().operation, "fit to window", Qt::CaseInsensitive)==0){
            fitToWindow();
        }
        else if(QString::compare(temp.top().operation, "normal size", Qt::CaseInsensitive)==0){
            normalSize();
        }
        else if(QString::compare(temp.top().operation, "zoom in", Qt::CaseInsensitive)==0){
            zoomIn();
        }
        else if(QString::compare(temp.top().operation, "zoom out", Qt::CaseInsensitive)==0){
            zoomOut();
        }
        else if(QString::compare(temp.top().operation, "close file", Qt::CaseInsensitive)==0){
            closeFile();
        }
        else if(QString::compare(temp.top().operation, "rotate", Qt::CaseInsensitive)==0){
            ang = temp.top().angle;
            rotate();
        }
        else if(QString::compare(temp.top().operation, "crop", Qt::CaseInsensitive)==0){
            rec = temp.top().rectangle;
            crop();
        }
        else if(QString::compare(temp.top().operation, "zoom to region", Qt::CaseInsensitive)==0){
            zoomToRegion(temp.top().rectangle);
        }
        else if(QString::compare(temp.top().operation, "resize", Qt::CaseInsensitive)==0){
            on_actionAdjust_size_triggered();
        }

        stack1.push(temp.pop());
    }
}

void MainWindow::undo(void){
    ui->imageArea->setVisible(false);
    enterFunction();
    if(stack1.size()>0){
        stack2.push(stack1.pop());
    }
    undoing=true;
    // put orgimage to screen
    ui->imageArea->setPixmap(*orgImage);
    scaleFactor=1;
    ui->imageArea->resize(scaleFactor*ui->imageArea->pixmap()->size());
    rotation =0;
    //and do all operations on stack1
    doStack1();
    //
    rubberBand->hide();
    undoing=false;
    exitFunction();
    ui->imageArea->setVisible(true);
}
void MainWindow::redo(void){
    ui->imageArea->setVisible(false);
    enterFunction();
    if(stack2.size()>0){
        stack1.push(stack2.pop());

        undoing=true;
        // put orgimage to screen
        ui->imageArea->setPixmap(*orgImage);
        scaleFactor=1;
        ui->imageArea->resize(scaleFactor*ui->imageArea->pixmap()->size());
        rotation =0;
        //and do all operations on stack1
        doStack1();
        //
        rubberBand->hide();
        undoing=false;
    }
    
    rubberBand->hide();
    exitFunction();
    ui->imageArea->setVisible(true);
}

void MainWindow::closeFile(void){
    //clear everything here
    if(isNeedSave()){
        if(!checkSave())
            return;
    }
    enterFunction();
    rotateOrgImage = new QPixmap();
    ui->imageArea->setPixmap(QPixmap());
    ui->imageArea->setFrameStyle(QFrame::NoFrame); //remove frame
    scaleImage(1/scaleFactor);
    rubberBand->hide();
    if(!undoing){
        QRect rec;
        snapshot("close file", rec, 0);
    }
    exitFunction();
}

void MainWindow::reset(void){
    enterFunction();
    stack2.clear();
    stack1.clear();
    ui->imageArea->setPixmap(*orgImage);
    rotateOrgImage = new QPixmap(*orgImage);
    scaleFactor=1;
    ui->imageArea->resize(scaleFactor*ui->imageArea->pixmap()->size());
    rotation =0;
    rubberBand->hide();
    exitFunction();
}

bool valid_input(QString s){
    QRegExp regex("\\d*"); //regex all integers
    return !s.isEmpty() && regex.exactMatch(s);
}

void MainWindow::rotate(void){
    if(!isImageLoaded()){
        QMessageBox msg;
        msg.setText("no image to be rotated");
        msg.exec();
        return;
    }
    enterFunction();
    rubberBand->hide();
    bool ok= false;
    double text;
    if(!undoing)
        text = QInputDialog::getDouble(this, tr("Angle"), tr("Angle in degree"),30,-360,360,2, &ok);

    if (ok || undoing){
        try{
            double angle;
            if(!undoing)
                angle=text;
            else
                angle=ang;

            rotation += angle;
//            rotation =rotation - 360/(int)rotation * (int) rotation; //mod like op
            rotation = rotation - (int)rotation/360 * 360; //mod like op
            QPixmap pixmap(*rotateOrgImage);
            QMatrix rm;
            rm.rotate(rotation);
            pixmap = pixmap.transformed(rm);
            ui->imageArea->setPixmap(pixmap);
            ui->imageArea->resize(scaleFactor*ui->imageArea->pixmap()->size());
            if(!undoing){
                QRect rec;
                snapshot("rotate", rec, angle);
            }
        }catch(std::exception &e){
            QMessageBox msgBox;
            msgBox.setText("Please Enter a Valid Angle.");
            msgBox.exec();
        }
    }else if (!ok){
        //do nothing
    }
    exitFunction();
}

void MainWindow::crop(void){
    if(!isImageLoaded()){
        QMessageBox msg;
        msg.setText("no image to be cropped");
        msg.exec();
        return;
    }
    if(rubberBand->isVisible() || undoing){
        enterFunction();
        rubberBand->hide();
        QRect currRect ;
        if(!undoing)
            currRect = getSelectedRegOnImg();
        else
            currRect = rec;
        QPixmap pix = ui->imageArea->pixmap()->copy(currRect);
        ui->imageArea->setPixmap(pix);
        ui->imageArea->resize(scaleFactor*ui->imageArea->pixmap()->size());
        rotateOrgImage = new QPixmap(* ui->imageArea->pixmap());
        rotation = 0.0;
        if(!undoing)
            snapshot("crop", currRect, 0);

        exitFunction();
    }
}

void MainWindow::exit(void){
    if(isNeedSave()){
        if(!checkSave())
            return;
    }
    QApplication::exit();
}

bool MainWindow::isNeedSave(void){
    return stack1.size()>1 && isImageLoaded() && !isSaved;
}

bool MainWindow::checkSave(void){
    QMessageBox::StandardButton ret;
    ret = QMessageBox::warning(this, tr("Application"),
                 tr("The image has been modified.\n"
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
    return (ui->imageArea->pixmap() != NULL && !ui->imageArea->pixmap()->isNull());
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

void MainWindow::zoomToRegion(QRect rec)
{
    enterFunction();
    //scale to required region
    double s;
    if(rec.width() > rec.height()){
        s = 1.0*ui->imageArea->width()/this->width();           // scale first the QLabel:imageArea to fit the window
        s*= 1.0*rec.width()/ui->imageArea->pixmap()->width();   //then scale the specified region
    }else {
        s = 1.0*ui->imageArea->height()/(this->height()-10);
        s*= 1.0*rec.height()/ui->imageArea->pixmap()->height();
    }
    //check scale boundriesint width = ui->imageArea->width();
    int width = ui->imageArea->width();
    int height = ui->imageArea->height();

    if(width*height/(s*s) < MAX_IMG_AREA)     // can zoom to selected region
        scaleImage(1/s);
    else                      // can't, so zoom as much as you can
        scaleImage(sqrt(MAX_IMG_AREA/(width*height)));

    if(!undoing){
        snapshot("zoom to region", rec, 0);
    }
    //scroll to required region
    centeredRect(&rec);
         // scroll to topleft point of QLabel:imageArea
    scrollArea->horizontalScrollBar()->setValue(scrollArea->horizontalScrollBar()->minimum());
    scrollArea->verticalScrollBar()->setValue(scrollArea->verticalScrollBar()->minimum());
        // scroll to make required place centred on small dimension
    scrollArea->horizontalScrollBar()->setValue(rec.x()*scaleFactor);
    scrollArea->verticalScrollBar()->setValue(rec.y()*scaleFactor);
    exitFunction();
}

void MainWindow::centeredRect(QRect *rec)
{
    if(rec->width() < rec->height()){
        double a = 1.0*(this->width()-rec->width()*scaleFactor)/2.0;
        rec->setX(rec->x()-a/scaleFactor);
        rec->setX(rec->x()>0?rec->x():0);      // to be non-negative
    }else{
        double a = 1.0*(this->height()-rec->height()*scaleFactor)/2.0;
        rec->setY(rec->y()-a/scaleFactor);
        rec->setY(rec->y()>0?rec->y():0);      // to be non-negative
    }
}

void MainWindow::mousePressEvent(QMouseEvent *e)
{
    if(ui->imageArea->underMouse()){
        origin = e->pos();
        rubberBand->setGeometry(QRect(origin, QSize()));
    }
}


QPoint MainWindow::getInscribedPoint(QPoint current_point){
    int x1 = ui->imageArea->pos().x();
    int y1 = ui->imageArea->pos().y()+44;

    int x2 = x1 + ui->imageArea->width();
    int y2 = y1 + ui->imageArea->height();

    int x,y;

    if(current_point.x() < x1)
        x = x1;
    else if (current_point.x() > x2)
        x = x2;
    else
        x = current_point.x();

    if(current_point.y() < y1)
        y = y1;
    else if (current_point.y() > y2)
        y = y2;
    else
        y = current_point.y();

    return QPoint(x, y);
}

void MainWindow::mouseMoveEvent(QMouseEvent *e)
{
    QPoint point = getInscribedPoint(e->pos());

    rubberBand->setGeometry(QRect(origin, point).normalized());
    end = point;
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

bool MainWindow::readDimentions(int *width, int *height, int *unit_type, bool *isProp)
{
    QDialog *d = new QDialog();
    QVBoxLayout *vbox = new QVBoxLayout();

    QComboBox *comboBox = new QComboBox();
    comboBox->addItems(QStringList() << "pixels" << "percentage");

    QLabel *label_width = new QLabel("Width: ");
    QLineEdit *edit_width = new QLineEdit(QString::number(ui->imageArea->pixmap()->width()));

    QLabel *label_height = new QLabel("Height: ");
    QLineEdit *edit_height = new QLineEdit(QString::number(ui->imageArea->pixmap()->height()));


    QCheckBox *check_box = new QCheckBox("Scale proportionally");
    check_box->setChecked(true);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                                        | QDialogButtonBox::Cancel);
    vbox->addWidget(comboBox);
    vbox->addWidget(label_width);
    vbox->addWidget(edit_width);
    vbox->addWidget(label_height);
    vbox->addWidget(edit_height);
    vbox->addWidget(check_box);
    vbox->addWidget(buttonBox);

    d->setLayout(vbox);

    connect(buttonBox, SIGNAL(accepted()), d, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), d, SLOT(reject()));

    connect(comboBox, SIGNAL(currentIndexChanged(int)), edit_width, SLOT(clear()));
    connect(comboBox, SIGNAL(currentIndexChanged(int)), edit_height, SLOT(clear()));

    int result = d->exec();
    if(result == QDialog::Accepted)
    {
        if(valid_input(edit_width->text()) && valid_input(edit_height->text())){
            *unit_type = comboBox->currentIndex();
            *width = edit_width->text().toInt();
            *height = edit_height->text().toInt();
            *isProp = check_box->isChecked();
            return true;
        } else {
            QMessageBox msgBox;
            msgBox.setText("Please enter valid dimensions!");
            msgBox.exec();
            return false;
        }
    }
    return false;
}

void MainWindow::on_actionAdjust_size_triggered()
{
    if(!isImageLoaded()){
        QMessageBox msg;
        msg.setText("no image is loaded!");
        msg.exec();
        return;
    }
    rubberBand->hide();

    int width, height, unit_type;
    bool isProp;

    if(undoing){
        width = temp.top().width;
        height = temp.top().height;
        unit_type = temp.top().unit_type;
        isProp = temp.top().isProp;
    }
    else if(!readDimentions(&width, &height, &unit_type, &isProp))
        return;

    int valid_size[] = {(int)qSqrt(MAX_IMG_AREA), 100};
    if(std::max(width, height) > valid_size[unit_type]){
        QMessageBox msgBox;
        msgBox.setText("Input can't exceed "+QString::number(valid_size[unit_type])+"!");
        msgBox.exec();
        return;
    }

    if(unit_type == 1){     //percentage
        width = ui->imageArea->pixmap()->width() * width / 100;
        height = ui->imageArea->pixmap()->height() * height / 100;
    }

    QPixmap pix = ui->imageArea->pixmap()->scaled(width, height, isProp? Qt::KeepAspectRatio : Qt::IgnoreAspectRatio);
    ui->imageArea->setPixmap(pix);
    scaleImage(1);
    QRect rec;
    if(!undoing){
        snapshot("resize", rec, 0);
        stack1.top().width=width;
        stack1.top().height=height;
        stack1.top().unit_type=unit_type;
        stack1.top().isProp=isProp;
    }
    rotateOrgImage = new QPixmap(* ui->imageArea->pixmap());
    rotation = 0;
}

void MainWindow::enterFunction(){
//    this->setWindowTitle(tr("loading"));
    QApplication::processEvents();
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QApplication::processEvents();
//    this->setCursor(Qt::BusyCursor);
}

void MainWindow::exitFunction(){
//    this->setWindowTitle(tr("Image Viewer"));
    QApplication::setOverrideCursor(Qt::ArrowCursor);
    QApplication::processEvents();
//    this->setCursor(Qt::ArrowCursor);
}
