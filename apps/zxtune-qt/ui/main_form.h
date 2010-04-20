/********************************************************************************
** Form generated from reading UI file 'main_form.ui'
**
** Created: Tue Apr 20 09:02:04 2010
**      by: Qt User Interface Compiler version 4.6.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef MAIN_FORM_H
#define MAIN_FORM_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHeaderView>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QToolBar>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainForm
{
public:
    QAction *actionPlay;
    QAction *actionPause;
    QAction *actionStop;
    QAction *actionPrevious;
    QAction *actionNext;
    QAction *actionPrevious_2;
    QAction *actionShowPlayback;
    QAction *actionShowSeeking;
    QWidget *centralwidget;
    QMenuBar *menubar;
    QMenu *menuFile;
    QMenu *menuPlayback;
    QMenu *menuControls;
    QToolBar *controlToolbar;
    QToolBar *seekToolbar;

    void setupUi(QMainWindow *MainForm)
    {
        if (MainForm->objectName().isEmpty())
            MainForm->setObjectName(QString::fromUtf8("MainForm"));
        MainForm->resize(640, 479);
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(MainForm->sizePolicy().hasHeightForWidth());
        MainForm->setSizePolicy(sizePolicy);
        MainForm->setWindowTitle(QString::fromUtf8("zxtune-qt"));
        actionPlay = new QAction(MainForm);
        actionPlay->setObjectName(QString::fromUtf8("actionPlay"));
        actionPause = new QAction(MainForm);
        actionPause->setObjectName(QString::fromUtf8("actionPause"));
        actionStop = new QAction(MainForm);
        actionStop->setObjectName(QString::fromUtf8("actionStop"));
        actionPrevious = new QAction(MainForm);
        actionPrevious->setObjectName(QString::fromUtf8("actionPrevious"));
        actionNext = new QAction(MainForm);
        actionNext->setObjectName(QString::fromUtf8("actionNext"));
        actionPrevious_2 = new QAction(MainForm);
        actionPrevious_2->setObjectName(QString::fromUtf8("actionPrevious_2"));
        actionShowPlayback = new QAction(MainForm);
        actionShowPlayback->setObjectName(QString::fromUtf8("actionShowPlayback"));
        actionShowPlayback->setCheckable(true);
        actionShowPlayback->setChecked(true);
        actionShowSeeking = new QAction(MainForm);
        actionShowSeeking->setObjectName(QString::fromUtf8("actionShowSeeking"));
        actionShowSeeking->setCheckable(true);
        actionShowSeeking->setChecked(true);
        centralwidget = new QWidget(MainForm);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        MainForm->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainForm);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 640, 24));
        menuFile = new QMenu(menubar);
        menuFile->setObjectName(QString::fromUtf8("menuFile"));
        menuPlayback = new QMenu(menubar);
        menuPlayback->setObjectName(QString::fromUtf8("menuPlayback"));
        menuControls = new QMenu(menubar);
        menuControls->setObjectName(QString::fromUtf8("menuControls"));
        MainForm->setMenuBar(menubar);
        controlToolbar = new QToolBar(MainForm);
        controlToolbar->setObjectName(QString::fromUtf8("controlToolbar"));
        QSizePolicy sizePolicy1(QSizePolicy::Maximum, QSizePolicy::MinimumExpanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(controlToolbar->sizePolicy().hasHeightForWidth());
        controlToolbar->setSizePolicy(sizePolicy1);
        controlToolbar->setAllowedAreas(Qt::BottomToolBarArea|Qt::TopToolBarArea);
        MainForm->addToolBar(Qt::TopToolBarArea, controlToolbar);
        seekToolbar = new QToolBar(MainForm);
        seekToolbar->setObjectName(QString::fromUtf8("seekToolbar"));
        MainForm->addToolBar(Qt::TopToolBarArea, seekToolbar);

        menubar->addAction(menuFile->menuAction());
        menubar->addAction(menuPlayback->menuAction());
        menubar->addAction(menuControls->menuAction());
        menuPlayback->addAction(actionPlay);
        menuPlayback->addAction(actionPause);
        menuPlayback->addAction(actionStop);
        menuPlayback->addSeparator();
        menuPlayback->addAction(actionNext);
        menuPlayback->addAction(actionPrevious_2);
        menuControls->addAction(actionShowPlayback);
        menuControls->addAction(actionShowSeeking);

        retranslateUi(MainForm);
        QObject::connect(actionShowPlayback, SIGNAL(toggled(bool)), controlToolbar, SLOT(setVisible(bool)));
        QObject::connect(actionShowSeeking, SIGNAL(toggled(bool)), seekToolbar, SLOT(setVisible(bool)));

        QMetaObject::connectSlotsByName(MainForm);
    } // setupUi

    void retranslateUi(QMainWindow *MainForm)
    {
        actionPlay->setText(QApplication::translate("MainForm", "Play", 0, QApplication::UnicodeUTF8));
        actionPause->setText(QApplication::translate("MainForm", "Pause", 0, QApplication::UnicodeUTF8));
        actionStop->setText(QApplication::translate("MainForm", "Stop", 0, QApplication::UnicodeUTF8));
        actionPrevious->setText(QApplication::translate("MainForm", "Previous", 0, QApplication::UnicodeUTF8));
        actionNext->setText(QApplication::translate("MainForm", "Next", 0, QApplication::UnicodeUTF8));
        actionPrevious_2->setText(QApplication::translate("MainForm", "Previous", 0, QApplication::UnicodeUTF8));
        actionShowPlayback->setText(QApplication::translate("MainForm", "Playback", 0, QApplication::UnicodeUTF8));
        actionShowSeeking->setText(QApplication::translate("MainForm", "Seeking", 0, QApplication::UnicodeUTF8));
        menuFile->setTitle(QApplication::translate("MainForm", "File", 0, QApplication::UnicodeUTF8));
        menuPlayback->setTitle(QApplication::translate("MainForm", "Playback", 0, QApplication::UnicodeUTF8));
        menuControls->setTitle(QApplication::translate("MainForm", "Controls", 0, QApplication::UnicodeUTF8));
        controlToolbar->setWindowTitle(QApplication::translate("MainForm", "toolBar", 0, QApplication::UnicodeUTF8));
        seekToolbar->setWindowTitle(QApplication::translate("MainForm", "toolBar", 0, QApplication::UnicodeUTF8));
        Q_UNUSED(MainForm);
    } // retranslateUi

};

namespace Ui {
    class MainForm: public Ui_MainForm {};
} // namespace Ui

QT_END_NAMESPACE

#endif // MAIN_FORM_H
