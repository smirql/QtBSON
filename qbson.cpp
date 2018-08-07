#include <QDebug>
#include "qbson.h"
//#include "QMongoDriver.h"

#include <QUuid>
#include <QDataStream>

#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/stdx/string_view.hpp>
#include <bsoncxx/types/value.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/exception/exception.hpp>


namespace BSON {
namespace _private {

template <typename T>
const T& refVariantValue(const QVariant & variant, bool &ok) {
    static T def;

    if (variant.userType() == qMetaTypeId<T>())
            return *reinterpret_cast<const T*>(variant.constData());

    ok = false;

    return def;
}

bsoncxx::types::value toCustomBSONBinary(const QVariant &v, QList<QByteArray> & data_lst) {
    using namespace bsoncxx::types;
    using bsoncxx::binary_sub_type;
    b_binary bin;
    bin.sub_type = binary_sub_type::k_user;

    {
        QByteArray data;

        {
            QDataStream stream(&data, QIODevice::WriteOnly);
            stream.setVersion(QDataStream::Qt_5_6);
            stream << v;
        }

        data_lst << data;
    }

    const QByteArray & data = data_lst.last();

    bin.size = data.size();
    bin.bytes = (uint8_t*) data.constData();

    return value(bin);
}

QVariant fromCustomBSONBinary(const bsoncxx::types::b_binary & binary) {
    QVariant res;
    QByteArray data((const char*) binary.bytes, binary.size);
    {
        QDataStream stream(data);
        stream.setVersion(QDataStream::Qt_5_6);
        stream >> res;
    }
    return res;
}

bsoncxx::types::value toBsonValue(const QVariant &v,
                                  QList<QByteArray> & data_lst,
                                  QList<bsoncxx::document::value> & b_docs,
                                  QList<bsoncxx::array::value> & b_arrays) {
    using namespace bsoncxx;
    using namespace bsoncxx::types;
    using bsoncxx::binary_sub_type;
    using namespace std::chrono;

    int type = v.type();

    switch(type) {
    case QVariant::Int:
        return value(b_int32{v.toInt()});
    case QVariant::String: {
        bool f = true;
        const QString & data = refVariantValue<QString>(v, f);
        if (!f)
            throw BSONexception(QString("Error in %1")
                                .arg(v.typeName()));
        {
            QByteArray utf8 = data.toUtf8();
            data_lst << utf8;
        }
        const QByteArray & utf8 = data_lst.last();
        return value(b_utf8{
                         stdx::string_view(utf8.constData(), utf8.size())});
    }
    case QVariant::StringList: {
        bool f = true;
        const QStringList & sl = refVariantValue<QStringList>(v, f);
        if (!f)
            throw BSONexception(QString("Error in %1")
                                .arg(v.typeName()));

        auto array_builder = bsoncxx::builder::basic::array{};

        for (auto iter = sl.constBegin();
             iter != sl.constEnd();
             ++iter) {
            array_builder.append(toBsonValue(*iter,
                                             data_lst,
                                             b_docs,
                                             b_arrays));
        }

        b_arrays << bsoncxx::array::value(array_builder.view());

        return value(b_array{b_arrays.last().view()});
    }
    case QVariant::LongLong:
        return value(b_int64{v.toLongLong()});
    case QVariant::UInt:
        return value(b_int64{v.toUInt()});
    case QVariant::Map: {
        bool f = true;
        const QVariantMap & obj = refVariantValue<QVariantMap>(v, f);
        if (!f)
            throw BSONexception(QString("Error in %1")
                                .arg(v.typeName()));

        using bsoncxx::builder::basic::kvp;
        auto doc = builder::basic::document{};

        auto it = obj.cbegin();
        while(it != obj.cend()) {
            const std::string name = it.key().toStdString();
            bool ok = true;
            doc.append(kvp(name, toBsonValue(it.value(),
                                             data_lst,
                                             b_docs,
                                             b_arrays)));
            Q_ASSERT(ok);
            ++it;
        }
        b_docs << bsoncxx::document::value(doc.view());

        return value(b_document{b_docs.last().view()});
    }
    case QVariant::List: {
        bool f = true;
        const QVariantList list = refVariantValue<QVariantList>(v, f);
        if (!f)
            throw BSONexception(QString("Error in %1")
                                .arg(v.typeName()));

        auto array_builder = bsoncxx::builder::basic::array{};

        for (auto iter = list.constBegin();
             iter != list.constEnd();
             ++iter) {
            array_builder.append(toBsonValue(*iter,
                                             data_lst,
                                             b_docs,
                                             b_arrays));
        }

        b_arrays << bsoncxx::array::value(array_builder.view());

        return value(b_array{b_arrays.last().view()});
    }
    case QVariant::Double:
        return value(b_double{v.toDouble()});
    case QVariant::Bool:
        return value(b_bool{v.toBool()});
    case QVariant::Time: return toCustomBSONBinary(v, data_lst);
    case QVariant::Date: return toCustomBSONBinary(v, data_lst);
    case QVariant::DateTime: {
        bool f = true;
        const QDateTime & data = refVariantValue<QDateTime>(v, f);
        if (!f)
            throw BSONexception(QString("Error in %1")
                                .arg(v.typeName()));

        milliseconds secs(data.toMSecsSinceEpoch());
        return value(b_date(secs));
    }
    case QVariant::Invalid:
        return value(b_null());
    case QVariant::ByteArray: {
        bool f = true;
        const QByteArray & binary = refVariantValue<QByteArray>(v, f);
        if (!f)
            throw BSONexception(QString("Error in %1")
                                .arg(v.typeName()));

        b_binary bin;
        bin.sub_type = binary_sub_type::k_binary;

        bin.size = binary.size();
        bin.bytes = (uint8_t*) binary.constData();

        return value(bin);
    }
    case QVariant::Uuid: {
        bool f = true;
        const QUuid & uuid = refVariantValue<QUuid>(v, f);
        if (!f)
            throw BSONexception(QString("Error in %1")
                                .arg(v.typeName()));

        QByteArray blob = uuid.toRfc4122();

        b_binary bin;
        bin.sub_type = binary_sub_type::k_uuid;

        bin.size = blob.size();
        bin.bytes = (uint8_t*) blob.constData();

        return value(bin);
    }
    case QVariant::UserType: {

        if (v.userType() == qMetaTypeId<BSONbinary>()) {
            bool f = true;
            const BSONbinary & binary = refVariantValue<BSONbinary>(v, f);
            if (!f)
                throw BSONexception(QString("Error in %1")
                                    .arg(v.typeName()));

            b_binary bin;

            switch (binary.type) {
            case BSONbinary::Unknown :
                bin.sub_type = binary_sub_type::k_binary;
                break;
            case BSONbinary::Function :
                bin.sub_type = binary_sub_type::k_function;
                break;
            case BSONbinary::MD5 :
                bin.sub_type = binary_sub_type::k_md5;
                break;
//            case BSONbinary::User :
//                bin.sub_type = binary_sub_type::k_user;
            }

            bin.size = binary.data.size();
            bin.bytes = (uint8_t*) binary.data.constData();

            return value(bin);
        }
        if (v.userType() == qMetaTypeId<BSONcode>()) {
            bool f = true;
            const BSONcode & binary = refVariantValue<BSONcode>(v, f);
            if (!f)
                throw BSONexception(QString("Error in %1")
                                    .arg(v.typeName()));

            return value{b_code{binary.code.toStdString()}};
        }
        if (v.userType() == qMetaTypeId<BSONcodeWscope>()) {
            bool f = true;
            auto & binary = refVariantValue<BSONcodeWscope>(v, f);
            if (!f)
                throw BSONexception(QString("Error in %1")
                                    .arg(v.typeName()));

            return value(b_codewscope{binary.code.toStdString(), toBson(binary.scope)});
        }
        if (v.userType() == qMetaTypeId<BSONmaxkey>()) {
            return value(b_maxkey{});
        }
        if (v.userType() == qMetaTypeId<BSONminkey>()) {
            return value(b_minkey{});
        }
        if (v.userType() == qMetaTypeId<BSONoid>()) {
            bool f = true;
            const BSONoid & binary = refVariantValue<BSONoid>(v, f);
            if (!f)
                throw BSONexception(QString("Error in %1")
                                    .arg(v.typeName()));

            return value(b_oid{oid(binary.toHex().toStdString())});
        }
        if (v.userType() == qMetaTypeId<BSONregexp>()) {
            bool f = true;
            const BSONregexp & binary = refVariantValue<BSONregexp>(v, f);
            if (!f)
                throw BSONexception(QString("Error in %1")
                                    .arg(v.typeName()));

            return value(b_regex{binary.regexp.toStdString(), binary.regexp.toStdString()});
        }

    }
    default:
        if (v.canConvert(QVariant::Map))
            return toBsonValue(v.toMap(), data_lst,
                               b_docs, b_arrays);
        if (v.canConvert(QVariant::List))
            return toBsonValue(v.toList(), data_lst,
                               b_docs, b_arrays);
        if (v.canConvert(QVariant::LongLong))
            return toBsonValue(v.toLongLong(), data_lst,
                               b_docs, b_arrays);
        if (v.canConvert(QVariant::Int))
            return toBsonValue(v.toInt(), data_lst,
                               b_docs, b_arrays);
        if (v.canConvert(QVariant::String))
            return toBsonValue(v.toString(), data_lst,
                               b_docs, b_arrays);

        throw BSONexception(QString("Error in unknown type %1")
                            .arg(v.typeName()));
    }
}

QVariant fromBsonValue(const bsoncxx::types::value & value) {
    using namespace bsoncxx;
    using namespace bsoncxx::types;
    using bsoncxx::type;
    using bsoncxx::binary_sub_type;

    switch (value.type()) {
    case type::k_double: return value.get_double().value;
    case type::k_utf8: {
        const stdx::string_view & view = value.get_utf8().value;
        QByteArray data(view.data(), view.size());
        return QString::fromUtf8(data);
    }
    case type::k_undefined: return QVariant();
    case type::k_oid: {
        BSONoid id;
        const bsoncxx::oid & oid = value.get_oid().value;
        id.data = QByteArray::fromHex( QString::fromStdString(oid.to_string()).toLatin1() );
        id.time = QDateTime::fromSecsSinceEpoch((qint64) oid.get_time_t());
        return QVariant::fromValue(id);
    }
    case type::k_bool: return value.get_bool().value;
    case type::k_date: {
        QDateTime res;
        res.setMSecsSinceEpoch(value.get_date().value.count());
        return res;
    }
    case type::k_binary: {
        const b_binary & binary = value.get_binary();

        switch (binary.sub_type) {
        case binary_sub_type::k_uuid : {
            QByteArray ba((const char*) binary.bytes, binary.size);
            return QUuid(ba);
        }
        case binary_sub_type::k_binary : {
            return QByteArray((const char*) binary.bytes, binary.size);
        }
        case binary_sub_type::k_function : {
            BSONbinary data;
            data.type = BSONbinary::Function;
            data.data = QByteArray((const char*) binary.bytes, binary.size);
            return QVariant::fromValue(data);
        }
        case binary_sub_type::k_md5 : {
            BSONbinary data;
            data.type = BSONbinary::MD5;
            data.data = QByteArray((const char*) binary.bytes, binary.size);
            return QVariant::fromValue(data);
        }
        case binary_sub_type::k_user :
            return fromCustomBSONBinary(binary);
        default:
            break;
        }
    }
    case type::k_null: QVariant();
    case type::k_regex: {
        BSONregexp re;
        re.regexp = QString::fromStdString(value.get_regex().regex.to_string());
        re.options = QString::fromStdString(value.get_regex().options.to_string());
        return QVariant::fromValue(re);
    }
    case type::k_code: {
        BSONcode code;
        code.code = QString::fromStdString(value.get_code().code.to_string());
        return QVariant::fromValue(code);
    }
    case type::k_array: {
        QVariantList res;

        const b_array & array = value.get_array();
        for (auto iter = array.value.cbegin();
             iter != array.value.cend();
             ++iter) {
            res << fromBsonValue((*iter).get_value());
        }

        return res;
    }
    case type::k_document: {

        QVariantMap res;

        const b_document & doc = value.get_document();

        for (auto iter = doc.value.cbegin();
             iter != doc.value.cend();
             ++iter) {
            const bsoncxx::document::element & element = (*iter);
            res.insert(
                        QString::fromStdString(element.key().to_string()),
                        fromBsonValue(element.get_value()));
        }
        return res;
    }
    case type::k_int32:
        return QVariant(value.get_int32().value);
    case type::k_int64:
        return QVariant((qint64) value.get_int64().value);
    default:
        throw BSONexception(QString("Error in unknown type %1")
                            .arg((int) value.type()));
    }
}

void initTypes() {
    static QAtomicInt inited = 0;

    if (inited.testAndSetOrdered(0, 1)) {

        qRegisterMetaType<BSONbinary>("BSONbinary");
        QMetaType::registerDebugStreamOperator<BSONbinary>();
        QMetaType::registerEqualsComparator<BSONbinary>();
        qRegisterMetaTypeStreamOperators<BSONbinary>();

        qRegisterMetaType<BSONoid>("BSONoid");
        QMetaType::registerEqualsComparator<BSONoid>();
        QMetaType::registerDebugStreamOperator<BSONoid>();
        qRegisterMetaTypeStreamOperators<BSONoid>();

        qRegisterMetaType<BSONregexp>("BSONregexp");
        QMetaType::registerEqualsComparator<BSONregexp>();
        QMetaType::registerDebugStreamOperator<BSONregexp>();
        qRegisterMetaTypeStreamOperators<BSONregexp>();

        qRegisterMetaType<BSONcode>("BSONcode");
        QMetaType::registerEqualsComparator<BSONcode>();
        QMetaType::registerDebugStreamOperator<BSONcode>();
        qRegisterMetaTypeStreamOperators<BSONcode>();

        qRegisterMetaType<BSONcodeWscope>("BSONcodeWscope");
        QMetaType::registerEqualsComparator<BSONcodeWscope>();
        QMetaType::registerDebugStreamOperator<BSONcodeWscope>();
        qRegisterMetaTypeStreamOperators<BSONcodeWscope>();

        qRegisterMetaType<BSONmaxkey>("BSONmaxkey");
        QMetaType::registerEqualsComparator<BSONmaxkey>();
        QMetaType::registerDebugStreamOperator<BSONmaxkey>();
        qRegisterMetaTypeStreamOperators<BSONmaxkey>();

        qRegisterMetaType<BSONminkey>("BSONminkey");
        QMetaType::registerEqualsComparator<BSONminkey>();
        QMetaType::registerDebugStreamOperator<BSONminkey>();
        qRegisterMetaTypeStreamOperators<BSONminkey>();

        inited = 2;
    } else while (inited == 1);
}
}

bsoncxx::document::value toBson(const QVariantMap & obj, bool &ok)
noexcept
{
    using namespace bsoncxx;
    try {
        return toBson(obj);
    } catch (BSONexception & e) {
        Q_UNUSED(e)
        ok = false;
        return document::value(builder::basic::document{});
    } catch (...) {
        throw BSONexception("BSON::fromBson unknown exception");
    }
}

bsoncxx::document::value toBson(const QVariantMap & obj)
{
    using namespace bsoncxx;
    using bsoncxx::builder::basic::kvp;
    using namespace _private;
    auto doc = builder::basic::document{};

    initTypes();

    QList<QByteArray> data_lst;
    QList<bsoncxx::document::value> b_docs;
    QList<bsoncxx::array::value> b_arrays;

    QVariantMap::const_iterator it = obj.begin();
    while(it != obj.end()) {
        const std::string name = it.key().toStdString();
        doc.append(kvp(name, toBsonValue(it.value(), data_lst,
                                         b_docs, b_arrays)));
        ++it;
    }
    return document::value(doc.view());
}

QVariantMap fromBson(const bsoncxx::document::value &bson, bool &ok)
noexcept
{
    try {
        return fromBson(bson.view());
    } catch (BSONexception & e) {
        qDebug() << "from BSON error" << e.data();
        ok = false;
        return QVariantMap();
    }
}

QVariantMap fromBson(const bsoncxx::document::value &bson)
{
    return fromBson(bson.view());
}

QVariantMap fromBson(const bsoncxx::document::view & bson, bool &ok)
noexcept
{
    try {
        return fromBson(bson);
    } catch (BSONexception &e) {
        qDebug() << "from BSON error" << e.data();
        ok = false;
        return QVariantMap();
    }
}

QVariantMap fromBson(const bsoncxx::document::view &bson)
{
    QVariantMap obj;

    for (auto iter = bson.cbegin();
         iter != bson.cend();
         ++iter) {

        const bsoncxx::document::element & elem = (*iter);

        try {
            const QString key = QString::fromStdString(elem.key().to_string());
            const QVariant value =
                    _private::fromBsonValue(elem.get_value());
            obj.insert(key, value);
        } catch (bsoncxx::exception & e) {
            throw BSONexception(QString::fromStdString(e.code().message()));
        } catch (...) {
            throw BSONexception("BSON::fromBson unknown exception");
        }
    }

    return obj;
}

QVariant fromBsonValue(const bsoncxx::types::value &value, bool & ok)
noexcept
{
    ok = true;
    try {
        return _private::fromBsonValue(value);
    } catch (BSONexception &) {
        ok = false;
        return QVariant();
    } catch (...) {
        qDebug() << "BSON::fromBsonValue unknown exception";
        return QVariant();
    }
}

QVariant fromBsonValue(const bsoncxx::types::value &value)
{
    return _private::fromBsonValue(value);
}

void init()
{
    _private::initTypes();
}

}

