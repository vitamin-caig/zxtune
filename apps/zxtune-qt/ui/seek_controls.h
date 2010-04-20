/********************************************************************************
** Form generated from reading UI file 'seek_controls.ui'
**
** Created: Tue Apr 20 09:02:04 2010
**      by: Qt User Interface Compiler version 4.6.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef SEEK_CONTROLS_H
#define SEEK_CONTROLS_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QSlider>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_SeekControls
{
public:
    QHBoxLayout *horizontalLayout;
    QLabel *timeDisplay;
    QSlider *timePosition;

    void setupUi(QWidget *SeekControls)
    {
        if (SeekControls->objectName().isEmpty())
            SeekControls->setObjectName(QString::fromUtf8("SeekControls"));
        SeekControls->resize(167, 43);
        QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(SeekControls->sizePolicy().hasHeightForWidth());
        SeekControls->setSizePolicy(sizePolicy);
        horizontalLayout = new QHBoxLayout(SeekControls);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        timeDisplay = new QLabel(SeekControls);
        timeDisplay->setObjectName(QString::fromUtf8("timeDisplay"));
        QSizePolicy sizePolicy1(QSizePolicy::Minimum, QSizePolicy::Minimum);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(timeDisplay->sizePolicy().hasHeightForWidth());
        timeDisplay->setSizePolicy(sizePolicy1);
        timeDisplay->setText(QString::fromUtf8("0:0:0"));

        horizontalLayout->addWidget(timeDisplay);

        timePosition = new QSlider(SeekControls);
        timePosition->setObjectName(QString::fromUtf8("timePosition"));
        sizePolicy.setHeightForWidth(timePosition->sizePolicy().hasHeightForWidth());
        timePosition->setSizePolicy(sizePolicy);
        timePosition->setOrientation(Qt::Horizontal);

        horizontalLayout->addWidget(timePosition);


        retranslateUi(SeekControls);

        QMetaObject::connectSlotsByName(SeekControls);
    } // setupUi

    void retranslateUi(QWidget *SeekControls)
    {
        SeekControls->setWindowTitle(QApplication::translate("SeekControls", "Form", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class SeekControls: public Ui_SeekControls {};
} // namespace Ui

QT_END_NAMESPACE

#endif // SEEK_CONTROLS_H
