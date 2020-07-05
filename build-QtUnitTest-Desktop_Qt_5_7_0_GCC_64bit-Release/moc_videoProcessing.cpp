/****************************************************************************
** Meta object code from reading C++ file 'videoProcessing.hpp'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.7.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../heart_rate/videoProcessing.hpp"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'videoProcessing.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.7.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_IvideoProcessing_t {
    QByteArrayData data[15];
    char stringdata0[210];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_IvideoProcessing_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_IvideoProcessing_t qt_meta_stringdata_IvideoProcessing = {
    {
QT_MOC_LITERAL(0, 0, 16), // "IvideoProcessing"
QT_MOC_LITERAL(1, 17, 9), // "sendFrame"
QT_MOC_LITERAL(2, 27, 0), // ""
QT_MOC_LITERAL(3, 28, 14), // "frameProcessed"
QT_MOC_LITERAL(4, 43, 8), // "sendFace"
QT_MOC_LITERAL(5, 52, 13), // "faceProcessed"
QT_MOC_LITERAL(6, 66, 23), // "sendHeartRateEstimation"
QT_MOC_LITERAL(7, 90, 19), // "heartRateEstimation"
QT_MOC_LITERAL(8, 110, 16), // "sendSignalToPlot"
QT_MOC_LITERAL(9, 127, 11), // "vectorFloat"
QT_MOC_LITERAL(10, 139, 9), // "sigToPlot"
QT_MOC_LITERAL(11, 149, 13), // "sendFFTtoPlot"
QT_MOC_LITERAL(12, 163, 9), // "FFTtoPlot"
QT_MOC_LITERAL(13, 173, 16), // "receiveGrabFrame"
QT_MOC_LITERAL(14, 190, 19) // "receiveToggleStream"

    },
    "IvideoProcessing\0sendFrame\0\0frameProcessed\0"
    "sendFace\0faceProcessed\0sendHeartRateEstimation\0"
    "heartRateEstimation\0sendSignalToPlot\0"
    "vectorFloat\0sigToPlot\0sendFFTtoPlot\0"
    "FFTtoPlot\0receiveGrabFrame\0"
    "receiveToggleStream"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_IvideoProcessing[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   49,    2, 0x06 /* Public */,
       4,    1,   52,    2, 0x06 /* Public */,
       6,    1,   55,    2, 0x06 /* Public */,
       8,    1,   58,    2, 0x06 /* Public */,
      11,    1,   61,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      13,    0,   64,    2, 0x0a /* Public */,
      14,    0,   65,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QImage,    3,
    QMetaType::Void, QMetaType::QImage,    5,
    QMetaType::Void, QMetaType::Float,    7,
    QMetaType::Void, 0x80000000 | 9,   10,
    QMetaType::Void, 0x80000000 | 9,   12,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void IvideoProcessing::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        IvideoProcessing *_t = static_cast<IvideoProcessing *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->sendFrame((*reinterpret_cast< const QImage(*)>(_a[1]))); break;
        case 1: _t->sendFace((*reinterpret_cast< const QImage(*)>(_a[1]))); break;
        case 2: _t->sendHeartRateEstimation((*reinterpret_cast< const float(*)>(_a[1]))); break;
        case 3: _t->sendSignalToPlot((*reinterpret_cast< const vectorFloat(*)>(_a[1]))); break;
        case 4: _t->sendFFTtoPlot((*reinterpret_cast< const vectorFloat(*)>(_a[1]))); break;
        case 5: _t->receiveGrabFrame(); break;
        case 6: _t->receiveToggleStream(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (IvideoProcessing::*_t)(const QImage & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&IvideoProcessing::sendFrame)) {
                *result = 0;
                return;
            }
        }
        {
            typedef void (IvideoProcessing::*_t)(const QImage & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&IvideoProcessing::sendFace)) {
                *result = 1;
                return;
            }
        }
        {
            typedef void (IvideoProcessing::*_t)(const float & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&IvideoProcessing::sendHeartRateEstimation)) {
                *result = 2;
                return;
            }
        }
        {
            typedef void (IvideoProcessing::*_t)(const vectorFloat );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&IvideoProcessing::sendSignalToPlot)) {
                *result = 3;
                return;
            }
        }
        {
            typedef void (IvideoProcessing::*_t)(const vectorFloat );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&IvideoProcessing::sendFFTtoPlot)) {
                *result = 4;
                return;
            }
        }
    }
}

const QMetaObject IvideoProcessing::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_IvideoProcessing.data,
      qt_meta_data_IvideoProcessing,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *IvideoProcessing::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *IvideoProcessing::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_IvideoProcessing.stringdata0))
        return static_cast<void*>(const_cast< IvideoProcessing*>(this));
    return QObject::qt_metacast(_clname);
}

int IvideoProcessing::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void IvideoProcessing::sendFrame(const QImage & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void IvideoProcessing::sendFace(const QImage & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void IvideoProcessing::sendHeartRateEstimation(const float & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void IvideoProcessing::sendSignalToPlot(const vectorFloat _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void IvideoProcessing::sendFFTtoPlot(const vectorFloat _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}
QT_END_MOC_NAMESPACE