bool operator==(const BSONbinary &rhs, const BSONbinary &lhs) {
    return lhs.type == rhs.type &&
            lhs.data == rhs.data;
}

QDebug operator<<(QDebug debug, const BSONbinary &c)
{
    QDebugStateSaver saver(debug);
    Q_UNUSED(saver)
    debug.nospace() << "BSONbinary(" << c.type << ", " << "data size" << c.data.size() << ")";
    return debug;
}

QDataStream &operator<<(QDataStream &out, const BSONbinary &value)
{
    quint8 type = value.type;
    out << type;
    out << value.data;

    return out;
}

QDataStream &operator>>(QDataStream &in, BSONbinary &value)
{
    quint8 type;
    in >> type;

    switch (type) {
    case BSONbinary::Unknown:
        value.type = BSONbinary::Unknown;
        break;
    case BSONbinary::Function:
        value.type = BSONbinary::Function;
        break;
    case BSONbinary::MD5:
        value.type = BSONbinary::MD5;
        break;
//    case BSONbinary::User:
//        value.type = BSONbinary::User;
//        break;
    }

    in >> value.data;

    return in;
}

bool operator==(const BSONoid &lhs, const BSONoid &rhs) {
    return lhs.data == rhs.data;
}

QDebug operator<<(QDebug debug, const BSONoid &c)
{
    QDebugStateSaver saver(debug);
    Q_UNUSED(saver)
    debug.nospace() << "BSONoid(" << c.toHex() << ", " << c.time.toString(Qt::ISODate) << ")";
    return debug;
}

