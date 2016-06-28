#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QScrollArea>
#include <QLabel>
#include <QCloseEvent>
#include <QRubberBand>
#include <QLineEdit>
#include <QStack>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *);
private:
    QPixmap * orgImage;
    Ui::MainWindow *ui;
    QLabel * imageArea;
    QScrollArea * scrollArea;
    bool loadFile(const QString &);
    double scaleFactor;
    double rotation = 0.0;
    void adjustScrollBar(QScrollBar *scrollBar, double factor);
    void initArea(void);
    bool isNeedSave(void);
    bool isImageLoaded(void);
    QPoint origin, end;
    QRubberBand *rubberBand;
    QRect getSelectedRegOnImg();
    void zoomToRegion(QRect rec,bool undoing);
    void centeredRect(QRect *rec);
    void snapshot();
    bool readDimentions(int *, int *, int *, bool *);
    struct screenshot{
        QPixmap pix;
        QRect rectangle;
        bool need_rectangle;
        double scale;
    };
    QStack<screenshot> stack1,stack2;
    const int MAX_IMG_AREA = 100000000;
    const int MIN_IMG_AREA = 10;
    const double ZOOM_FACTOR = 1.25;
    bool isSaved = false;
    QPoint getInscribedPoint(QPoint );

public slots:
    void open(void);
    void save(void);
    void zoomIn(void);
    void zoomOut(void);
    void scaleImage(double scale);
    void fitToWindow(void);
    void normalSize(void);
    void connectActions(void);
    void crop(void);
    void rotate(void);
    void closeFile(void);
    void reset(void);
    void undo(void);
    void redo(void);
    void exit(void);
    bool checkSave(void);
protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseDoubleClickEvent(QMouseEvent *);
    void wheelEvent(QWheelEvent *);
private slots:
    void on_actionAdjust_size_triggered();
};

#endif // MAINWINDOW_H
