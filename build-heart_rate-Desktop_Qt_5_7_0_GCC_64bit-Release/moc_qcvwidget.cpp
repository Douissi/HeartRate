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
    QByteArrayData data[8];
    char stringdata0[104];
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
QT_MOC_LITERAL(5, 47, 26), // "receiveHeartRateEstimation"
QT_MOC_LITERAL(6, 74, 9), // "heartRate"
QT_MOC_LITERAL(7, 84, 19) // "receiveToggleStream"

    },
    "QcvWidget\0sendToggleSTream\0\0receiveFrame\0"
    "frame\0receiveHeartRateEstimation\0"
    "heartRate\0receiveToggleStream"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_QcvWidget[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   34,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       3,    1,   35,    2, 0x08 /* Private */,
       5,    1,   38,    2, 0x08 /* Private */,
       7,    0,   41,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::QImage,    4,
    QMetaType::Void, QMetaType::Float,    6,
    QMetaType::Void,

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
        case 2: _t->receiveHeartRateEstimation((*reinterpret_cast< const float(*)>(_a[1]))); break;
        case 3: _t->receiveToggleStream(); break;
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
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void QcvWidget::sendToggleSTream()
{
    QMetaObject::activate(this, &staticMetaObject, 0, Q_NULLPTR);
}
QT_END_MOC_NAMESPACE