QDataStream &operator<<(QDataStream &out, const BSONoid &value)
{
    out << value.data << value.time;
    return out;
}

QDataStream &operator>>(QDataStream &in, BSONoid &value)
{
    in >> value.data;
    in >> value.time;
    return in;
}

bool operator==(const BSONregexp &lhs, const BSONregexp &rhs) {
    return lhs.regexp == rhs.regexp &&
            lhs.options == rhs.options;
}

QDebug operator<<(QDebug debug, const BSONregexp &c)
{
    QDebugStateSaver saver(debug);
    Q_UNUSED(saver)
    debug.nospace() << "BSONregexp(" << c.regexp << ", " << c.options << ")";
    return debug;
}

QDataStream &operator<<(QDataStream &out, const BSONregexp &value)
{
    out << value.regexp << value.options;
    return out;
}

QDataStream &operator>>(QDataStream &in, BSONregexp &value)
{
    in >> value.regexp;
    in >> value.options;
    return in;
}

bool operator==(const BSONcode &lhs, const BSONcode &rhs) {
    return lhs.code == rhs.code;
}

QDebug operator<<(QDebug debug, const BSONcode &c)
{
    QDebugStateSaver saver(debug);
    Q_UNUSED(saver)
    debug.nospace() << "BSONcode(" << c.code << ")";
    return debug;
}

