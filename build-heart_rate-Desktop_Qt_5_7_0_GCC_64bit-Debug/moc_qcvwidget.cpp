/****************************************************************************
** Meta object code from reading C++ file 'qcvwidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.7.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../heart_rate/qcvwidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'qcvwidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.7.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_QcvWidget_t {
    QByteArrayData data[17];
    char stringdata0[219];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_QcvWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_QcvWidget_t qt_meta_stringdata_QcvWidget = {
    {
QT_MOC_LITERAL(0, 0, 9), // "QcvWidget"
QT_MOC_LITERAL(1, 10, 16), // "sendToggleSTream"
QT_MOC_LITERAL(2, 27, 0), // ""
QT_MOC_LITERAL(3, 28, 12), // "receiveFrame"
QT_MOC_LITERAL(4, 41, 5), // "frame"
QT_MOC_LITERAL(5, 47, 11), // "receiveFace"
QT_MOC_LITERAL(6, 59, 4), // "face"
QT_MOC_LITERAL(7, 64, 26), // "receiveHeartRateEstimation"
QT_MOC_LITERAL(8, 91, 9), // "heartRate"
QT_MOC_LITERAL(9, 101, 19), // "receiveToggleStream"
QT_MOC_LITERAL(10, 121, 19), // "receiveRealTimeMode"
QT_MOC_LITERAL(11, 141, 18), // "receiveOfflineMode"
QT_MOC_LITERAL(12, 160, 14), // "faceSignalPlot"
QT_MOC_LITERAL(13, 175, 11), // "vectorFloat"
QT_MOC_LITERAL(14, 187, 9), // "sigToPlot"
QT_MOC_LITERAL(15, 197, 11), // "faceFFTPlot"
QT_MOC_LITERAL(16, 209, 9) // "FFTtoPlot"

    },
    "QcvWidget\0sendToggleSTream\0\0receiveFrame\0"
    "frame\0receiveFace\0face\0"
    "receiveHeartRateEstimation\0heartRate\0"
    "receiveToggleStream\0receiveRealTimeMode\0"
    "receiveOfflineMode\0faceSignalPlot\0"
    "vectorFloat\0sigToPlot\0faceFFTPlot\0"
    "FFTtoPlot"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_QcvWidget[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   59,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       3,    1,   60,    2, 0x08 /* Private */,
       5,    1,   63,    2, 0x08 /* Private */,
       7,    1,   66,    2, 0x08 /* Private */,
       9,    0,   69,    2, 0x08 /* Private */,
      10,    0,   70,    2, 0x08 /* Private */,
      11,    0,   71,    2, 0x08 /* Private */,
      12,    1,   72,    2, 0x08 /* Private */,
      15,    1,   75,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::QImage,    4,
    QMetaType::Void, QMetaType::QImage,    6,
    QMetaType::Void, QMetaType::Float,    8,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 13,   14,
    QMetaType::Void, 0x80000000 | 13,   16,

       0        // eod
};

void QcvWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        QcvWidget *_t = static_cast<QcvWidget *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->sendToggleSTream(); break;
        case 1: _t->receiveFrame((*reinterpret_cast< const QImage(*)>(_a[1]))); break;
        case 2: _t->receiveFace((*reinterpret_cast< const QImage(*)>(_a[1]))); break;
        case 3: _t->receiveHeartRateEstimation((*reinterpret_cast< const float(*)>(_a[1]))); break;
        case 4: _t->receiveToggleStream(); break;
        case 5: _t->receiveRealTimeMode(); break;
        case 6: _t->receiveOfflineMode(); break;
        case 7: _t->faceSignalPlot((*reinterpret_cast< const vectorFloat(*)>(_a[1]))); break;
        case 8: _t->faceFFTPlot((*reinterpret_cast< const vectorFloat(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (QcvWidget::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&QcvWidget::sendToggleSTream)) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject QcvWidget::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_QcvWidget.data,
      qt_meta_data_QcvWidget,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *QcvWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *QcvWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_QcvWidget.stringdata0))
        return static_cast<void*>(const_cast< QcvWidget*>(this));
    return QWidget::qt_metacast(_clname);
}

int QcvWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 9;
    }
    return _id;
}

// SIGNAL 0
void QcvWidget::sendToggleSTream()
{
    QMetaObject::activate(this, &staticMetaObject, 0, Q_NULLPTR);
}
QT_END_MOC_NAMESPACE
