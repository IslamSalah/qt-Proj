#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QScrollArea>
#include <QLabel>
#include <QCloseEvent>
#include <QRubberBand>

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
    Ui::MainWindow *ui;
    QLabel * imageArea;
    QScrollArea * scrollArea;
    bool loadFile(const QString &);
    double scaleFactor;
    void adjustScrollBar(QScrollBar *scrollBar, double factor);
    void initArea(void);
    bool isNeedSave(void);
    bool isImageLoaded(void);
    QPoint origin, end;
    QRubberBand *rubberBand;
    QRect getSelectedRegOnImg();
    void zoomToRegion(QRect rec);
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
};

#endif // MAINWINDOW_H