bool operator==(const BSONcodeWscope &lhs, const BSONcodeWscope &rhs) {
    return lhs.code == rhs.code &&
            lhs.scope == rhs.scope;
}

QDebug operator<<(QDebug debug, const BSONcodeWscope &c)
{
    QDebugStateSaver saver(debug);
    Q_UNUSED(saver)
    debug.nospace() << "BSONcodeWscope(" << c.code << ", " << "scope..." << ")";
    return debug;
}

QDataStream &operator<<(QDataStream &out, const BSONcodeWscope &value)
{
    out << value.code;
    out << value.scope;
    return out;
}

QDataStream &operator>>(QDataStream &in, BSONcodeWscope &value)
{
    in >> value.code;
    in >> value.scope;
    return in;
}

bool operator==(const BSONmaxkey &, const BSONmaxkey &) {
    return true;
}

QDebug operator<<(QDebug debug, const BSONmaxkey &)
{
    QDebugStateSaver saver(debug);
    Q_UNUSED(saver)
    debug.nospace() << "BSONmaxkey()";
    return debug;
}

QDataStream &operator<<(QDataStream &out, const BSONmaxkey &)
{
    return out;
}

QDataStream &operator>>(QDataStream &in, BSONmaxkey &)
{
    return in;
}

bool operator==(const BSONminkey &, const BSONminkey &) {
    return true;
}

QDebug operator<<(QDebug debug, const BSONminkey &)
{
    QDebugStateSaver saver(debug);
    Q_UNUSED(saver)
    debug.nospace() << "BSONminkey()";
    return debug;
}

QDataStream &operator<<(QDataStream &out, const BSONcode &value)
{
    out << value.code;
    return out;
}

QDataStream &operator>>(QDataStream &in, BSONcode &value)
{
    in >> value.code;
    return in;
}

QDataStream &operator<<(QDataStream &out, const BSONminkey &)
{
    return out;
}

QDataStream &operator>>(QDataStream &in, BSONminkey &)
{
    return in;
}

